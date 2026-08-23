/*
 * gaokun3 图形安装器（LiveCD）
 *
 * 直接画 KMS，不装合成器 —— weston/wayland 一整套对一个只有六屏的安装器
 * 来说太重了，而这个镜像的体积目标是 ≤120 MiB。
 *
 * ★ 两个渲染后端，编译期选：
 *     PNG  离线，把每一屏写成图片。**没有目标硬件也能验证排版和文案。**
 *     DRM  设备上，画到 KMS dumb buffer。
 *   之所以这么设计，是因为这台机器经常被占用（它同时是作者的日用平板），
 *   而"改一行要等能上机才能看结果"是不可接受的迭代速度。
 *
 * 逻辑坐标固定 1280x800，输出时整体缩放。面板物理是 1600x2560 竖屏，
 * 平板横着用，所以 DRM 后端会旋转 90°。
 *
 * 构建：见 live/installer/Makefile
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <cairo/cairo.h>
#include <pango/pangocairo.h>
#include "strings.h"

#define UI_W 1280.0
#define UI_H 800.0

/* ── 主题 ──────────────────────────────────────────────────────────────── */
typedef struct { double r, g, b, a; } Col;
static const Col C_BG      = {0.063, 0.078, 0.094, 1};
static const Col C_SURF    = {0.102, 0.125, 0.161, 1};
static const Col C_SURF2   = {0.137, 0.169, 0.212, 1};
static const Col C_LINE    = {0.204, 0.243, 0.294, 1};
static const Col C_TEXT    = {0.910, 0.933, 0.961, 1};
static const Col C_MUTED   = {0.545, 0.600, 0.659, 1};
static const Col C_ACCENT  = {0.302, 0.639, 1.000, 1};
static const Col C_DANGER  = {1.000, 0.420, 0.369, 1};
static const Col C_OK      = {0.353, 0.820, 0.604, 1};
static const Col C_DIM     = {0.300, 0.340, 0.400, 1};

static void set_col(cairo_t *cr, Col c) { cairo_set_source_rgba(cr, c.r, c.g, c.b, c.a); }

static void rrect(cairo_t *cr, double x, double y, double w, double h, double r)
{
    if (r > w / 2) r = w / 2;
    if (r > h / 2) r = h / 2;
    cairo_new_sub_path(cr);
    cairo_arc(cr, x + w - r, y + r,     r, -M_PI / 2, 0);
    cairo_arc(cr, x + w - r, y + h - r, r, 0, M_PI / 2);
    cairo_arc(cr, x + r,     y + h - r, r, M_PI / 2, M_PI);
    cairo_arc(cr, x + r,     y + r,     r, M_PI, 3 * M_PI / 2);
    cairo_close_path(cr);
}

/* Pango 画文字。用 "Sans" 让 fontconfig 去挑 —— 镜像里装了 wqy-zenhei，
 * 中文会自动回退过去。写死字体名反而会在换字体包时静默变成方框。 */
static void text(cairo_t *cr, double x, double y, double w, double size,
                 Col c, const char *align, const char *fmt, ...)
{
    char buf[2048];
    va_list ap; va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);

    PangoLayout *l = pango_cairo_create_layout(cr);
    char font[64]; snprintf(font, sizeof font, "Sans %g", size);
    PangoFontDescription *fd = pango_font_description_from_string(font);
    pango_layout_set_font_description(l, fd);
    pango_font_description_free(fd);
    pango_layout_set_text(l, buf, -1);
    if (w > 0) {
        pango_layout_set_width(l, (int)(w * PANGO_SCALE));
        pango_layout_set_wrap(l, PANGO_WRAP_WORD_CHAR);
        if (!strcmp(align, "center")) pango_layout_set_alignment(l, PANGO_ALIGN_CENTER);
        else if (!strcmp(align, "right")) pango_layout_set_alignment(l, PANGO_ALIGN_RIGHT);
    }
    set_col(cr, c);
    cairo_move_to(cr, x, y);
    pango_cairo_show_layout(cr, l);
    g_object_unref(l);
}

static double text_h(cairo_t *cr, double w, double size, const char *s)
{
    PangoLayout *l = pango_cairo_create_layout(cr);
    char font[64]; snprintf(font, sizeof font, "Sans %g", size);
    PangoFontDescription *fd = pango_font_description_from_string(font);
    pango_layout_set_font_description(l, fd);
    pango_font_description_free(fd);
    pango_layout_set_text(l, s, -1);
    if (w > 0) { pango_layout_set_width(l, (int)(w * PANGO_SCALE)); pango_layout_set_wrap(l, PANGO_WRAP_WORD_CHAR); }
    int hh; pango_layout_get_pixel_size(l, NULL, &hh);
    g_object_unref(l);
    return hh;
}

/* ── 可点区域 ──────────────────────────────────────────────────────────── */
/* 每一帧重新登记。⚠️ 触摸目标最小 88 逻辑像素 —— 这机器没有鼠标，
 * 手指在 13 英寸屏上的实际接触面积远大于设计稿上看着的那点。 */
#define MAX_HITS 32
typedef struct { double x, y, w, h; int id; bool enabled; } Hit;
static Hit  g_hits[MAX_HITS];
static int  g_nhits;
static void hit_reset(void) { g_nhits = 0; }
static void hit_add(double x, double y, double w, double h, int id, bool en)
{
    if (g_nhits < MAX_HITS) g_hits[g_nhits++] = (Hit){x, y, w, h, id, en};
}
static int hit_test(double x, double y)
{
    for (int i = g_nhits - 1; i >= 0; i--) {
        Hit *t = &g_hits[i];
        if (t->enabled && x >= t->x && x <= t->x + t->w && y >= t->y && y <= t->y + t->h)
            return t->id;
    }
    return -1;
}

/* ── 控件 ──────────────────────────────────────────────────────────────── */
static void button(cairo_t *cr, double x, double y, double w, double h,
                   const char *label, int id, bool primary, bool enabled, bool danger)
{
    Col fill = primary ? (danger ? C_DANGER : C_ACCENT) : C_SURF2;
    if (!enabled) fill = C_DIM;
    rrect(cr, x, y, w, h, 14); set_col(cr, fill); cairo_fill(cr);
    if (!primary && enabled) {
        rrect(cr, x, y, w, h, 14); set_col(cr, C_LINE);
        cairo_set_line_width(cr, 2); cairo_stroke(cr);
    }
    Col tc = primary ? (Col){0.05,0.07,0.09,1} : (enabled ? C_TEXT : C_MUTED);
    double th = text_h(cr, w, 21, label);
    text(cr, x, y + (h - th) / 2, w, 21, tc, "center", "%s", label);
    hit_add(x, y, w, h, id, enabled);
}

static void card(cairo_t *cr, double x, double y, double w, double h, bool sel, bool enabled)
{
    rrect(cr, x, y, w, h, 18);
    set_col(cr, enabled ? C_SURF : (Col){0.09,0.10,0.12,1});
    cairo_fill(cr);
    rrect(cr, x, y, w, h, 18);
    set_col(cr, sel ? C_ACCENT : C_LINE);
    cairo_set_line_width(cr, sel ? 4 : 2);
    cairo_stroke(cr);
}

static void progress_bar(cairo_t *cr, double x, double y, double w, double pct)
{
    rrect(cr, x, y, w, 18, 9); set_col(cr, C_SURF2); cairo_fill(cr);
    if (pct > 0) {
        double fw = w * (pct / 100.0); if (fw < 18) fw = 18;
        rrect(cr, x, y, fw, 18, 9); set_col(cr, C_ACCENT); cairo_fill(cr);
    }
}

/* ── 应用状态 ──────────────────────────────────────────────────────────── */
typedef enum { SC_WELCOME, SC_DISK, SC_MODE, SC_OPTS, SC_CONFIRM, SC_RUN, SC_DONE } Screen;

typedef struct { char path[64]; long size_mib; char model[64]; int removable; } Disk;
typedef struct { char path[64]; long start, end, size_mib; char name[40], fs[16], os[16]; } Part;
typedef struct { char disk[64]; long start, end, size_mib; } FreeRgn;

#define MAXD 8
#define MAXP 64
#define MAXF 16
typedef struct {
    Screen screen;
    Disk    disks[MAXD]; int ndisks; int seldisk;
    Part    parts[MAXP]; int nparts;
    FreeRgn frees[MAXF]; int nfrees; int selfree;
    bool    mode_wipe;
    bool    want_rescue;
    char    esp[64];            /* 双系统模式下复用的 ESP */
    long    plan_userdata_mib;
    char    plan_err[160];
    double  pct;
    char    step[200];
    char    logtail[6][160]; int nlog;
    bool    failed;
    double  hold;               /* 确认页"按住"进度 0..1 */
} App;

enum {
    ID_NONE = -1,
    ID_START = 1, ID_QUIT, ID_BACK, ID_NEXT,
    ID_MODE_WIPE, ID_MODE_ALONG, ID_RESCUE_TOGGLE, ID_CONFIRM_HOLD, ID_REBOOT,
    ID_DISK0 = 100, ID_FREE0 = 200
};

static const char *human(long mib, char *buf, size_t n)
{
    if (mib >= 1024 * 1024) snprintf(buf, n, "%.1f TiB", mib / 1024.0 / 1024.0);
    else if (mib >= 1024)   snprintf(buf, n, "%.1f GiB", mib / 1024.0);
    else                    snprintf(buf, n, "%ld MiB", mib);
    return buf;
}

/* 双系统模式可不可用：要有【现成的 ESP】而且要有够大的空闲区 */
static bool along_ok(App *a, const char **why)
{
    if (!a->esp[0]) { *why = S_MODE_WHY_NOESP; return false; }
    long best = 0;
    for (int i = 0; i < a->nfrees; i++) if (a->frees[i].size_mib > best) best = a->frees[i].size_mib;
    if (best < 20644) { *why = S_MODE_WHY_NOROOM; return false; }
    return true;
}

/* ── 各屏 ──────────────────────────────────────────────────────────────── */
static void draw_chrome(cairo_t *cr, const char *title, const char *sub)
{
    set_col(cr, C_BG); cairo_paint(cr);
    text(cr, 64, 48, UI_W - 128, 34, C_TEXT, "left", "%s", title);
    if (sub && *sub) text(cr, 64, 100, UI_W - 128, 17, C_MUTED, "left", "%s", sub);
    set_col(cr, C_LINE); cairo_rectangle(cr, 64, 140, UI_W - 128, 2); cairo_fill(cr);
}

static void sc_welcome(cairo_t *cr, App *a)
{
    (void)a;
    draw_chrome(cr, S_WELCOME_TITLE, S_WELCOME_SUB);
    text(cr, 64, 200, UI_W - 128, 19, C_TEXT, "left",
         S_WELCOME_BODY);
    rrect(cr, 64, 320, UI_W - 128, 110, 14); set_col(cr, C_SURF); cairo_fill(cr);
    text(cr, 96, 344, UI_W - 192, 17, C_MUTED, "left",
         S_WELCOME_NOTE);
    button(cr, 64, UI_H - 140, 300, 88, S_BTN_START, ID_START, true, true, false);
    button(cr, 388, UI_H - 140, 240, 88, S_BTN_QUIT, ID_QUIT, false, true, false);
}

static void sc_disk(cairo_t *cr, App *a)
{
    draw_chrome(cr, S_DISK_TITLE, S_DISK_SUB);
    double y = 180;
    char b1[32];
    for (int i = 0; i < a->ndisks && i < 5; i++) {
        Disk *d = &a->disks[i];
        bool sel = (a->seldisk == i);
        card(cr, 64, y, UI_W - 128, 96, sel, true);
        text(cr, 96, y + 18, 600, 22, C_TEXT,  "left", "%s", d->path);
        text(cr, 96, y + 54, 600, 15, C_MUTED, "left", "%s", d->model);
        text(cr, UI_W - 360, y + 30, 264, 22, sel ? C_ACCENT : C_TEXT, "right",
             "%s", human(d->size_mib, b1, sizeof b1));
        hit_add(64, y, UI_W - 128, 96, ID_DISK0 + i, true);
        y += 112;
    }
    if (a->ndisks == 0)
        text(cr, 64, 200, UI_W - 128, 19, C_DANGER, "left", S_DISK_NONE);
    button(cr, 64, UI_H - 140, 200, 88, S_BTN_BACK, ID_BACK, false, true, false);
    button(cr, UI_W - 364, UI_H - 140, 300, 88, S_BTN_NEXT, ID_NEXT, true, a->seldisk >= 0, false);
}

static void sc_mode(cairo_t *cr, App *a)
{
    draw_chrome(cr, S_MODE_TITLE, S_MODE_SUB);
    const char *why = NULL;
    bool can_along = along_ok(a, &why);
    char b1[32];

    /* 清空整盘 */
    card(cr, 64, 180, (UI_W - 160) / 2, 300, a->mode_wipe, true);
    text(cr, 96, 208, 496, 24, C_TEXT, "left", S_MODE_WIPE_TITLE);
    text(cr, 96, 252, 496, 16, C_DANGER, "left", S_MODE_WIPE_WARN);
    text(cr, 96, 320, 496, 16, C_MUTED, "left",
         S_MODE_WIPE_BODY);
    hit_add(64, 180, (UI_W - 160) / 2, 300, ID_MODE_WIPE, true);

    /* 保留现有系统 */
    double x2 = 64 + (UI_W - 160) / 2 + 32;
    card(cr, x2, 180, (UI_W - 160) / 2, 300, !a->mode_wipe, can_along);
    text(cr, x2 + 32, 208, 496, 24, can_along ? C_TEXT : C_MUTED, "left", S_MODE_ALONG_TITLE);
    if (can_along) {
        long best = 0;
        for (int i = 0; i < a->nfrees; i++) if (a->frees[i].size_mib > best) best = a->frees[i].size_mib;
        text(cr, x2 + 32, 252, 496, 16, C_OK, "left",
             S_MODE_ALONG_OK);
        /* ⚠️ 这一行含设备路径（/dev/nvme0n1p1 这种不可断开的长 token）。
         *    字号比同级正文小一档、宽度取满卡片内宽 —— 否则 Pango 会从
         *    路径中间断行，看起来像两个不同的路径。 */
        text(cr, x2 + 32, 320, 496, 15, C_MUTED, "left",
             S_MODE_ALONG_INFO, human(best, b1, sizeof b1), a->esp);
    } else {
        text(cr, x2 + 32, 252, 496, 16, C_MUTED, "left", "%s", why ? why : "");
    }
    hit_add(x2, 180, (UI_W - 160) / 2, 300, ID_MODE_ALONG, can_along);

    button(cr, 64, UI_H - 140, 200, 88, S_BTN_BACK, ID_BACK, false, true, false);
    button(cr, UI_W - 364, UI_H - 140, 300, 88, S_BTN_NEXT, ID_NEXT, true, true, false);
}

static void sc_opts(cairo_t *cr, App *a)
{
    draw_chrome(cr, S_OPTS_TITLE, S_OPTS_SUB);
    card(cr, 64, 180, UI_W - 128, 130, false, true);
    text(cr, 96, 204, 700, 21, C_TEXT, "left", S_OPTS_RESCUE_TITLE);
    text(cr, 96, 240, 760, 15, C_MUTED, "left",
         S_OPTS_RESCUE_BODY);
    /* 开关 */
    double sx = UI_W - 210, sy = 218;
    rrect(cr, sx, sy, 120, 56, 28);
    set_col(cr, a->want_rescue ? C_ACCENT : C_SURF2); cairo_fill(cr);
    cairo_arc(cr, a->want_rescue ? sx + 92 : sx + 28, sy + 28, 22, 0, 2 * M_PI);
    set_col(cr, (Col){1,1,1,1}); cairo_fill(cr);
    hit_add(sx - 20, sy - 20, 160, 96, ID_RESCUE_TOGGLE, true);

    char b1[32];
    text(cr, 64, 350, UI_W - 128, 17, C_MUTED, "left",
         S_OPTS_DATA,
         a->plan_userdata_mib > 0 ? human(a->plan_userdata_mib, b1, sizeof b1) : S_OPTS_LATER);
    if (a->plan_err[0])
        text(cr, 64, 390, UI_W - 128, 17, C_DANGER, "left", "%s", a->plan_err);

    button(cr, 64, UI_H - 140, 200, 88, S_BTN_BACK, ID_BACK, false, true, false);
    button(cr, UI_W - 364, UI_H - 140, 300, 88, S_BTN_NEXT, ID_NEXT, true, !a->plan_err[0], false);
}

static void sc_confirm(cairo_t *cr, App *a)
{
    draw_chrome(cr, S_CONFIRM_TITLE, S_CONFIRM_SUB);
    char b1[32];
    if (a->mode_wipe) {
        text(cr, 64, 180, UI_W - 128, 22, C_DANGER, "left",
             S_CONFIRM_WIPE_HEAD);
        double y = 224;
        int shown = 0;
        for (int i = 0; i < a->nparts && shown < 6; i++) {
            Part *p = &a->parts[i];
            if (strncmp(p->path, a->disks[a->seldisk].path, strlen(a->disks[a->seldisk].path))) continue;
            text(cr, 96, y, 900, 16, C_TEXT, "left", "%s   %s   %s   %s",
                 p->path, human(p->size_mib, b1, sizeof b1),
                 p->os[0] ? p->os : "-", p->name[0] ? p->name : (p->fs[0] ? p->fs : "-"));
            y += 30; shown++;
        }
        if (shown == 0) { text(cr, 96, y, 900, 16, C_MUTED, "left", S_CONFIRM_NOPARTS); y += 30; }
        if (shown >= 6) { text(cr, 96, y, 900, 15, C_MUTED, "left", S_CONFIRM_MORE); y += 30; }
    } else {
        text(cr, 64, 180, UI_W - 128, 22, C_OK, "left",
             S_CONFIRM_ALONG_HEAD);
        text(cr, 96, 230, UI_W - 192, 17, C_MUTED, "left",
             S_CONFIRM_ALONG_BODY,
             human(a->frees[a->selfree >= 0 ? a->selfree : 0].size_mib, b1, sizeof b1), a->esp);
    }
    text(cr, 64, 470, UI_W - 128, 17, C_MUTED, "left",
         S_CONFIRM_RESCUE, a->want_rescue ? S_WORD_INSTALL : S_WORD_NOINSTALL);

    /* 按住确认 —— 触摸屏上单击太容易误触，而这一步不可撤销。
     * ⚠️ S_BTN_BACK和确认条【必须同一行、隔开】：第一版把返回摞在确认条正上方，
     *    离屏幕最危险的那个控件只有几像素，手指按下去很容易滑到下面那个。 */
    button(cr, 64, UI_H - 160, 200, 96, S_BTN_BACK, ID_BACK, false, true, false);
    double bx = 300, by = UI_H - 160, bw = UI_W - 364, bh = 96;
    rrect(cr, bx, by, bw, bh, 16); set_col(cr, C_SURF2); cairo_fill(cr);
    if (a->hold > 0) {
        cairo_save(cr); rrect(cr, bx, by, bw, bh, 16); cairo_clip(cr);
        set_col(cr, C_DANGER); cairo_rectangle(cr, bx, by, bw * a->hold, bh); cairo_fill(cr);
        cairo_restore(cr);
    }
    rrect(cr, bx, by, bw, bh, 16); set_col(cr, C_DANGER);
    cairo_set_line_width(cr, 3); cairo_stroke(cr);
    if (a->hold > 0)
        text(cr, bx, by + 34, bw, 22, C_TEXT, "center", S_CONFIRM_HOLD_BUSY, (int)(a->hold * 100));
    else
        text(cr, bx, by + 34, bw, 22, C_TEXT, "center", S_CONFIRM_HOLD_IDLE);
    hit_add(bx, by, bw, bh, ID_CONFIRM_HOLD, true);
}

static void sc_run(cairo_t *cr, App *a)
{
    draw_chrome(cr, S_RUN_TITLE, S_RUN_SUB);
    progress_bar(cr, 64, 200, UI_W - 128, a->pct);
    text(cr, 64, 240, UI_W - 128, 21, C_TEXT, "left", "%s", a->step);
    text(cr, UI_W - 200, 240, 136, 21, C_ACCENT, "right", "%d%%", (int)a->pct);
    double y = 320;
    for (int i = 0; i < a->nlog; i++) {
        text(cr, 64, y, UI_W - 128, 14, C_MUTED, "left", "%s", a->logtail[i]);
        y += 24;
    }
}

static void sc_done(cairo_t *cr, App *a)
{
    if (a->failed) {
        draw_chrome(cr, S_FAIL_TITLE, S_FAIL_SUB);
        text(cr, 64, 200, UI_W - 128, 19, C_DANGER, "left", "%s", a->step);
        double y = 280;
        for (int i = 0; i < a->nlog; i++) { text(cr, 64, y, UI_W - 128, 14, C_MUTED, "left", "%s", a->logtail[i]); y += 24; }
        button(cr, 64, UI_H - 140, 300, 88, S_BTN_QUIT, ID_QUIT, false, true, false);
    } else {
        draw_chrome(cr, S_DONE_TITLE, NULL);
        text(cr, 64, 200, UI_W - 128, 19, C_TEXT, "left",
             S_DONE_BODY,
             a->want_rescue ? S_DONE_RESCUE : "");
        button(cr, 64, UI_H - 140, 300, 88, S_BTN_REBOOT, ID_REBOOT, true, true, false);
        button(cr, 388, UI_H - 140, 240, 88, S_BTN_QUIT, ID_QUIT, false, true, false);
    }
}

static void draw(cairo_t *cr, App *a)
{
    hit_reset();
    switch (a->screen) {
        case SC_WELCOME: sc_welcome(cr, a); break;
        case SC_DISK:    sc_disk(cr, a);    break;
        case SC_MODE:    sc_mode(cr, a);    break;
        case SC_OPTS:    sc_opts(cr, a);    break;
        case SC_CONFIRM: sc_confirm(cr, a); break;
        case SC_RUN:     sc_run(cr, a);     break;
        case SC_DONE:    sc_done(cr, a);    break;
    }
}

/* ── 后端（shell 库）────────────────────────────────────────────────────── */
/* 图形前端不自己实现任何分区逻辑 —— 全部转给 scripts/live/installer-lib.sh。
 * ★ 一套后端两个前端，命令行安装器用的是同一份代码。 */
static const char *g_lib = "/usr/share/gaokun3/installer-lib.sh";

static int backend(const char *call, void (*cb)(const char *, void *), void *ud)
{
    char cmd[1024];
    snprintf(cmd, sizeof cmd, ". '%s' && %s", g_lib, call);
    FILE *f = popen(cmd, "r");
    if (!f) return -1;
    char line[1024];
    while (fgets(line, sizeof line, f)) {
        size_t n = strlen(line);
        while (n && (line[n-1] == '\n' || line[n-1] == '\r')) line[--n] = 0;
        if (cb) cb(line, ud);
    }
    return pclose(f);
}

/* 从 "k=v k=v" 里取一个值 */
static bool kv(const char *line, const char *key, char *out, size_t n)
{
    char pat[64]; snprintf(pat, sizeof pat, "%s=", key);
    const char *p = line;
    size_t kl = strlen(pat);
    while ((p = strstr(p, pat))) {
        if (p == line || p[-1] == ' ') {
            p += kl;
            const char *e = strchr(p, ' ');
            size_t len = e ? (size_t)(e - p) : strlen(p);
            if (len >= n) len = n - 1;
            memcpy(out, p, len); out[len] = 0;
            return true;
        }
        p += kl;
    }
    return false;
}
static long kvl(const char *line, const char *key)
{
    char b[64];
    return kv(line, key, b, sizeof b) ? atol(b) : 0;
}

static void on_probe(const char *line, void *ud)
{
    App *a = ud; char b[64];
    if (!strncmp(line, "DISK ", 5)) {
        if (a->ndisks >= MAXD) return;
        Disk *d = &a->disks[a->ndisks];
        kv(line, "path", d->path, sizeof d->path);
        kv(line, "model", d->model, sizeof d->model);
        d->size_mib = kvl(line, "size_mib");
        d->removable = (int)kvl(line, "removable");
        /* ⚠️ 不把可移动介质列成安装目标：那多半就是正在跑这个安装器的 U 盘 */
        if (!d->removable) a->ndisks++;
    } else if (!strncmp(line, "PART ", 5)) {
        if (a->nparts >= MAXP) return;
        Part *p = &a->parts[a->nparts++];
        kv(line, "path", p->path, sizeof p->path);
        kv(line, "name", p->name, sizeof p->name);
        kv(line, "fs",   p->fs,   sizeof p->fs);
        kv(line, "os",   p->os,   sizeof p->os);
        p->start = kvl(line, "start"); p->end = kvl(line, "end");
        p->size_mib = kvl(line, "size_mib");
        if (kv(line, "os", b, sizeof b) && !strcmp(b, "esp") && !a->esp[0])
            snprintf(a->esp, sizeof a->esp, "%s", p->path);
    } else if (!strncmp(line, "FREE ", 5)) {
        if (a->nfrees >= MAXF) return;
        FreeRgn *f = &a->frees[a->nfrees++];
        kv(line, "disk", f->disk, sizeof f->disk);
        f->start = kvl(line, "start"); f->end = kvl(line, "end");
        f->size_mib = kvl(line, "size_mib");
    }
}

static void on_plan(const char *line, void *ud)
{
    App *a = ud;
    if (!strncmp(line, "PLANSUM ", 8)) a->plan_userdata_mib = kvl(line, "userdata_mib");
    else if (!strncmp(line, "PLANERR ", 8)) {
        char m[64]; kv(line, "msg", m, sizeof m);
        if (!strcmp(m, "not-enough-space"))
            snprintf(a->plan_err, sizeof a->plan_err, S_ERR_NOSPACE,
                     kvl(line, "avail_mib"), kvl(line, "need_mib"));
        else
            snprintf(a->plan_err, sizeof a->plan_err, S_ERR_PLAN, m);
    }
}

/* 选中的空闲区（最大的那个） */
static int best_free(App *a)
{
    int best = -1; long bs = 0;
    for (int i = 0; i < a->nfrees; i++)
        if (a->frees[i].size_mib > bs) { bs = a->frees[i].size_mib; best = i; }
    return best;
}

static void recompute_plan(App *a)
{
    a->plan_err[0] = 0; a->plan_userdata_mib = 0;
    if (a->seldisk < 0) return;
    char call[512];
    if (a->mode_wipe) {
        snprintf(call, sizeof call, "gk3_plan --disk %s --mode wipe --rescue %s",
                 a->disks[a->seldisk].path, a->want_rescue ? "yes" : "no");
    } else {
        int fi = best_free(a);
        if (fi < 0) { snprintf(a->plan_err, sizeof a->plan_err, S_ERR_NOFREE); return; }
        a->selfree = fi;
        snprintf(call, sizeof call,
                 "gk3_plan --disk %s --mode alongside --rescue %s --region-start %ld --region-end %ld --esp %s",
                 a->disks[a->seldisk].path, a->want_rescue ? "yes" : "no",
                 a->frees[fi].start, a->frees[fi].end, a->esp);
    }
    backend(call, on_plan, a);
}

/* ── 事件 ──────────────────────────────────────────────────────────────── */
/* 返回 true 表示需要重绘 */
static bool on_tap(App *a, int id)
{
    switch (id) {
        case ID_START:  a->screen = SC_DISK; return true;
        case ID_QUIT:   exit(0);
        case ID_BACK:
            if (a->screen > SC_WELCOME && a->screen < SC_RUN) a->screen--;
            return true;
        case ID_NEXT:
            if (a->screen == SC_DISK && a->seldisk >= 0) { a->screen = SC_MODE; recompute_plan(a); }
            else if (a->screen == SC_MODE) { a->screen = SC_OPTS; recompute_plan(a); }
            else if (a->screen == SC_OPTS && !a->plan_err[0]) a->screen = SC_CONFIRM;
            return true;
        case ID_MODE_WIPE:  a->mode_wipe = true;  recompute_plan(a); return true;
        case ID_MODE_ALONG: a->mode_wipe = false; recompute_plan(a); return true;
        case ID_RESCUE_TOGGLE: a->want_rescue = !a->want_rescue; recompute_plan(a); return true;
        case ID_REBOOT: system("reboot"); return false;
        default:
            if (id >= ID_DISK0 && id < ID_DISK0 + MAXD) { a->seldisk = id - ID_DISK0; return true; }
            return false;
    }
}

/* ── 离线渲染：把每一屏写成 PNG ─────────────────────────────────────────── */
/* ★ 这不是"顺手加的调试功能"，是这个安装器能被开发出来的前提：
 *   目标机器同时是作者的日用平板，经常拿不到。没有它，改一行文案都要等上机。 */
static void render_png(App *a, const char *dir, const char *name)
{
    cairo_surface_t *s = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, (int)UI_W, (int)UI_H);
    cairo_t *cr = cairo_create(s);
    draw(cr, a);
    char path[512]; snprintf(path, sizeof path, "%s/%s.png", dir, name);
    cairo_surface_write_to_png(s, path);
    cairo_destroy(cr); cairo_surface_destroy(s);
    printf("  写出 %s\n", path);
}

static void fake_data(App *a)
{
    /* 一台装着 Windows、还留了 60 GiB 空闲的机器 —— 双系统那条路才有东西可看 */
    a->ndisks = 1;
    snprintf(a->disks[0].path, sizeof a->disks[0].path, "/dev/nvme0n1");
    snprintf(a->disks[0].model, sizeof a->disks[0].model, "SAMSUNG MZVL2512HCJQ");
    a->disks[0].size_mib = 488386;
    a->nparts = 4;
    struct { const char *p, *n, *fs, *os; long mib; } P[] = {
        {"/dev/nvme0n1p1", "esp",      "vfat", "esp"},
        {"/dev/nvme0n1p2", "",         "",     "msr"},
        {"/dev/nvme0n1p3", "Windows",  "ntfs", "windows"},
        {"/dev/nvme0n1p4", "Recovery", "ntfs", "winre"},
    };
    long mibs[4] = {300, 16, 400000, 1000};
    for (int i = 0; i < 4; i++) {
        snprintf(a->parts[i].path, sizeof a->parts[i].path, "%s", P[i].p);
        snprintf(a->parts[i].name, sizeof a->parts[i].name, "%s", P[i].n);
        snprintf(a->parts[i].fs,   sizeof a->parts[i].fs,   "%s", P[i].fs);
        snprintf(a->parts[i].os,   sizeof a->parts[i].os,   "%s", P[i].os);
        a->parts[i].size_mib = mibs[i];
    }
    snprintf(a->esp, sizeof a->esp, "/dev/nvme0n1p1");
    a->nfrees = 1;
    snprintf(a->frees[0].disk, sizeof a->frees[0].disk, "/dev/nvme0n1");
    a->frees[0].start = 823296000; a->frees[0].end = 823296000 + 60L * 1024 * 2048 - 1;
    a->frees[0].size_mib = 60 * 1024;
    a->plan_userdata_mib = 463162;
}

static int run_png(const char *dir)
{
    App a = {0}; a.seldisk = -1; a.selfree = -1; a.mode_wipe = true; a.want_rescue = true;
    fake_data(&a);
    printf("离线渲染每一屏到 %s\n", dir);
    a.screen = SC_WELCOME; render_png(&a, dir, "1-welcome");
    a.screen = SC_DISK;                       render_png(&a, dir, "2-disk-none");
    a.seldisk = 0;                            render_png(&a, dir, "3-disk-selected");
    a.screen = SC_MODE;                       render_png(&a, dir, "4-mode-wipe");
    a.mode_wipe = false;                      render_png(&a, dir, "5-mode-alongside");
    a.screen = SC_OPTS;                       render_png(&a, dir, "6-opts-rescue-on");
    a.want_rescue = false;                    render_png(&a, dir, "7-opts-rescue-off");
    a.want_rescue = true;
    a.screen = SC_CONFIRM; a.mode_wipe = true; render_png(&a, dir, "8-confirm-wipe");
    a.hold = 0.55;                             render_png(&a, dir, "9-confirm-holding");
    a.hold = 0; a.mode_wipe = false; a.selfree = 0; render_png(&a, dir, "10-confirm-alongside");
    a.screen = SC_RUN; a.pct = 42;
    snprintf(a.step, sizeof a.step, "正在写入 super.img（12 GiB）");
    a.nlog = 3;
    snprintf(a.logtail[0], sizeof a.logtail[0], "创建分区 misc metadata boot_a boot_b super userdata");
    snprintf(a.logtail[1], sizeof a.logtail[1], "格式化 /dev/nvme0n1p10 为 ext4");
    snprintf(a.logtail[2], sizeof a.logtail[2], "展开 sparse 镜像 super.img …");
    render_png(&a, dir, "11-running");
    a.screen = SC_DONE;                        render_png(&a, dir, "12-done");
    a.failed = true; snprintf(a.step, sizeof a.step, "写 super 分区失败：设备上没有空间");
    render_png(&a, dir, "13-failed");
    /* 一块空盘：双系统那条路应当被禁用并说明原因 */
    App b = {0}; b.seldisk = 0; b.selfree = -1; b.mode_wipe = true; b.want_rescue = true;
    b.ndisks = 1; snprintf(b.disks[0].path, sizeof b.disks[0].path, "/dev/nvme0n1");
    snprintf(b.disks[0].model, sizeof b.disks[0].model, "空盘");
    b.disks[0].size_mib = 488386; b.screen = SC_MODE;
    render_png(&b, dir, "14-mode-alongside-unavailable");
    return 0;
}

#ifdef GK3_DRM
#include "drm_backend.inc"
#endif

int main(int argc, char **argv)
{
    const char *png = NULL;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--png-dir") && i + 1 < argc) png = argv[++i];
        else if (!strcmp(argv[i], "--lib") && i + 1 < argc) g_lib = argv[++i];
        else if (!strcmp(argv[i], "--help")) {
            printf("用法: %s [--png-dir 目录] [--lib installer-lib.sh]\n", argv[0]);
            printf("  --png-dir  离线把每一屏渲染成 PNG（不需要目标硬件）\n");
            return 0;
        }
    }
    if (png) return run_png(png);
#ifdef GK3_DRM
    return run_drm();
#else
    fprintf(stderr, "这个构建没有 DRM 后端。用 --png-dir 做离线渲染，或者用 -DGK3_DRM 重新编译。\n");
    return 2;
#endif
}
