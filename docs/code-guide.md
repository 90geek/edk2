# EDK2 GUI BIOS — 代码说明文档

> 面向需要维护或扩展图形化 BIOS UI 的开发者。  
> 对应分支 `ui`，所有新增代码均在 `MdeModulePkg/` 下。

---

## 目录

1. [目录结构](#目录结构)
2. [LvglLib — lvgl 移植层](#lvgllib--lvgl-移植层)
3. [GuiBootLogoLib — POST 启动画面](#guibootlogolib--post-启动画面)
4. [GuiDisplayEngineDxe — BIOS Setup UI](#guidisplayenginedxe--bios-setup-ui)
5. [GuiBootManagerMenuApp — 启动菜单](#guibootmanagermenuapp--启动菜单)
6. [常见扩展场景](#常见扩展场景)
7. [调试方法](#调试方法)

---

## 目录结构

```
MdeModulePkg/
├── Include/Library/
│   └── LvglLib.h                  ← 公开 API（三个函数）
│
├── Library/
│   ├── LvglLib/                   ← lvgl 移植层（库）
│   │   ├── lvgl/                  ← git submodule: lvgl v9.2.2
│   │   ├── lv_conf.h              ← lvgl 编译配置
│   │   ├── LvglLibInit.c          ← UefiLvglInit/Tick/Deinit
│   │   ├── LvglLib.inf
│   │   ├── fonts/
│   │   │   ├── lv_font_cjk_14.c  ← NotoSansSC 14px（生成）
│   │   │   └── lv_font_cjk_18.c  ← NotoSansSC 18px（生成，默认）
│   │   └── port/
│   │       ├── LvglPort.h
│   │       ├── lv_uefi_disp.c    ← GOP 帧缓冲显示后端
│   │       ├── lv_uefi_indev_kb.c ← 键盘输入适配器
│   │       └── lv_uefi_indev_ptr.c ← 鼠标/触控输入适配器
│   │
│   └── GuiBootLogoLib/            ← BootLogoLib 实现
│       ├── GuiBootLogoLib.c
│       └── GuiBootLogoLib.inf
│
├── Application/
│   ├── LvglDemoApp/               ← P1 验证用 Demo
│   │   ├── LvglDemoApp.c
│   │   └── LvglDemoApp.inf
│   └── GuiBootManagerMenuApp/     ← 启动菜单
│       ├── GuiBootManagerMenuApp.c
│       └── GuiBootManagerMenuApp.inf
│
└── Universal/
    └── GuiDisplayEngineDxe/       ← HII Setup UI 引擎
        ├── GuiDisplayEngine.h     ← 内部头文件（全局对象声明）
        ├── GuiDisplayEngine.c     ← 协议安装 + 表单渲染主循环
        ├── Layout.c               ← 双栏布局初始化
        ├── WidgetFactory.c        ← HII IFR → lvgl 控件映射
        └── GuiDisplayEngineDxe.inf
```

---

## LvglLib — lvgl 移植层

### 公开 API（`MdeModulePkg/Include/Library/LvglLib.h`）

```c
/**
 * 初始化 lvgl 并注册 UEFI 显示/输入设备。
 * 必须在使用任何 lv_xxx() 函数前调用。
 * 每次 Init 对应一次 Deinit；不支持嵌套。
 */
VOID UefiLvglInit (
    IN EFI_HANDLE        ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable
);

/**
 * 驱动 lvgl 一帧：调用 lv_timer_handler() + gBS->Stall(5000µs)。
 * 在事件循环中持续调用，刷新屏幕并处理输入。
 */
VOID UefiLvglTick (VOID);

/**
 * 释放 lvgl 资源，解除注册的显示和输入设备。
 * 调用后 GOP 帧缓冲恢复空白。
 */
VOID UefiLvglDeinit (VOID);
```

### 显示后端（`port/lv_uefi_disp.c`）

```
GOP 帧缓冲
   ↑ lv_display_set_flush_cb()
   │  flush_cb: GOP->Blt(BltBufferToVideo, ...)
   │
lv_display_t
   └─ draw_buf: AllocatePool(W × H × 4 bytes)
```

关键函数：

| 函数 | 说明 |
|------|------|
| `LvglDispInit()` | 定位 GOP，创建 `lv_display_t`，注册 flush 回调 |
| `FlushCb()` | 将 lvgl 脏区域通过 `EfiBltBufferToVideo` 写入帧缓冲 |
| `LvglDispDeinit()` | 释放绘图缓冲，删除 `lv_display_t` |

**分辨率**：自动读取 `GOP->Mode->Info->HorizontalResolution / VerticalResolution`。  
**像素格式**：`LV_COLOR_FORMAT_ARGB8888` ↔ `EFI_GRAPHICS_OUTPUT_BLT_PIXEL`（32-bit BGRA，字节序一致）。

### 键盘适配器（`port/lv_uefi_indev_kb.c`）

```
SimpleTextInputEx
   → ReadKeyStrokeEx()
   → 映射 EFI ScanCode → LV_KEY_xxx
   → lv_indev_t (LV_INDEV_TYPE_KEYPAD)
```

按键映射表：

| EFI ScanCode | lvgl 键 |
|------|---------|
| `SCAN_UP` | `LV_KEY_UP` |
| `SCAN_DOWN` | `LV_KEY_DOWN` |
| `SCAN_LEFT` | `LV_KEY_LEFT` |
| `SCAN_RIGHT` | `LV_KEY_RIGHT` |
| `SCAN_ESC` | `LV_KEY_ESC` |
| `CHAR_CARRIAGE_RETURN` | `LV_KEY_ENTER` |
| `CHAR_BACKSPACE` | `LV_KEY_BACKSPACE` |

### lv_conf.h 关键配置

```c
#define LV_COLOR_DEPTH        32    // 匹配 GOP ARGB8888
#define LV_USE_OS             LV_OS_NONE   // 无操作系统
#define LV_FONT_DEFAULT       &lv_font_cjk_18  // 18px CJK 字体
#define LV_USE_STDLIB_MALLOC  LV_STDLIB_BUILTIN
// 内存池：由 lv_conf.h 的 LV_MEM_SIZE 控制（默认 256KB）

// 声明自定义字体（在其他模块中用 extern 引用）
#define LV_FONT_CUSTOM_DECLARE \
    extern const lv_font_t lv_font_cjk_14; \
    extern const lv_font_t lv_font_cjk_18;
```

### 重新生成 CJK 字库

```bash
# 需要 node.js + lv_font_conv
npm install -g lv_font_conv

# 14px：ASCII + CJK 基础区 4E00-55FF
lv_font_conv --bpp 4 --size 14 --no-compress \
  --font NotoSansSC-Regular.ttf \
  -r 0x20-0x7E -r 0x4E00-0x55FF \
  --format lvgl -o MdeModulePkg/Library/LvglLib/fonts/lv_font_cjk_14.c \
  --force-fast-kern-format

# 18px（同上，改 --size 18 和输出文件名）
```

---

## GuiBootLogoLib — POST 启动画面

**文件**：`MdeModulePkg/Library/GuiBootLogoLib/GuiBootLogoLib.c`  
**接口**：实现 EDK2 标准 `BootLogoLib` 库类（`MdeModulePkg/Include/Library/BootLogoLib.h`）

### 函数说明

```c
/**
 * 被 PlatformBootManagerLib 在 BDS 阶段调用。
 * 初始化 lvgl，渲染深蓝启动画面。
 * 幂等：重复调用无副作用。
 */
EFI_STATUS EFIAPI BootLogoEnableLogo (VOID);

/**
 * 更新进度条值和标题文字。
 * Progress: 0-100（超出范围返回 EFI_INVALID_PARAMETER）。
 * 首次调用时若 lvgl 未初始化，自动调用 EnsureInited()。
 */
EFI_STATUS EFIAPI BootLogoUpdateProgress (
    IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  TitleForeground,
    IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  TitleBackground,
    IN CHAR16                         *Title,        // 进度文字（UTF-16）
    IN EFI_GRAPHICS_OUTPUT_BLT_PIXEL  ProgressColor, // 忽略（用 #0078D4）
    IN UINTN                          Progress,      // 0-100
    IN UINTN                          PreviousValue  // 忽略
);

/**
 * 销毁 lvgl，GOP 恢复空白。
 * 在 BDS 进入 OS 启动前调用，释放资源供操作系统使用。
 */
EFI_STATUS EFIAPI BootLogoDisableLogo (VOID);
```

### 静态布局

```
┌─────────────────────────── 1280×800 ────────────────────────────┐
│                                                                  │
│                                                                  │
│                     Loongson Technology                          │  居中
│                                                                  │
│                                                                  │
│                        Starting...                               │  底部 -60px
│          ████████████████░░░░░░░░░░░░░░░░  60% 宽               │  底部 -40px，8px 高
└──────────────────────────────────────────────────────────────────┘
背景: #1A1A2E   进度条: #0078D4 (fill) / #333355 (track)
```

### 自定义厂商名

修改 `GuiBootLogoLib.c` 第 35 行：
```c
lv_label_set_text (VendorLabel, "Your Company Name");
```

---

## GuiDisplayEngineDxe — BIOS Setup UI

**文件**：`MdeModulePkg/Universal/GuiDisplayEngineDxe/`  
**接口**：实现 `EDKII_FORM_DISPLAY_ENGINE_PROTOCOL`（`Protocol/DisplayProtocol.h`）

### GuiDisplayEngine.c — 主文件

```c
/* 协议实例（全局） */
STATIC EDKII_FORM_DISPLAY_ENGINE_PROTOCOL mGuiDisplayEngine = {
    GuiFormDisplay,   // 渲染一个 HII 表单，阻塞直到用户操作
    GuiExitDisplay,   // 析构 lvgl
    NULL              // ConfirmDataChange（可选，未实现）
};
```

#### `GuiFormDisplay()` 执行流程

```
GuiFormDisplay(FormData, UserInput)
│
├─ 首次调用?  →  UefiLvglInit() + GuiLayoutInit()
│
├─ lv_obj_clean(gRightPanel)       // 清除上一个表单的控件
├─ 设置标题栏文字（FormData->FormTitle → UTF-8）
│
├─ 遍历 FormData->StatementListHead
│   └─ GuiCreateWidget(gRightPanel, Statement, FormData->HiiHandle)
│
└─ 事件循环 while(TRUE)
    ├─ UefiLvglTick()
    └─ ReadKeyStroke()
        ├─ SCAN_ESC  → UserInput->Action = BROWSER_ACTION_FORM_EXIT; break
        ├─ SCAN_F10  → UserInput->Action = BROWSER_ACTION_SUBMIT;    break
        └─ SCAN_F9   → UserInput->Action = BROWSER_ACTION_DEFAULT;   break
```

**重要**：`GuiFormDisplay` 是阻塞的（符合协议语义），直到用户按 ESC/F10/F9 才返回。

### Layout.c — 布局初始化

```c
/* 全局 lvgl 对象（在 GuiDisplayEngine.h 中 extern 声明） */
lv_obj_t *gRootContainer;  // 覆盖全屏的父容器
lv_obj_t *gTitleBar;       // 顶部蓝色标题栏（32px）
lv_obj_t *gLeftPanel;      // 左侧灰色导航面板（35%）
lv_obj_t *gRightPanel;     // 右侧白色表单内容区（65%）
lv_obj_t *gStatusBar;      // 底部深蓝状态栏（28px）
```

调用 `GuiLayoutInit(ScreenW, ScreenH)` 一次后即可。表单切换时只需 `lv_obj_clean(gRightPanel)` 并重新填充。

**调整比例**：修改 `GuiDisplayEngine.h` 中的宏：
```c
#define GUI_TITLE_HEIGHT      32   // 标题栏高度（像素）
#define GUI_STATUS_HEIGHT     28   // 状态栏高度（像素）
#define GUI_LEFT_PANEL_RATIO  35   // 左侧面板宽度占比（%）
```

### WidgetFactory.c — HII IFR → lvgl 控件

入口函数：
```c
lv_obj_t *GuiCreateWidget (
    IN lv_obj_t                       *Parent,
    IN FORM_DISPLAY_ENGINE_STATEMENT  *Statement,
    IN EFI_HII_HANDLE                  HiiHandle
);
```

每个 OpCode 对应一个 `CreateXxx()` 静态函数，返回 `lv_obj_t *`（如果不支持返回 `NULL`，父函数跳过）。

#### 控件实现详情

**Checkbox → `lv_switch`**
```c
// 水平 Row 布局：左侧 lv_label（Prompt），右侧 lv_switch
// 当前值：Statement->CurrentValue.Value.b (BOOLEAN)
lv_obj_set_flex_flow(Row, LV_FLEX_FLOW_ROW);
lv_obj_set_flex_align(Row, LV_FLEX_ALIGN_SPACE_BETWEEN, ...);
if (Statement->CurrentValue.Value.b)
    lv_obj_add_state(Sw, LV_STATE_CHECKED);
```

**Numeric → `lv_spinbox`**
```c
// 根据 EFI_IFR_NUMERIC_SIZE（1/2/4 字节）分别设置 range/value/step
switch (Numeric->Flags & EFI_IFR_NUMERIC_SIZE) {
    case EFI_IFR_NUMERIC_SIZE_1: /* u8 */ break;
    case EFI_IFR_NUMERIC_SIZE_2: /* u16 */ break;
    case EFI_IFR_NUMERIC_SIZE_4: /* u32 */ break;
}
```

**OneOf → `lv_dropdown`**
```c
// 遍历 Statement->OptionListHead，每个 DISPLAY_QUESTION_OPTION 加入一项
BASE_LIST_FOR_EACH(Link, &Statement->OptionListHead) {
    Option = DISPLAY_QUESTION_OPTION_FROM_LINK(Link);  // ← 必须用这个宏
    // 比较 Option->OptionOpCode->Value 和 CurrentValue 确定选中项
}
```

**Date → 三个 `lv_roller`（年/月/日）**
```c
// Year: 2000-2099，Month: 1-12，Day: 1-31
// 当前值：Statement->CurrentValue.Value.date.{Year,Month,Day}
```

**Ref → `lv_button` + `lv_label`**
```c
// 读取 Ref->Question.Header.Prompt 作为按钮文字
EFI_IFR_REF *Ref = (EFI_IFR_REF *)Statement->OpCode;
SetLabelFromHiiString(BtnLbl, HiiHandle, Ref->Question.Header.Prompt);
```

#### 新增 OpCode 支持

在 `GuiCreateWidget()` 的 `switch` 中添加新 `case`，然后实现对应的 `CreateXxx()` 函数，参考已有实现即可。当前返回 `NULL` 的 OpCode：

```c
case EFI_IFR_STRING_OP:       // → 待实现：lv_textarea
case EFI_IFR_PASSWORD_OP:     // → 待实现：lv_textarea（密码模式）
case EFI_IFR_ORDERED_LIST_OP: // → 待实现：可拖拽列表
```

### 辅助函数

```c
/* 从 HII 字符串数据库取字符串（UTF-16），需调用方 FreePool */
CHAR16 *GuiGetHiiString (
    IN EFI_HII_HANDLE  HiiHandle,
    IN EFI_STRING_ID   StringId
);

/* 设置 lv_label 文字，内部调用 GuiGetHiiString + Char16ToUtf8Simple */
STATIC VOID SetLabelFromHiiString (
    IN lv_obj_t       *Label,
    IN EFI_HII_HANDLE  HiiHandle,
    IN EFI_STRING_ID   StringId
);
```

---

## GuiBootManagerMenuApp — 启动菜单

**文件**：`MdeModulePkg/Application/GuiBootManagerMenuApp/GuiBootManagerMenuApp.c`  
**入口**：`BootManagerMenuEntry()`（与原 BootManagerMenuApp 同名，保持兼容）  
**FILE_GUID**：`EEC25BDC-67F2-4D95-B1D5-F81B2039D11D`（同原 App，无需修改 `PcdBootManagerMenuFile`）

### 执行流程

```
BootManagerMenuEntry()
│
├─ UefiLvglInit()
├─ 创建布局：标题栏 + 滚动列表容器 + 状态栏
│
├─ EfiBootManagerGetLoadOptions(&Count, LoadOptionTypeBoot)
│   └─ 遍历，每项创建 lv_obj_t（Item）+ lv_label（ItemLabel）
│       └─ gItems[i] = Item
│
├─ UpdateSelection(0)   // 高亮第一项
├─ UefiLvglTick()
│
└─ 事件循环
    ├─ SCAN_UP    → UpdateSelection(gSelectedIdx - 1)
    ├─ SCAN_DOWN  → UpdateSelection(gSelectedIdx + 1)
    ├─ Enter      → EfiBootManagerBoot(&BootOptions[gSelectedIdx])
    └─ SCAN_ESC   → Done = TRUE
```

### `UpdateSelection()` 实现

```c
STATIC VOID UpdateSelection (IN UINTN NewIdx) {
    // 旧选项：恢复深色背景
    lv_obj_set_style_bg_color(gItems[gSelectedIdx], lv_color_hex(0x2A2A3E), LV_PART_MAIN);
    gSelectedIdx = NewIdx;
    // 新选项：高亮蓝色 + 滚动到视图
    lv_obj_set_style_bg_color(gItems[gSelectedIdx], lv_color_hex(0x0078D4), LV_PART_MAIN);
    lv_obj_scroll_to_view(gItems[gSelectedIdx], LV_ANIM_OFF);
}
```

---

## 常见扩展场景

### 1. 修改配色方案

所有颜色宏集中在 `GuiDisplayEngine.h`：

```c
#define GUI_COLOR_BG          0xF0F0F0  // 全局背景
#define GUI_COLOR_PANEL       0xFFFFFF  // 左右面板底色
#define GUI_COLOR_TITLE_BG    0x2B5797  // 标题栏蓝色
#define GUI_COLOR_TITLE_TEXT  0xFFFFFF  // 标题文字
#define GUI_COLOR_HIGHLIGHT   0x0078D4  // 高亮/按钮
#define GUI_COLOR_STATUS_BG   0x1E3A6E  // 状态栏
#define GUI_COLOR_STATUS_TEXT 0xFFFFFF  // 状态栏文字
```

### 2. 添加 Logo 图片

在 `GuiBootLogoLib.c` 的 `EnsureInited()` 中，使用 `lv_image_create()` 并挂载 BMP/PNG（lvgl 内置解码器）：

```c
// 静态嵌入 BMP（用 lv_font_conv 或 xxd -i 转为 C 数组）
LV_IMAGE_DECLARE(my_logo_bmp);
lv_obj_t *Img = lv_image_create(Screen);
lv_image_set_src(Img, &my_logo_bmp);
lv_obj_align(Img, LV_ALIGN_CENTER, 0, -100);
```

### 3. 增加左侧导航树

在 `Layout.c` 的 `gLeftPanel` 中添加 `lv_list` 控件，并在 `GuiFormDisplay()` 中根据当前 `FormData->FormId` 高亮对应项：

```c
// Layout.c
lv_obj_t *NavList = lv_list_create(gLeftPanel);
lv_obj_set_size(NavList, lv_pct(100), lv_pct(100));
// GuiDisplayEngine.c GuiFormDisplay() 中：
// lv_list_add_button(NavList, NULL, "Boot Settings");
```

### 4. 响应控件值变化

当前实现只渲染控件（只读）。要响应用户修改，需要在 `CreateXxx()` 中注册 `lv_obj_add_event_cb()`，将新值写回 `Statement->CurrentValue`，并在退出时设置 `UserInput->SelectedStatement`：

```c
// 示例：switch 状态变化
lv_obj_add_event_cb(Sw, SwitchEventCb, LV_EVENT_VALUE_CHANGED, Statement);

STATIC VOID SwitchEventCb(lv_event_t *E) {
    FORM_DISPLAY_ENGINE_STATEMENT *Stmt = lv_event_get_user_data(E);
    Stmt->CurrentValue.Value.b = lv_obj_has_state(lv_event_get_target(E), LV_STATE_CHECKED);
}
```

---

## 调试方法

### 串口日志

所有 `DEBUG()` 宏输出通过串口。QEMU 启动时加 `-serial file:/tmp/serial.log`，然后：

```bash
tail -f /tmp/serial.log | grep -E "Gui|Lvgl|HII|ERROR"
```

常见输出：
```
[Bds] GuiFormDisplay: FormTitle=Front Page, Statements=3
[Bds] GuiCreateWidget: OpCode=0x06 (REF), Prompt=Device Manager
[Bds] GuiFormDisplay: User pressed ESC, action=0x20000
```

### 截图验证

```bash
# 在 BIOS Setup 状态下截图
./ui_run.sh shot mybuild
# → docs/screenshots/mybuild_t03.png / t08.png / t13.png
```

### 调整等待时间

`ui_run.sh shot` 模式的截图时间点（脚本内）：
```bash
take_shot "t03" 3    # t=3s：POST 启动画面
take_shot "t08" 5    # t=8s：Shell 完成后 + UiApp 渲染
take_shot "t13" 5    # t=13s：稳定界面
```

如果 QEMU 较慢（无 KVM），把延时改大（`3 → 10`，`5 → 10`）。

### lvgl 内部日志

在 `lv_conf.h` 中开启：
```c
#define LV_USE_LOG     1
#define LV_LOG_LEVEL   LV_LOG_LEVEL_INFO
#define LV_LOG_PRINTF  1   // 输出到标准输出（串口）
```

---

*最后更新：2026-05-24　　分支：ui*
