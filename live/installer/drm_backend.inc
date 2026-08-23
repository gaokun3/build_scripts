/* DRM + libinput 后端。由 gk3-installer.c 在 GK3_DRM 下 #include。
 *
 * 单独一个 .inc 而不是独立编译单元：它要用到主文件里的 App / draw() /
 * hit_test() 这些 static 符号，拆成两个 .o 就得把它们全部导出 —— 
 * 为了一个只有两处调用的后端，不值得。
 */
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>
#include <sys/mman.h>
#include <time.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <libinput.h>
#include <libudev.h>

typedef struct {
    int fd;
    uint32_t conn_id, crtc_id, fb_id, handle;
    uint32_t width, height, pitch;
    uint64_t size;
    uint8_t *map;
    drmModeModeInfo mode;
    drmModeCrtc *saved;
} Drm;

/* 面板是 1600x2560 竖屏，平板横着用 —— 逻辑坐标 1280x800 旋转 90° 再铺满。
 * ⚠️ 这个逆变换和 drm_present() 里的 cairo 变换【必须成对改】，
 *    否则手指点 A 处、程序以为点了 B 处，而且这种错很难从截图上看出来。 */
static void drm_to_logical(Drm *d, double dx, double dy, double *lx, double *ly)
{
    *lx = dy / ((double)d->height / UI_H);
    *ly = ((double)d->width - dx) / ((double)d->width / UI_W);
}

static int drm_open(Drm *d)
{
    /* ⚠️ 本机 DRM 主节点是 card1 不是 card0（见 stage4-findings #60）——
     *    所以扫一遍，别写死。 */
    for (int i = 0; i < 8; i++) {
        char path[64]; snprintf(path, sizeof path, "/dev/dri/card%d", i);
        int fd = open(path, O_RDWR | O_CLOEXEC);
        if (fd < 0) continue;
        drmModeRes *res = drmModeGetResources(fd);
        if (!res) { close(fd); continue; }
        for (int c = 0; c < res->count_connectors; c++) {
            drmModeConnector *conn = drmModeGetConnector(fd, res->connectors[c]);
            if (!conn) continue;
            if (conn->connection == DRM_MODE_CONNECTED && conn->count_modes > 0) {
                d->fd = fd; d->conn_id = conn->connector_id; d->mode = conn->modes[0];
                drmModeEncoder *enc = conn->encoder_id ? drmModeGetEncoder(fd, conn->encoder_id) : NULL;
                d->crtc_id = (enc && enc->crtc_id) ? enc->crtc_id : (uint32_t)res->crtcs[0];
                if (enc) drmModeFreeEncoder(enc);
                d->saved = drmModeGetCrtc(fd, d->crtc_id);
                fprintf(stderr, "[gk3] %s 上的显示输出：%ux%u\n", path,
                        d->mode.hdisplay, d->mode.vdisplay);
                drmModeFreeConnector(conn); drmModeFreeResources(res);
                return 0;
            }
            drmModeFreeConnector(conn);
        }
        drmModeFreeResources(res); close(fd);
    }
    fprintf(stderr, "!! 没找到已连接的显示输出\n");
    return -1;
}

static int drm_make_fb(Drm *d)
{
    struct drm_mode_create_dumb creq;
    memset(&creq, 0, sizeof creq);
    creq.width = d->mode.hdisplay; creq.height = d->mode.vdisplay; creq.bpp = 32;
    if (drmIoctl(d->fd, DRM_IOCTL_MODE_CREATE_DUMB, &creq) < 0) {
        fprintf(stderr, "!! CREATE_DUMB 失败: %s\n", strerror(errno)); return -1;
    }
    d->width = creq.width; d->height = creq.height;
    d->pitch = creq.pitch; d->size = creq.size; d->handle = creq.handle;
    if (drmModeAddFB(d->fd, d->width, d->height, 24, 32, d->pitch, d->handle, &d->fb_id)) {
        fprintf(stderr, "!! AddFB 失败: %s\n", strerror(errno)); return -1;
    }
    struct drm_mode_map_dumb mreq;
    memset(&mreq, 0, sizeof mreq);
    mreq.handle = d->handle;
    if (drmIoctl(d->fd, DRM_IOCTL_MODE_MAP_DUMB, &mreq) < 0) {
        fprintf(stderr, "!! MAP_DUMB 失败: %s\n", strerror(errno)); return -1;
    }
    d->map = mmap(0, d->size, PROT_READ | PROT_WRITE, MAP_SHARED, d->fd, mreq.offset);
    if (d->map == MAP_FAILED) { fprintf(stderr, "!! mmap 失败\n"); return -1; }
    memset(d->map, 0, d->size);
    return drmModeSetCrtc(d->fd, d->crtc_id, d->fb_id, 0, 0, &d->conn_id, 1, &d->mode);
}

static void drm_present(Drm *d, App *a)
{
    cairo_surface_t *s = cairo_image_surface_create_for_data(
        d->map, CAIRO_FORMAT_RGB24, d->width, d->height, d->pitch);
    cairo_t *cr = cairo_create(s);
    cairo_translate(cr, d->width, 0);
    cairo_rotate(cr, M_PI / 2);
    cairo_scale(cr, (double)d->height / UI_H, (double)d->width / UI_W);
    draw(cr, a);
    cairo_destroy(cr);
    cairo_surface_flush(s);
    cairo_surface_destroy(s);
}

static int li_open(const char *path, int flags, void *ud)
{ (void)ud; int fd = open(path, flags); return fd < 0 ? -errno : fd; }
static void li_close(int fd, void *ud) { (void)ud; close(fd); }
static const struct libinput_interface LI_IFACE = {
    .open_restricted = li_open, .close_restricted = li_close
};

static double now_s(void)
{
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

/* 执行安装，边跑边刷进度屏。
 *
 * 后端把进度打在 stderr（`PROGRESS <百分比> <说明>`），其余输出当成日志。
 * ⚠️ 这里【不做任何分区决策】—— 方案是前面算好的，这一步只是执行加显示。
 */
static void run_install(App *a, Drm *d)
{
    char call[1024];
    /* 把界面上的选择原样转成后端参数。--apply 是唯一会写盘的入口。 */
    if (a->mode_wipe)
        snprintf(call, sizeof call,
                 "gk3_apply --disk %s --mode wipe --rescue %s --release /media/gk3/gaokun3/payload 2>&1",
                 a->disks[a->seldisk].path, a->want_rescue ? "yes" : "no");
    else
        snprintf(call, sizeof call,
                 "gk3_apply --disk %s --mode alongside --rescue %s --region-start %ld --region-end %ld"
                 " --esp %s --release /media/gk3/gaokun3/payload 2>&1",
                 a->disks[a->seldisk].path, a->want_rescue ? "yes" : "no",
                 a->frees[a->selfree].start, a->frees[a->selfree].end, a->esp);

    char cmd[1400];
    snprintf(cmd, sizeof cmd, ". '%s' && %s", g_lib, call);
    FILE *f = popen(cmd, "r");
    if (!f) { a->failed = true; snprintf(a->step, sizeof a->step, "起不动安装后端"); a->screen = SC_DONE; return; }

    char line[1024];
    while (fgets(line, sizeof line, f)) {
        size_t n = strlen(line);
        while (n && (line[n-1] == '\n' || line[n-1] == '\r')) line[--n] = 0;
        if (!strncmp(line, "PROGRESS ", 9)) {
            int pct = 0; char msg[200] = {0};
            if (sscanf(line + 9, "%d %199[^\n]", &pct, msg) >= 1) {
                a->pct = pct;
                if (msg[0]) snprintf(a->step, sizeof a->step, "%s", msg);
            }
        } else if (line[0]) {
            /* 日志只留最后几行 —— 屏幕就那么大，而且出错时最后几行最有用 */
            int keep = (int)(sizeof a->logtail / sizeof a->logtail[0]);
            if (a->nlog < keep) a->nlog++;
            for (int i = 0; i < a->nlog - 1; i++)
                memcpy(a->logtail[i], a->logtail[i+1], sizeof a->logtail[0]);
            snprintf(a->logtail[a->nlog - 1], sizeof a->logtail[0], "%s", line);
        }
        drm_present(d, a);
    }
    int rc = pclose(f);
    a->failed = (rc != 0);
    if (a->failed && !a->step[0])
        snprintf(a->step, sizeof a->step, "安装失败（退出码 %d）", rc);
    if (!a->failed) { a->pct = 100; snprintf(a->step, sizeof a->step, "完成"); }
    a->screen = SC_DONE;
    drm_present(d, a);
}

static int run_drm(void)
{
    Drm d;
    memset(&d, 0, sizeof d);
    if (drm_open(&d) || drm_make_fb(&d)) return 1;

    struct udev *ud = udev_new();
    struct libinput *li = libinput_udev_create_context(&LI_IFACE, NULL, ud);
    if (!li || libinput_udev_assign_seat(li, "seat0")) {
        fprintf(stderr, "!! libinput 起不来 —— 没有输入就没法用\n");
        return 1;
    }

    App a;
    memset(&a, 0, sizeof a);
    a.seldisk = -1; a.selfree = -1; a.mode_wipe = true; a.want_rescue = true;
    backend("gk3_probe", on_probe, &a);
    if (a.ndisks == 1) a.seldisk = 0;   /* 只有一块盘就直接选中 */

    bool dirty = true, holding = false;
    double hold_start = 0;

    for (;;) {
        if (dirty) { drm_present(&d, &a); dirty = false; }

        struct pollfd pfd;
        pfd.fd = libinput_get_fd(li); pfd.events = POLLIN; pfd.revents = 0;
        poll(&pfd, 1, holding ? 40 : 400);
        libinput_dispatch(li);

        struct libinput_event *ev;
        while ((ev = libinput_get_event(li))) {
            enum libinput_event_type t = libinput_event_get_type(ev);
            double lx = -1, ly = -1;
            bool down = false, up = false;
            if (t == LIBINPUT_EVENT_TOUCH_DOWN) {
                struct libinput_event_touch *te = libinput_event_get_touch_event(ev);
                double dx = libinput_event_touch_get_x_transformed(te, d.width);
                double dy = libinput_event_touch_get_y_transformed(te, d.height);
                drm_to_logical(&d, dx, dy, &lx, &ly);
                down = true;
            } else if (t == LIBINPUT_EVENT_TOUCH_UP) {
                up = true;
            }
            if (down && lx >= 0) {
                int id = hit_test(lx, ly);
                if (id == ID_CONFIRM_HOLD) { holding = true; hold_start = now_s(); }
                else if (id != ID_NONE && on_tap(&a, id)) dirty = true;
            }
            /* ⚠️ 松手就重来：这一步不可撤销，必须是明确的持续意图 */
            if (up && holding) { holding = false; a.hold = 0; dirty = true; }
            libinput_event_destroy(ev);
        }

        if (holding) {
            a.hold = (now_s() - hold_start) / 2.0;   /* 按住 2 秒 */
            if (a.hold >= 1.0) {
                holding = false; a.hold = 1.0;
                a.screen = SC_RUN; a.pct = 0;
                snprintf(a.step, sizeof a.step, "准备中……");
                drm_present(&d, &a);
                run_install(&a, &d);
                dirty = true;
            } else dirty = true;
        }
    }
    return 0;
}
