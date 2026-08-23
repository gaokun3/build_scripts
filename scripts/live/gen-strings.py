#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""从 live/installer/strings.zh.txt 生成 live/installer/gk3-strings.h。

⚠️★ 输出【不能】叫 strings.h —— 那是 POSIX 标准头的名字。一旦它没被
   拷到构建目录，#include "strings.h" 不会报"文件不存在"，而是静默
   包含 /usr/include/strings.h，然后吐一堆"宏未声明"。
   实测踩过：chroot 里编译时就是这么失败的，错误信息完全指不到根因。

改文案的流程：编辑 strings.zh.txt → 跑这个脚本 → 重编。
★ 之所以要有这一步，是因为文案散在 C 代码里时，改一句话就要动源码，
  而非程序员改不了、程序员也懒得改。抽出来之后文案是一份独立的、
  可以整体重写的文件。

⚠️ 脚本会核对占位符：%s/%d/%ld 的【个数与顺序】必须和代码期望的一致。
   多一个少一个都会让 printf 读到垃圾内存 —— 那是会崩的，不是显示错。
"""
import io, os, re, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE)) if os.path.basename(HERE) == "live" else os.path.dirname(HERE)
SRC  = os.path.join(ROOT, "live", "installer", "strings.zh.txt")
DST  = os.path.join(ROOT, "live", "installer", "gk3-strings.h")

# 代码期望的占位符。改代码时这张表要跟着改 —— 它是唯一的真相源。
EXPECT = {
    "MODE.ALONG.INFO":    ["%s", "%s"],
    "OPTS.DATA":          ["%s"],
    "CONFIRM.ALONG.BODY": ["%s", "%s"],
    "CONFIRM.RESCUE":     ["%s"],
    "CONFIRM.HOLD.BUSY":  ["%d", "%%"],
    "DONE.BODY":          ["%s"],
    "ERR.NOSPACE":        ["%ld", "%ld"],
    "ERR.PLAN":           ["%s"],
    "ERR.EXIT":           ["%d"],
}

def main():
    txt = io.open(SRC, encoding="utf-8").read()
    items, bad = [], []
    for line in txt.split(chr(10)):
        if not line.strip() or line.startswith("#"):
            continue
        if "=" not in line:
            bad.append("不像条目也不像注释: " + line); continue
        k, v = line.split("=", 1)
        k, v = k.strip(), v.strip()
        if not re.match(r"^[A-Z][A-Z0-9._]*$", k):
            bad.append("ID 不合法: " + k); continue
        got = re.findall(r"%(?:ld|[sd%])", v)
        want = EXPECT.get(k, [])
        if got != want:
            bad.append("占位符不符 " + k + ": 文案是 " + str(got) + "，代码要 " + str(want))
        items.append((k, v))
    if bad:
        for b in bad: sys.stderr.write("!! " + b + chr(10))
        sys.exit(1)

    out = []
    out.append("/* 自动生成，请勿手改 —— 改 strings.zh.txt 后跑 scripts/live/gen-strings.py */")
    out.append("#ifndef GK3_STRINGS_H")
    out.append("#define GK3_STRINGS_H")
    out.append("")
    for k, v in items:
        name = "S_" + k.replace(".", "_")
        out.append("#define " + name.ljust(26) + chr(34) + v + chr(34))
    out.append("")
    out.append("#endif")
    io.open(DST, "w", encoding="utf-8", newline=chr(10)).write(chr(10).join(out) + chr(10))
    sys.stdout.write("生成 " + DST + "：" + str(len(items)) + " 条" + chr(10))

main()
