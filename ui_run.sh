#!/usr/bin/env bash
# ui_run.sh — 启动 EDK2 图形化 BIOS UI (OVMF + QEMU) 演示
#
# 用法:
#   ./ui_run.sh              # 默认：BIOS Setup UI (UiApp)
#   ./ui_run.sh demo         # LvglDemoApp 计时器演示
#   ./ui_run.sh nodisk       # 无启动盘，显示 POST 启动画面
#   ./ui_run.sh shot [name]  # 静默运行并截图（用于 CI）
#
# 显示模式 (--display):
#   自动检测 $DISPLAY → gtk/sdl；无 X11 → vnc（端口 5900）
#
# 退出快捷键: Ctrl+Alt+G 释放鼠标，Ctrl+Alt+Q 退出 QEMU

set -euo pipefail

# ── 路径配置 ────────────────────────────────────────────────────────────────
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/Build/OvmfX64/DEBUG_GCC"
OVMF_CODE="$BUILD_DIR/FV/OVMF_CODE.fd"
OVMF_VARS_ORIG="$BUILD_DIR/FV/OVMF_VARS.fd"
OVMF_VARS_WORK="/tmp/ovmf_vars_ui_run.fd"
TESTFS="$SCRIPT_DIR/testfs"
UIAPP="$BUILD_DIR/X64/UiApp.efi"
SERIAL_LOG="/tmp/ovmf-serial.log"
MON_SOCK="/tmp/qemu-ui-mon.sock"

# ── 参数解析 ────────────────────────────────────────────────────────────────
MODE="${1:-setup}"          # setup | demo | nodisk | shot
SHOT_NAME="${2:-shot}"
shift $# 2>/dev/null || true   # 消耗脚本参数，避免传给 QEMU

# ── 前置检查 ────────────────────────────────────────────────────────────────
die() { echo "✗ $*" >&2; exit 1; }

[ -f "$OVMF_CODE" ]   || die "OVMF_CODE.fd 未找到，请先执行:
    source edksetup.sh && build -p OvmfPkg/OvmfPkgX64.dsc -a X64 -t GCC -b DEBUG"
[ -f "$OVMF_VARS_ORIG" ] || die "OVMF_VARS.fd 未找到"
[ -d "$TESTFS" ]       || die "testfs/ 目录不存在"

command -v qemu-system-x86_64 &>/dev/null || die "未找到 qemu-system-x86_64"
command -v ffmpeg &>/dev/null || HAVE_FFMPEG=0 || true
HAVE_FFMPEG="${HAVE_FFMPEG:-1}"

# KVM 加速
KVM_ARGS=""
[ -w /dev/kvm ] && KVM_ARGS="-enable-kvm" && echo "✓ KVM 加速已启用"

# ── 显示后端 ─────────────────────────────────────────────────────────────────
if [ "$MODE" = "shot" ]; then
    DISPLAY_ARGS="-vga std -display none"
    echo "✓ 显示模式: 无界面（截图模式）"
elif [ -n "${DISPLAY:-}" ]; then
    # 检查 gtk 是否真的可用（不启动 QEMU，只看帮助输出）
    if qemu-system-x86_64 -display help 2>&1 | grep -q "^gtk"; then
        DISPLAY_ARGS="-vga std -display gtk,gl=off"
        echo "✓ 显示模式: GTK"
    else
        DISPLAY_ARGS="-vga std -display sdl"
        echo "✓ 显示模式: SDL"
    fi
else
    VNC_PORT=0   # :0 = TCP 5900
    DISPLAY_ARGS="-vga std -display vnc=:${VNC_PORT}"
    echo "✓ 显示模式: VNC（无 X11）"
    echo "  → 用 VNC 客户端连接 $(hostname -I | awk '{print $1}'):5900"
fi

# ── 磁盘与 startup.nsh ───────────────────────────────────────────────────────
DISK_ARGS=""
case "$MODE" in
    setup)
        echo "✓ 模式: BIOS Setup UI (GuiDisplayEngineDxe)"
        [ -f "$UIAPP" ] || die "UiApp.efi 未找到: $UIAPP"
        cp "$UIAPP" "$TESTFS/UiApp.efi"
        cat > "$TESTFS/startup.nsh" << 'NSH'
@echo -off
fs0:
UiApp.efi
NSH
        DISK_ARGS="-drive format=raw,file=fat:rw:$TESTFS"
        ;;
    demo)
        echo "✓ 模式: LvglDemoApp (lvgl 渲染 + CJK 字体)"
        cat > "$TESTFS/startup.nsh" << 'NSH'
@echo -off
fs0:
LvglDemoApp.efi
NSH
        DISK_ARGS="-drive format=raw,file=fat:rw:$TESTFS"
        ;;
    nodisk)
        echo "✓ 模式: 无启动盘 (GuiBootLogoLib POST 画面)"
        DISK_ARGS=""
        ;;
    shot)
        echo "✓ 模式: 静默截图"
        [ -f "$UIAPP" ] && cp "$UIAPP" "$TESTFS/UiApp.efi"
        cat > "$TESTFS/startup.nsh" << 'NSH'
@echo -off
fs0:
UiApp.efi
NSH
        DISK_ARGS="-drive format=raw,file=fat:rw:$TESTFS"
        ;;
    *)
        die "未知模式: $MODE。可用：setup | demo | nodisk | shot"
        ;;
esac

# ── 复制 VARS（每次启动干净 NVRAM）─────────────────────────────────────────
cp "$OVMF_VARS_ORIG" "$OVMF_VARS_WORK"

# ── 启动 QEMU ────────────────────────────────────────────────────────────────
echo ""
echo "━━━ 启动 QEMU ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

MONITOR_ARGS=""
if [ "$MODE" = "shot" ]; then
    rm -f "$MON_SOCK"
    MONITOR_ARGS="-monitor unix:$MON_SOCK,server,nowait"
fi

# shellcheck disable=SC2086
qemu-system-x86_64 \
    -machine q35 $KVM_ARGS \
    -m 512M \
    -drive if=pflash,format=raw,readonly=on,file="$OVMF_CODE" \
    -drive if=pflash,format=raw,file="$OVMF_VARS_WORK" \
    $DISK_ARGS \
    $DISPLAY_ARGS \
    $MONITOR_ARGS \
    -serial file:"$SERIAL_LOG" \
    -name "EDK2 GUI BIOS Demo" &

QEMU_PID=$!

# ── 截图模式 ─────────────────────────────────────────────────────────────────
if [ "$MODE" = "shot" ]; then
    # 等待 monitor socket
    for i in $(seq 1 30); do [ -S "$MON_SOCK" ] && break; sleep 0.5; done

    take_shot() {
        local label="$1" delay="$2"
        sleep "$delay"
        local ppm="/tmp/${SHOT_NAME}_${label}.ppm"
        local png="$SCRIPT_DIR/docs/screenshots/${SHOT_NAME}_${label}.png"
        python3 - "$MON_SOCK" "$ppm" << 'EOF'
import socket, time, sys
s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
s.connect(sys.argv[1])
s.settimeout(3)
try: s.recv(4096)
except: pass
s.sendall(('screendump ' + sys.argv[2] + '\n').encode())
time.sleep(0.4)
try: s.recv(4096)
except: pass
s.close()
EOF
        if [ -f "$ppm" ] && [ -s "$ppm" ]; then
            mkdir -p "$(dirname "$png")"
            if [ "$HAVE_FFMPEG" = "1" ]; then
                ffmpeg -y -loglevel quiet -i "$ppm" "$png" && rm "$ppm"
                echo "  📸 $png"
            else
                mv "$ppm" "${ppm%.ppm}.png"
                echo "  📸 ${ppm%.ppm}.png (PPM→PNG 转换需要 ffmpeg)"
            fi
        else
            echo "  ✗ 截图失败 ($label)"
        fi
    }

    take_shot "t03" 3
    take_shot "t08" 5
    take_shot "t13" 5

    kill "$QEMU_PID" 2>/dev/null
    wait "$QEMU_PID" 2>/dev/null || true
    echo ""
    echo "截图完成。串口日志: $SERIAL_LOG"
    exit 0
fi

# ── 交互模式：等待退出 ────────────────────────────────────────────────────────
echo "QEMU PID: $QEMU_PID"
echo "串口日志: $SERIAL_LOG"
echo ""
echo "提示:"
echo "  Ctrl+Alt+G  — 释放鼠标"
echo "  Ctrl+Alt+Q  — 退出 QEMU（GTK/SDL）"
echo "  在 BIOS Setup 内: Esc 返回 / F10 保存退出"
echo ""

wait "$QEMU_PID" || true
echo ""
echo "QEMU 已退出。"
echo "串口日志: $SERIAL_LOG"

# 恢复 startup.nsh
cat > "$TESTFS/startup.nsh" << 'NSH'
@echo -off
fs0:
LvglDemoApp.efi
reset -s
NSH
