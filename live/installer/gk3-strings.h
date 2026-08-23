/* 自动生成，请勿手改 —— 改 strings.zh.txt 后跑 scripts/live/gen-strings.py */
#ifndef GK3_STRINGS_H
#define GK3_STRINGS_H

enum {
    STR_WELCOME_TITLE,
    STR_WELCOME_SUB,
    STR_WELCOME_BODY,
    STR_WELCOME_NOTE,
    STR_BTN_START,
    STR_BTN_QUIT,
    STR_BTN_BACK,
    STR_BTN_NEXT,
    STR_BTN_REBOOT,
    STR_DISK_TITLE,
    STR_DISK_SUB,
    STR_DISK_NONE,
    STR_MODE_TITLE,
    STR_MODE_SUB,
    STR_MODE_WIPE_TITLE,
    STR_MODE_WIPE_WARN,
    STR_MODE_WIPE_BODY,
    STR_MODE_ALONG_TITLE,
    STR_MODE_ALONG_OK,
    STR_MODE_ALONG_INFO,
    STR_MODE_WHY_NOESP,
    STR_MODE_WHY_NOROOM,
    STR_OPTS_TITLE,
    STR_OPTS_SUB,
    STR_OPTS_RESCUE_TITLE,
    STR_OPTS_RESCUE_BODY,
    STR_OPTS_DATA,
    STR_OPTS_LATER,
    STR_CONFIRM_TITLE,
    STR_CONFIRM_SUB,
    STR_CONFIRM_WIPE_HEAD,
    STR_CONFIRM_NOPARTS,
    STR_CONFIRM_MORE,
    STR_CONFIRM_ALONG_HEAD,
    STR_CONFIRM_ALONG_BODY,
    STR_CONFIRM_RESCUE,
    STR_WORD_INSTALL,
    STR_WORD_NOINSTALL,
    STR_CONFIRM_HOLD_IDLE,
    STR_CONFIRM_HOLD_BUSY,
    STR_RUN_TITLE,
    STR_RUN_SUB,
    STR_DONE_TITLE,
    STR_DONE_BODY,
    STR_DONE_RESCUE,
    STR_FAIL_TITLE,
    STR_FAIL_SUB,
    STR_ERR_NOSPACE,
    STR_ERR_PLAN,
    STR_ERR_NOFREE,
    STR_ERR_BACKEND,
    STR_ERR_EXIT,
    STR_STEP_PREP,
    STR_STEP_DONE,
    STR_MODE_SHRINK_TITLE,
    STR_MODE_SHRINK_BODY,
    STR_SHRINK_TITLE,
    STR_SHRINK_SUB,
    STR_SHRINK_NONE,
    STR_SHRINK_ROW,
    STR_SHRINK_WHY_DIRTY,
    STR_SHRINK_WHY_FS,
    STR_SHRINK_SIZE_HINT,
    STR_SHRINK_FREED,
    STR_SHRINK_GO,
    STR_SHRINK_WARN,
    STR_SOURCE_TITLE,
    STR_SOURCE_SUB,
    STR_SOURCE_USB_TITLE,
    STR_SOURCE_USB_BODY,
    STR_SOURCE_USB_MISSING,
    STR_SOURCE_NET_TITLE,
    STR_SOURCE_NET_BODY,
    STR_NET_TITLE,
    STR_NET_SUB,
    STR_NET_SCANNING,
    STR_NET_NONE,
    STR_NET_PASSWORD,
    STR_NET_CONNECT,
    STR_NET_RESCAN,
    STR_NET_CONNECTED,
    STR_NET_FAILED,
    STR_VARIANT_TITLE,
    STR_VARIANT_SUB,
    STR_VARIANT_FAILED,
    STR_VARIANT_SIZE,
    STR_ADV_TITLE,
    STR_ADV_SUB,
    STR_ADV_RESET,
    STR_ADV_TOTAL,
    STR_ADV_OVERFLOW,
    STR_SHELL_OPEN,
    STR_SHELL_HINT,
    STR__COUNT
};

/* 内置文案（中文）。gk3_strings_load() 可以整表替换。 */
static const char *gk3_str_default[STR__COUNT] = {
    "让我们在这台电脑上安装 Android",
    "HUAWEI MateBook E Go · Snapdragon 8cx Gen 3 (sc8280xp)",
    "我们将把 Android 安装到你的内置固态硬盘上。\\n在接下来的步骤中，你可以选择清除整个磁盘，或者保留现有系统，仅使用可用空间。",
    "你的设备使用 UEFI 引导，而不是 fastboot。安装完成后，systemd-boot 将为你启动系统，\\n内核和 ramdisk 会以普通文件的形式放置在 EFI 分区上。",
    "开始使用",
    "退出到终端",
    "返回",
    "下一步",
    "立即重新启动",
    "你想将 Android 安装在哪里？",
    "选择一个磁盘以继续。可移动介质（例如你正在使用的安装 U 盘）不会显示在这里。",
    "我们找不到可以安装的磁盘。请检查你的内置硬盘是否已被识别（可以在终端中运行 lsblk）。",
    "你希望如何安装？",
    "此选择将决定磁盘上现有内容的去留。请在继续之前仔细确认。",
    "清除整个磁盘",
    "此磁盘上的所有内容都将被删除，包括其他操作系统。",
    "这将为你提供最干净的分区布局，Android 将获得全部空间。\\n如果这台电脑只用来运行 Android，我们建议选择此项。",
    "保留现有系统",
    "我们只会使用可用空间，不会对你现有的分区进行任何更改。",
    "可用空间：%s\\n我们将重用你现有的 EFI 分区：%s",
    "此磁盘上没有 EFI 系统分区 —— 这意味着它上面没有可以保留的 UEFI 系统",
    "可用空间不足 20.2 GiB。请先返回现有系统压缩分区以腾出一些空间（在 Windows 中，可以使用“磁盘管理”来完成此操作）",
    "快速设置",
    "我们已经为你选择了推荐的设置。如果不确定，直接点击“下一步”即可。",
    "同时安装救援系统",
    "这将占用 1 GiB 空间，其中包含一套在内存中运行的 Linux（带有 ssh 和分区工具）。\\n当 Android 无法启动时，它是远程访问这台设备的唯一方式。我们强烈建议保留此项。",
    "/data 将获得所有剩余空间：大约 %s",
    "（我们将在下一步为你计算）",
    "在开始之前，请确认一下",
    "这是最后一步。在此之后，我们将开始对磁盘进行更改。",
    "以下分区将【全部被删除】：",
    "（此磁盘上目前没有任何分区）",
    "…以及其余分区",
    "你现有的分区【不会受到任何影响】。我们只会使用可用空间。",
    "我们将创建 %s 的 Android 分区，并在你的 EFI 分区 %s 中添加两个启动项。",
    "救援系统：%s",
    "安装",
    "不安装",
    "按住 2 秒以开始安装",
    "请继续按住…… %d%%",
    "我们正在为你安装 Android",
    "这可能需要几分钟时间。请不要关闭电源。",
    "一切就绪！",
    "请移除安装介质。重新启动后，你将进入 Android。%s",
    "\\n\\n我们还为你安装了救援系统：你可以在开机菜单中选择它。当 Android 无法启动时，它是远程访问这台设备的唯一方式。",
    "哎呀，出错了",
    "你的磁盘可能处于未完成的中间状态。请不要立即重启回原系统 —— 我们建议先保存或拍下下方的日志。",
    "空间不足：当前可用 %ld MiB，但我们至少需要 %ld MiB",
    "我们无法计算分区方案：%s",
    "此磁盘上没有可用空间",
    "我们无法启动安装后端",
    "安装未能完成（退出码 %d）",
    "正在准备一些事情……",
    "即将完成……",
    "缩小现有分区腾出空间",
    "我们可以帮你缩小一个现有分区，把腾出来的空间给 Android。你的文件不会被删除。",
    "要缩小哪个分区？",
    "我们只会调整分区的大小，不会删除里面的任何文件。开始之前，我们会先备份分区表。",
    "这块磁盘上没有可以缩小的分区。只有 NTFS 和 ext 文件系统支持在不丢数据的前提下缩小。",
    "%s · 当前 %s · 最小可缩到 %s",
    "这个 NTFS 分区没有被干净地卸载。请回到 Windows，关闭\"快速启动\"并正常关机，然后再试一次。",
    "这种文件系统不支持在不丢数据的前提下缩小",
    "缩小后的大小（GiB）",
    "这会腾出大约 %s 给 Android",
    "开始缩小",
    "这是整个安装过程中唯一会改动你现有数据的一步。请确认重要文件已经备份。",
    "从哪里获取 Android？",
    "你可以使用安装 U 盘里自带的镜像，也可以从网络下载最新版本。",
    "使用 U 盘里的镜像",
    "速度最快，不需要联网。这是我们推荐的方式。",
    "这个 U 盘里没有安装镜像",
    "从网络下载",
    "可以拿到最新版本，也可以选择不同的构建。这台设备的无线速度大约每秒 1–2 MB，1 GB 左右的镜像需要十几分钟。",
    "连接到 Wi-Fi",
    "这台设备只有无线网卡。选择一个网络并输入密码。",
    "正在搜索附近的网络……",
    "没有搜索到任何网络。请确认无线开关已打开，然后重试。",
    "网络密码",
    "连接",
    "重新搜索",
    "已连接到 %s（%s）",
    "连接失败。请检查密码是否正确，或者信号是否太弱。",
    "选择一个版本",
    "这些版本的区别在构建时就已经决定，安装之后无法互相切换。",
    "我们无法获取版本列表。请检查网络连接。",
    "下载大小约 %s",
    "分区大小",
    "这些是我们将要创建的分区。除非你清楚自己在做什么，否则建议保持默认值。",
    "恢复默认",
    "已分配 %s，共 %s",
    "分配的空间超过了可用空间",
    "打开终端",
    "我们已经在第二个虚拟终端上为你准备好了 shell。按 Ctrl+Alt+F2 切换过去，按 Ctrl+Alt+F1 回到这里。里面有 sgdisk、parted、resize2fs 等工具。",
};

/* ID 名字，给加载器按名字对号入座用 */
static const char *gk3_str_id[STR__COUNT] = {
    "WELCOME.TITLE",
    "WELCOME.SUB",
    "WELCOME.BODY",
    "WELCOME.NOTE",
    "BTN.START",
    "BTN.QUIT",
    "BTN.BACK",
    "BTN.NEXT",
    "BTN.REBOOT",
    "DISK.TITLE",
    "DISK.SUB",
    "DISK.NONE",
    "MODE.TITLE",
    "MODE.SUB",
    "MODE.WIPE.TITLE",
    "MODE.WIPE.WARN",
    "MODE.WIPE.BODY",
    "MODE.ALONG.TITLE",
    "MODE.ALONG.OK",
    "MODE.ALONG.INFO",
    "MODE.WHY.NOESP",
    "MODE.WHY.NOROOM",
    "OPTS.TITLE",
    "OPTS.SUB",
    "OPTS.RESCUE.TITLE",
    "OPTS.RESCUE.BODY",
    "OPTS.DATA",
    "OPTS.LATER",
    "CONFIRM.TITLE",
    "CONFIRM.SUB",
    "CONFIRM.WIPE.HEAD",
    "CONFIRM.NOPARTS",
    "CONFIRM.MORE",
    "CONFIRM.ALONG.HEAD",
    "CONFIRM.ALONG.BODY",
    "CONFIRM.RESCUE",
    "WORD.INSTALL",
    "WORD.NOINSTALL",
    "CONFIRM.HOLD.IDLE",
    "CONFIRM.HOLD.BUSY",
    "RUN.TITLE",
    "RUN.SUB",
    "DONE.TITLE",
    "DONE.BODY",
    "DONE.RESCUE",
    "FAIL.TITLE",
    "FAIL.SUB",
    "ERR.NOSPACE",
    "ERR.PLAN",
    "ERR.NOFREE",
    "ERR.BACKEND",
    "ERR.EXIT",
    "STEP.PREP",
    "STEP.DONE",
    "MODE.SHRINK.TITLE",
    "MODE.SHRINK.BODY",
    "SHRINK.TITLE",
    "SHRINK.SUB",
    "SHRINK.NONE",
    "SHRINK.ROW",
    "SHRINK.WHY.DIRTY",
    "SHRINK.WHY.FS",
    "SHRINK.SIZE.HINT",
    "SHRINK.FREED",
    "SHRINK.GO",
    "SHRINK.WARN",
    "SOURCE.TITLE",
    "SOURCE.SUB",
    "SOURCE.USB.TITLE",
    "SOURCE.USB.BODY",
    "SOURCE.USB.MISSING",
    "SOURCE.NET.TITLE",
    "SOURCE.NET.BODY",
    "NET.TITLE",
    "NET.SUB",
    "NET.SCANNING",
    "NET.NONE",
    "NET.PASSWORD",
    "NET.CONNECT",
    "NET.RESCAN",
    "NET.CONNECTED",
    "NET.FAILED",
    "VARIANT.TITLE",
    "VARIANT.SUB",
    "VARIANT.FAILED",
    "VARIANT.SIZE",
    "ADV.TITLE",
    "ADV.SUB",
    "ADV.RESET",
    "ADV.TOTAL",
    "ADV.OVERFLOW",
    "SHELL.OPEN",
    "SHELL.HINT",
};

static const char *gk3_str_over[STR__COUNT];   /* 加载进来的覆盖，NULL=用默认 */

static const char *gk3_s(int id)
{
    if (id < 0 || id >= STR__COUNT) return "?";
    return gk3_str_over[id] ? gk3_str_over[id] : gk3_str_default[id];
}

#define S_WELCOME_TITLE            gk3_s(STR_WELCOME_TITLE)
#define S_WELCOME_SUB              gk3_s(STR_WELCOME_SUB)
#define S_WELCOME_BODY             gk3_s(STR_WELCOME_BODY)
#define S_WELCOME_NOTE             gk3_s(STR_WELCOME_NOTE)
#define S_BTN_START                gk3_s(STR_BTN_START)
#define S_BTN_QUIT                 gk3_s(STR_BTN_QUIT)
#define S_BTN_BACK                 gk3_s(STR_BTN_BACK)
#define S_BTN_NEXT                 gk3_s(STR_BTN_NEXT)
#define S_BTN_REBOOT               gk3_s(STR_BTN_REBOOT)
#define S_DISK_TITLE               gk3_s(STR_DISK_TITLE)
#define S_DISK_SUB                 gk3_s(STR_DISK_SUB)
#define S_DISK_NONE                gk3_s(STR_DISK_NONE)
#define S_MODE_TITLE               gk3_s(STR_MODE_TITLE)
#define S_MODE_SUB                 gk3_s(STR_MODE_SUB)
#define S_MODE_WIPE_TITLE          gk3_s(STR_MODE_WIPE_TITLE)
#define S_MODE_WIPE_WARN           gk3_s(STR_MODE_WIPE_WARN)
#define S_MODE_WIPE_BODY           gk3_s(STR_MODE_WIPE_BODY)
#define S_MODE_ALONG_TITLE         gk3_s(STR_MODE_ALONG_TITLE)
#define S_MODE_ALONG_OK            gk3_s(STR_MODE_ALONG_OK)
#define S_MODE_ALONG_INFO          gk3_s(STR_MODE_ALONG_INFO)
#define S_MODE_WHY_NOESP           gk3_s(STR_MODE_WHY_NOESP)
#define S_MODE_WHY_NOROOM          gk3_s(STR_MODE_WHY_NOROOM)
#define S_OPTS_TITLE               gk3_s(STR_OPTS_TITLE)
#define S_OPTS_SUB                 gk3_s(STR_OPTS_SUB)
#define S_OPTS_RESCUE_TITLE        gk3_s(STR_OPTS_RESCUE_TITLE)
#define S_OPTS_RESCUE_BODY         gk3_s(STR_OPTS_RESCUE_BODY)
#define S_OPTS_DATA                gk3_s(STR_OPTS_DATA)
#define S_OPTS_LATER               gk3_s(STR_OPTS_LATER)
#define S_CONFIRM_TITLE            gk3_s(STR_CONFIRM_TITLE)
#define S_CONFIRM_SUB              gk3_s(STR_CONFIRM_SUB)
#define S_CONFIRM_WIPE_HEAD        gk3_s(STR_CONFIRM_WIPE_HEAD)
#define S_CONFIRM_NOPARTS          gk3_s(STR_CONFIRM_NOPARTS)
#define S_CONFIRM_MORE             gk3_s(STR_CONFIRM_MORE)
#define S_CONFIRM_ALONG_HEAD       gk3_s(STR_CONFIRM_ALONG_HEAD)
#define S_CONFIRM_ALONG_BODY       gk3_s(STR_CONFIRM_ALONG_BODY)
#define S_CONFIRM_RESCUE           gk3_s(STR_CONFIRM_RESCUE)
#define S_WORD_INSTALL             gk3_s(STR_WORD_INSTALL)
#define S_WORD_NOINSTALL           gk3_s(STR_WORD_NOINSTALL)
#define S_CONFIRM_HOLD_IDLE        gk3_s(STR_CONFIRM_HOLD_IDLE)
#define S_CONFIRM_HOLD_BUSY        gk3_s(STR_CONFIRM_HOLD_BUSY)
#define S_RUN_TITLE                gk3_s(STR_RUN_TITLE)
#define S_RUN_SUB                  gk3_s(STR_RUN_SUB)
#define S_DONE_TITLE               gk3_s(STR_DONE_TITLE)
#define S_DONE_BODY                gk3_s(STR_DONE_BODY)
#define S_DONE_RESCUE              gk3_s(STR_DONE_RESCUE)
#define S_FAIL_TITLE               gk3_s(STR_FAIL_TITLE)
#define S_FAIL_SUB                 gk3_s(STR_FAIL_SUB)
#define S_ERR_NOSPACE              gk3_s(STR_ERR_NOSPACE)
#define S_ERR_PLAN                 gk3_s(STR_ERR_PLAN)
#define S_ERR_NOFREE               gk3_s(STR_ERR_NOFREE)
#define S_ERR_BACKEND              gk3_s(STR_ERR_BACKEND)
#define S_ERR_EXIT                 gk3_s(STR_ERR_EXIT)
#define S_STEP_PREP                gk3_s(STR_STEP_PREP)
#define S_STEP_DONE                gk3_s(STR_STEP_DONE)
#define S_MODE_SHRINK_TITLE        gk3_s(STR_MODE_SHRINK_TITLE)
#define S_MODE_SHRINK_BODY         gk3_s(STR_MODE_SHRINK_BODY)
#define S_SHRINK_TITLE             gk3_s(STR_SHRINK_TITLE)
#define S_SHRINK_SUB               gk3_s(STR_SHRINK_SUB)
#define S_SHRINK_NONE              gk3_s(STR_SHRINK_NONE)
#define S_SHRINK_ROW               gk3_s(STR_SHRINK_ROW)
#define S_SHRINK_WHY_DIRTY         gk3_s(STR_SHRINK_WHY_DIRTY)
#define S_SHRINK_WHY_FS            gk3_s(STR_SHRINK_WHY_FS)
#define S_SHRINK_SIZE_HINT         gk3_s(STR_SHRINK_SIZE_HINT)
#define S_SHRINK_FREED             gk3_s(STR_SHRINK_FREED)
#define S_SHRINK_GO                gk3_s(STR_SHRINK_GO)
#define S_SHRINK_WARN              gk3_s(STR_SHRINK_WARN)
#define S_SOURCE_TITLE             gk3_s(STR_SOURCE_TITLE)
#define S_SOURCE_SUB               gk3_s(STR_SOURCE_SUB)
#define S_SOURCE_USB_TITLE         gk3_s(STR_SOURCE_USB_TITLE)
#define S_SOURCE_USB_BODY          gk3_s(STR_SOURCE_USB_BODY)
#define S_SOURCE_USB_MISSING       gk3_s(STR_SOURCE_USB_MISSING)
#define S_SOURCE_NET_TITLE         gk3_s(STR_SOURCE_NET_TITLE)
#define S_SOURCE_NET_BODY          gk3_s(STR_SOURCE_NET_BODY)
#define S_NET_TITLE                gk3_s(STR_NET_TITLE)
#define S_NET_SUB                  gk3_s(STR_NET_SUB)
#define S_NET_SCANNING             gk3_s(STR_NET_SCANNING)
#define S_NET_NONE                 gk3_s(STR_NET_NONE)
#define S_NET_PASSWORD             gk3_s(STR_NET_PASSWORD)
#define S_NET_CONNECT              gk3_s(STR_NET_CONNECT)
#define S_NET_RESCAN               gk3_s(STR_NET_RESCAN)
#define S_NET_CONNECTED            gk3_s(STR_NET_CONNECTED)
#define S_NET_FAILED               gk3_s(STR_NET_FAILED)
#define S_VARIANT_TITLE            gk3_s(STR_VARIANT_TITLE)
#define S_VARIANT_SUB              gk3_s(STR_VARIANT_SUB)
#define S_VARIANT_FAILED           gk3_s(STR_VARIANT_FAILED)
#define S_VARIANT_SIZE             gk3_s(STR_VARIANT_SIZE)
#define S_ADV_TITLE                gk3_s(STR_ADV_TITLE)
#define S_ADV_SUB                  gk3_s(STR_ADV_SUB)
#define S_ADV_RESET                gk3_s(STR_ADV_RESET)
#define S_ADV_TOTAL                gk3_s(STR_ADV_TOTAL)
#define S_ADV_OVERFLOW             gk3_s(STR_ADV_OVERFLOW)
#define S_SHELL_OPEN               gk3_s(STR_SHELL_OPEN)
#define S_SHELL_HINT               gk3_s(STR_SHELL_HINT)

#endif
