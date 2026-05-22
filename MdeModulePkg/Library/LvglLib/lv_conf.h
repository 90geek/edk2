/**
 * lv_conf.h - lvgl 编译配置（EDK2/UEFI 专用）
 * 禁用所有依赖 C 标准库的功能，启用 UEFI 集成
 * 适配 lvgl v9.2.2 宏命名规范
 */
#if 1  /* 启用此配置文件 */

#ifndef LV_CONF_H
#define LV_CONF_H

/*====================
   COLOR SETTINGS
 *====================*/

/* 颜色深度：32 匹配 GOP BGRA/XRGB8888 格式 */
#define LV_COLOR_DEPTH 32

/*=========================
   STDLIB WRAPPER SETTINGS
 *=========================*/

/*
 * v9.2.2 使用 LV_USE_STDLIB_MALLOC / LV_USE_STDLIB_STRING / LV_USE_STDLIB_SPRINTF
 * 可选值：LV_STDLIB_BUILTIN / LV_STDLIB_CLIB / LV_STDLIB_CUSTOM
 *
 * 使用 BUILTIN 分配器（内置 TLSF），内存池由 EDK2 静态数组提供。
 * 字符串函数同样使用 BUILTIN（lvgl 内置实现，无 stdlib 依赖）。
 */
#define LV_USE_STDLIB_MALLOC    LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_STRING    LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_SPRINTF   LV_STDLIB_BUILTIN

/*
 * 替换 C 标准头文件为 EDK2 提供的兼容头。
 * EDK2 编译环境通过 MdePkg 提供 stdint.h、stddef.h 等兼容层。
 */
#define LV_STDINT_INCLUDE       <stdint.h>
#define LV_STDDEF_INCLUDE       <stddef.h>
#define LV_STDBOOL_INCLUDE      <stdbool.h>
#define LV_INTTYPES_INCLUDE     <inttypes.h>
#define LV_LIMITS_INCLUDE       <limits.h>
#define LV_STDARG_INCLUDE       <stdarg.h>

/* BIOS Setup UI 预估用量：Theme+Widget树约 256KB，加 100% 余量 = 512KB
   Phase 1 可适当缩减，完整 UI (12种控件 + 鼠标光标) 建议不低于 256KB */
#define LV_MEM_SIZE (512 * 1024U)
#define LV_MEM_POOL_EXPAND_SIZE 0
#define LV_MEM_ADR 0                /* 0: 使用普通数组 */

/*====================
   HAL SETTINGS
 *====================*/

/* 刷新率：50ms 间隔 */
#define LV_DEF_REFR_PERIOD  50      /* [ms] */

#define LV_DPI_DEF 96               /* 典型屏幕 DPI */

/*=================
 * OPERATING SYSTEM
 *=================*/

/* UEFI 单线程环境，无 OS */
#define LV_USE_OS   LV_OS_NONE

/*========================
 * RENDERING CONFIGURATION
 *========================*/

#define LV_DRAW_BUF_STRIDE_ALIGN    1
#define LV_DRAW_BUF_ALIGN           4
#define LV_DRAW_TRANSFORM_USE_MATRIX 0
#define LV_DRAW_LAYER_SIMPLE_BUF_SIZE (24 * 1024)   /* [bytes] */
#define LV_DRAW_THREAD_STACK_SIZE     (8 * 1024)     /* [bytes] */

#define LV_USE_DRAW_SW 1
#if LV_USE_DRAW_SW == 1
    #define LV_DRAW_SW_SUPPORT_RGB565       1
    #define LV_DRAW_SW_SUPPORT_RGB565A8     1
    #define LV_DRAW_SW_SUPPORT_RGB888       1
    #define LV_DRAW_SW_SUPPORT_XRGB8888     1
    #define LV_DRAW_SW_SUPPORT_ARGB8888     1
    #define LV_DRAW_SW_SUPPORT_L8           1
    #define LV_DRAW_SW_SUPPORT_AL88         1
    #define LV_DRAW_SW_SUPPORT_A8           1
    #define LV_DRAW_SW_SUPPORT_I1           1
    #define LV_DRAW_SW_DRAW_UNIT_CNT        1
    #define LV_USE_DRAW_ARM2D_SYNC          0
    #define LV_USE_NATIVE_HELIUM_ASM        0
    #define LV_DRAW_SW_COMPLEX              1
    #if LV_DRAW_SW_COMPLEX == 1
        #define LV_DRAW_SW_SHADOW_CACHE_SIZE 0
        #define LV_DRAW_SW_CIRCLE_CACHE_SIZE 4
    #endif
    #define LV_USE_DRAW_SW_ASM              LV_DRAW_SW_ASM_NONE
    #define LV_USE_DRAW_SW_COMPLEX_GRADIENTS 0
#endif

/*=======================
 * FEATURE CONFIGURATION
 *=======================*/

/* 日志：关闭（无 printf） */
#define LV_USE_LOG 0

/* 断言 */
#define LV_USE_ASSERT_NULL          1
#define LV_USE_ASSERT_MALLOC        1
#define LV_USE_ASSERT_STYLE         0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ           0

/* 使用 EDK2 的 CpuDeadLoop()，语义与 while(1) 相同但对工具链更友好 */
#define LV_ASSERT_HANDLER_INCLUDE  <Library/BaseLib.h>
#define LV_ASSERT_HANDLER          CpuDeadLoop();

/* 调试覆盖层：关闭 */
#define LV_USE_REFR_DEBUG           0
#define LV_USE_LAYER_DEBUG          0
#define LV_USE_PARALLEL_DRAW_DEBUG  0

/* 其他 */
#define LV_ENABLE_GLOBAL_CUSTOM     0
#define LV_CACHE_DEF_SIZE           0
#define LV_IMAGE_HEADER_CACHE_DEF_CNT 0
#define LV_GRADIENT_MAX_STOPS       2
#define LV_COLOR_MIX_ROUND_OFS      0
#define LV_OBJ_STYLE_CACHE          0
#define LV_USE_OBJ_ID               0
#define LV_OBJ_ID_AUTO_ASSIGN       LV_USE_OBJ_ID
#define LV_USE_OBJ_ID_BUILTIN       1
#define LV_USE_OBJ_PROPERTY         0
#define LV_USE_OBJ_PROPERTY_NAME    1
#define LV_USE_VG_LITE_THORVG       0

/*=====================
 * COMPILER SETTINGS
 *====================*/

#define LV_BIG_ENDIAN_SYSTEM        0
#define LV_ATTRIBUTE_TICK_INC
#define LV_ATTRIBUTE_TIMER_HANDLER
#define LV_ATTRIBUTE_FLUSH_READY
#define LV_ATTRIBUTE_MEM_ALIGN_SIZE 1
#define LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_LARGE_CONST
#define LV_ATTRIBUTE_LARGE_RAM_ARRAY
#define LV_ATTRIBUTE_FAST_MEM
#define LV_EXPORT_CONST_INT(int_value) struct _silence_gcc_warning
#define LV_ATTRIBUTE_EXTERN_DATA

#define LV_USE_FLOAT                0
#define LV_USE_MATRIX               0
#define LV_USE_PRIVATE_API          0

/*==================
 *   FONT USAGE
 *===================*/

#define LV_FONT_MONTSERRAT_8  0
#define LV_FONT_MONTSERRAT_10 0
#define LV_FONT_MONTSERRAT_12 0
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 0
#define LV_FONT_MONTSERRAT_18 1
#define LV_FONT_MONTSERRAT_20 0
#define LV_FONT_MONTSERRAT_22 0
#define LV_FONT_MONTSERRAT_24 0
#define LV_FONT_MONTSERRAT_26 0
#define LV_FONT_MONTSERRAT_28 0
#define LV_FONT_MONTSERRAT_30 0
#define LV_FONT_MONTSERRAT_32 0
#define LV_FONT_MONTSERRAT_34 0
#define LV_FONT_MONTSERRAT_36 0
#define LV_FONT_MONTSERRAT_38 0
#define LV_FONT_MONTSERRAT_40 0
#define LV_FONT_MONTSERRAT_42 0
#define LV_FONT_MONTSERRAT_44 0
#define LV_FONT_MONTSERRAT_46 0
#define LV_FONT_MONTSERRAT_48 0

#define LV_FONT_MONTSERRAT_28_COMPRESSED 0
#define LV_FONT_DEJAVU_16_PERSIAN_HEBREW 0
#define LV_FONT_SIMSUN_14_CJK            0
#define LV_FONT_SIMSUN_16_CJK            0

#define LV_FONT_UNSCII_8  0
#define LV_FONT_UNSCII_16 0

#define LV_FONT_CUSTOM_DECLARE

/* 默认字体 */
#define LV_FONT_DEFAULT &lv_font_montserrat_14

#define LV_FONT_FMT_TXT_LARGE   0
#define LV_USE_FONT_COMPRESSED  0
#define LV_USE_FONT_PLACEHOLDER 1

/*=================
 *  TEXT SETTINGS
 *=================*/

/* 文本编码：UTF-8（支持 CJK） */
#define LV_TXT_ENC LV_TXT_ENC_UTF8

#define LV_TXT_BREAK_CHARS              " ,.;:-_)]}"
#define LV_TXT_LINE_BREAK_LONG_LEN      0
#define LV_TXT_LINE_BREAK_LONG_PRE_MIN_LEN  3
#define LV_TXT_LINE_BREAK_LONG_POST_MIN_LEN 3

#define LV_USE_BIDI                     0
#define LV_USE_ARABIC_PERSIAN_CHARS     0

/*==================
 * WIDGETS
 *================*/

/* 注意：v9.x 将 LV_USE_BTN 重命名为 LV_USE_BUTTON */
#define LV_WIDGETS_HAS_DEFAULT_VALUE    1

#define LV_USE_ANIMIMG      1
#define LV_USE_ARC          1
#define LV_USE_BAR          1
#define LV_USE_BUTTON       1   /* v9: LV_USE_BTN -> LV_USE_BUTTON */
#define LV_USE_BUTTONMATRIX 1
#define LV_USE_CALENDAR     0
#define LV_USE_CANVAS       1
#define LV_USE_CHART        0
#define LV_USE_CHECKBOX     1
#define LV_USE_DROPDOWN     1   /* 需要 lv_label */
#define LV_USE_IMAGE        1
#define LV_USE_IMAGEBUTTON  1
#define LV_USE_KEYBOARD     1
#define LV_USE_LABEL        1
#if LV_USE_LABEL
    #define LV_LABEL_TEXT_SELECTION  1
    #define LV_LABEL_LONG_TXT_HINT   1
    #define LV_LABEL_WAIT_CHAR_COUNT 3
#endif
#define LV_USE_LED          0
#define LV_USE_LINE         1
#define LV_USE_LIST         1
#define LV_USE_LOTTIE       0
#define LV_USE_MENU         1
#define LV_USE_MSGBOX       1
#define LV_USE_ROLLER       1   /* 需要 lv_label */
#define LV_USE_SCALE        1
#define LV_USE_SLIDER       1   /* 需要 lv_bar */
#define LV_USE_SPAN         1
#if LV_USE_SPAN
    #define LV_SPAN_SNIPPET_STACK_SIZE 64
#endif
#define LV_USE_SPINBOX      1
#define LV_USE_SPINNER      1
#define LV_USE_SWITCH       1
#define LV_USE_TEXTAREA     1   /* 需要 lv_label */
#if LV_USE_TEXTAREA != 0
    #define LV_TEXTAREA_DEF_PWD_SHOW_TIME 1500  /* ms */
#endif
#define LV_USE_TABLE        1
#define LV_USE_TABVIEW      0
#define LV_USE_TILEVIEW     0
#define LV_USE_WIN          0

/*==================
 * THEMES
 *==================*/

#define LV_USE_THEME_DEFAULT 1
#if LV_USE_THEME_DEFAULT
    #define LV_THEME_DEFAULT_DARK           0
    #define LV_THEME_DEFAULT_GROW           1
    #define LV_THEME_DEFAULT_TRANSITION_TIME 80
#endif

#define LV_USE_THEME_SIMPLE 1
#define LV_USE_THEME_MONO   1

/*==================
 * LAYOUTS
 *==================*/

#define LV_USE_FLEX 1
#define LV_USE_GRID 1

/*====================
 * 3RD PARTY LIBRARIES
 *====================*/

/* 文件系统：UEFI 环境全部禁用 */
#define LV_FS_DEFAULT_DRIVE_LETTER '\0'
#define LV_USE_FS_STDIO     0
#define LV_USE_FS_POSIX     0
#define LV_USE_FS_WIN32     0
#define LV_USE_FS_FATFS     0
#define LV_USE_FS_MEMFS     0
#define LV_USE_FS_LITTLEFS  0

/* 图像解码器 */
/* PNG 解码：Phase 1 禁用，避免 lodepng.c 引入 <stdlib.h>
   Phase 2 启用时需在 INF [BuildOptions] 添加 -DLODEPNG_NO_COMPILE_DISK
   并 patch lv_lodepng.c 移除裸 #include <stdlib.h> */
#define LV_USE_LODEPNG      0
#define LV_USE_LIBPNG       0
#define LV_USE_BMP          0
#define LV_USE_TJPGD        0   /* v9: LV_USE_SJPG -> LV_USE_TJPGD */
#define LV_USE_LIBJPEG_TURBO 0
#define LV_USE_GIF          0
#define LV_BIN_DECODER_RAM_LOAD 0
#define LV_USE_RLE          0
#define LV_USE_QRCODE       0
#define LV_USE_BARCODE      0
#define LV_USE_FREETYPE     0
#define LV_USE_TINY_TTF     0
#define LV_USE_RLOTTIE      0
#define LV_USE_VECTOR_GRAPHIC 0
#define LV_USE_THORVG_INTERNAL 0
#define LV_USE_THORVG_EXTERNAL 0
#define LV_USE_LZ4_INTERNAL 0
#define LV_USE_LZ4_EXTERNAL 0
#define LV_USE_FFMPEG       0

/*==================
 * OTHERS
 *==================*/

#define LV_USE_SNAPSHOT     0
#define LV_USE_SYSMON       0
#define LV_USE_PROFILER     0
#define LV_USE_MONKEY       0
#define LV_USE_GRIDNAV      0
#define LV_USE_FRAGMENT     0
#define LV_USE_IMGFONT      0
#define LV_USE_OBSERVER     1
#define LV_USE_IME_PINYIN   0
#define LV_USE_FILE_EXPLORER 0

/*==================
 * DEVICES
 *==================*/

/* 所有平台驱动禁用（UEFI 通过 GOP 直接操作帧缓冲） */
#define LV_USE_SDL          0
#define LV_USE_X11          0
#define LV_USE_WAYLAND      0
#define LV_USE_LINUX_FBDEV  0
#define LV_USE_NUTTX        0
#define LV_USE_LINUX_DRM    0
#define LV_USE_TFT_ESPI     0
#define LV_USE_EVDEV        0
#define LV_USE_LIBINPUT     0
#define LV_USE_ST7735       0
#define LV_USE_ST7789       0
#define LV_USE_ST7796       0
#define LV_USE_ILI9341      0
#define LV_USE_GENERIC_MIPI (LV_USE_ST7735 | LV_USE_ST7789 | LV_USE_ST7796 | LV_USE_ILI9341)
#define LV_USE_RENESAS_GLCDC 0
#define LV_USE_WINDOWS      0
#define LV_USE_OPENGLES     0
#define LV_USE_QNX          0

/*==================
 * EXAMPLES
 *==================*/

#define LV_BUILD_EXAMPLES 0

/*===================
 * DEMO USAGE
 ====================*/

#define LV_USE_DEMO_WIDGETS             0
#define LV_USE_DEMO_KEYPAD_AND_ENCODER  0
#define LV_USE_DEMO_BENCHMARK           0
#define LV_USE_DEMO_RENDER              0
#define LV_USE_DEMO_STRESS              0
#define LV_USE_DEMO_MUSIC               0
#define LV_USE_DEMO_FLEX_LAYOUT         0
#define LV_USE_DEMO_MULTILANG           0
#define LV_USE_DEMO_TRANSFORM           0
#define LV_USE_DEMO_SCROLL              0
#define LV_USE_DEMO_VECTOR_GRAPHIC      0

/*--END OF LV_CONF_H--*/

#endif /* LV_CONF_H */
#endif /* 启用此配置文件 */
