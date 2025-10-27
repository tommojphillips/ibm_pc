/* ui.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * ImGUI C Wrapper
 */

#ifndef UI
#define UI

#ifdef UI_EXPORTS
#define UI_EXPORT __declspec(dllexport)
#else
#define UI_EXPORT __declspec(dllimport)
#endif

typedef int UI_WINDOW_FLAGS;
enum {
    UI_WINDOW_FLAGS_None = 0,
    UI_WINDOW_FLAGS_NoTitleBar =                  1 << 0,   // Disable title-bar
    UI_WINDOW_FLAGS_NoResize =                    1 << 1,   // Disable user resizing with the lower-right grip
    UI_WINDOW_FLAGS_NoMove =                      1 << 2,   // Disable user moving the window
    UI_WINDOW_FLAGS_NoScrollbar =                 1 << 3,   // Disable scrollbars (window can still scroll with mouse or programmatically)
    UI_WINDOW_FLAGS_NoScrollWithMouse =           1 << 4,   // Disable user vertically scrolling with mouse wheel. On child window, mouse wheel will be forwarded to the parent unless NoScrollbar is also set.
    UI_WINDOW_FLAGS_NoCollapse =                  1 << 5,   // Disable user collapsing window by double-clicking on it. Also referred to as Window Menu Button (e.g. within a docking node).
    UI_WINDOW_FLAGS_AlwaysAutoResize =            1 << 6,   // Resize every window to its content every frame
    UI_WINDOW_FLAGS_NoBackground =                1 << 7,   // Disable drawing background color (WindowBg, etc.) and outside border. Similar as using SetNextWindowBgAlpha(0.0f).
    UI_WINDOW_FLAGS_NoSavedSettings =             1 << 8,   // Never load/save settings in .ini file
    UI_WINDOW_FLAGS_NoMouseInputs =               1 << 9,   // Disable catching mouse, hovering test with pass through.
    UI_WINDOW_FLAGS_MenuBar =                     1 << 10,  // Has a menu-bar
    UI_WINDOW_FLAGS_HorizontalScrollbar =         1 << 11,  // Allow horizontal scrollbar to appear (off by default). You may use SetNextWindowContentSize(ImVec2(width,0.0f)); prior to calling Begin() to specify width. Read code in imgui_demo in the "Horizontal Scrolling" section.
    UI_WINDOW_FLAGS_NoFocusOnAppearing =          1 << 12,  // Disable taking focus when transitioning from hidden to visible state
    UI_WINDOW_FLAGS_NoBringToFrontOnFocus =       1 << 13,  // Disable bringing window to front when taking focus (e.g. clicking on it or programmatically giving it focus)
    UI_WINDOW_FLAGS_AlwaysVerticalScrollbar =     1 << 14,  // Always show vertical scrollbar (even if ContentSize.y < Size.y)
    UI_WINDOW_FLAGS_AlwaysHorizontalScrollbar =   1 << 15,  // Always show horizontal scrollbar (even if ContentSize.x < Size.x)
    UI_WINDOW_FLAGS_NoNavInputs =                 1 << 16,  // No keyboard/gamepad navigation within the window
    UI_WINDOW_FLAGS_NoNavFocus =                  1 << 17,  // No focusing toward this window with keyboard/gamepad navigation (e.g. skipped by CTRL+TAB)
    UI_WINDOW_FLAGS_UnsavedDocument =             1 << 18,  // Display a dot next to the title. When used in a tab/docking context, tab is selected when clicking the X + closure is not assumed (will wait for user to stop submitting the tab). Otherwise closure is assumed when pressing the X, so if you keep submitting the tab may reappear at end of tab bar.
};

typedef int UI_COLOR;
enum {
    UI_COLOR_Text,
    UI_COLOR_TextDisabled,
    UI_COLOR_WindowBg,              // Background of normal windows
    UI_COLOR_ChildBg,               // Background of child windows
    UI_COLOR_PopupBg,               // Background of popups, menus, tooltips windows
    UI_COLOR_Border,
    UI_COLOR_BorderShadow,
    UI_COLOR_FrameBg,               // Background of checkbox, radio button, plot, slider, text input
    UI_COLOR_FrameBgHovered,
    UI_COLOR_FrameBgActive,
    UI_COLOR_TitleBg,               // Title bar
    UI_COLOR_TitleBgActive,         // Title bar when focused
    UI_COLOR_TitleBgCollapsed,      // Title bar when collapsed
    UI_COLOR_MenuBarBg,
    UI_COLOR_ScrollbarBg,
    UI_COLOR_ScrollbarGrab,
    UI_COLOR_ScrollbarGrabHovered,
    UI_COLOR_ScrollbarGrabActive,
    UI_COLOR_CheckMark,             // Checkbox tick and RadioButton circle
    UI_COLOR_SliderGrab,
    UI_COLOR_SliderGrabActive,
    UI_COLOR_Button,
    UI_COLOR_ButtonHovered,
    UI_COLOR_ButtonActive,
    UI_COLOR_Header,                // Header* colors are used for CollapsingHeader, TreeNode, Selectable, MenuItem
    UI_COLOR_HeaderHovered,
    UI_COLOR_HeaderActive,
    UI_COLOR_Separator,
    UI_COLOR_SeparatorHovered,
    UI_COLOR_SeparatorActive,
    UI_COLOR_ResizeGrip,            // Resize grip in lower-right and lower-left corners of windows.
    UI_COLOR_ResizeGripHovered,
    UI_COLOR_ResizeGripActive,
    UI_COLOR_InputTextCursor,       // InputText cursor/caret
    UI_COLOR_TabHovered,            // Tab background, when hovered
    UI_COLOR_Tab,                   // Tab background, when tab-bar is focused & tab is unselected
    UI_COLOR_TabSelected,           // Tab background, when tab-bar is focused & tab is selected
    UI_COLOR_TabSelectedOverline,   // Tab horizontal overline, when tab-bar is focused & tab is selected
    UI_COLOR_TabDimmed,             // Tab background, when tab-bar is unfocused & tab is unselected
    UI_COLOR_TabDimmedSelected,     // Tab background, when tab-bar is unfocused & tab is selected
    UI_COLOR_TabDimmedSelectedOverline,//..horizontal overline, when tab-bar is unfocused & tab is selected
    UI_COLOR_PlotLines,
    UI_COLOR_PlotLinesHovered,
    UI_COLOR_PlotHistogram,
    UI_COLOR_PlotHistogramHovered,
    UI_COLOR_TableHeaderBg,         // Table header background
    UI_COLOR_TableBorderStrong,     // Table outer and header borders (prefer using Alpha=1.0 here)
    UI_COLOR_TableBorderLight,      // Table inner borders (prefer using Alpha=1.0 here)
    UI_COLOR_TableRowBg,            // Table row background (even rows)
    UI_COLOR_TableRowBgAlt,         // Table row background (odd rows)
    UI_COLOR_TextLink,              // Hyperlink color
    UI_COLOR_TextSelectedBg,        // Selected text inside an InputText
    UI_COLOR_TreeLines,             // Tree node hierarchy outlines when using ImGuiTreeNodeFlags_DrawLines
    UI_COLOR_DragDropTarget,        // Rectangle highlighting a drop target
    UI_COLOR_NavCursor,             // Color of keyboard/gamepad navigation cursor/rectangle, when visible
    UI_COLOR_NavWindowingHighlight, // Highlight window when using CTRL+TAB
    UI_COLOR_NavWindowingDimBg,     // Darken/colorize entire screen behind the CTRL+TAB window list, when active
    UI_COLOR_ModalWindowDimBg,      // Darken/colorize entire screen behind a modal window, when one is active
    UI_COLOR_COUNT,
};

typedef int UI_STYLE_VAR;
enum {
    UI_STYLE_VAR_Alpha,                    // float     Alpha
    UI_STYLE_VAR_DisabledAlpha,            // float     DisabledAlpha
    UI_STYLE_VAR_WindowPadding,            // ImVec2    WindowPadding
    UI_STYLE_VAR_WindowRounding,           // float     WindowRounding
    UI_STYLE_VAR_WindowBorderSize,         // float     WindowBorderSize
    UI_STYLE_VAR_WindowMinSize,            // ImVec2    WindowMinSize
    UI_STYLE_VAR_WindowTitleAlign,         // ImVec2    WindowTitleAlign
    UI_STYLE_VAR_ChildRounding,            // float     ChildRounding
    UI_STYLE_VAR_ChildBorderSize,          // float     ChildBorderSize
    UI_STYLE_VAR_PopupRounding,            // float     PopupRounding
    UI_STYLE_VAR_PopupBorderSize,          // float     PopupBorderSize
    UI_STYLE_VAR_FramePadding,             // ImVec2    FramePadding
    UI_STYLE_VAR_FrameRounding,            // float     FrameRounding
    UI_STYLE_VAR_FrameBorderSize,          // float     FrameBorderSize
    UI_STYLE_VAR_ItemSpacing,              // ImVec2    ItemSpacing
    UI_STYLE_VAR_ItemInnerSpacing,         // ImVec2    ItemInnerSpacing
    UI_STYLE_VAR_IndentSpacing,            // float     IndentSpacing
    UI_STYLE_VAR_CellPadding,              // ImVec2    CellPadding
    UI_STYLE_VAR_ScrollbarSize,            // float     ScrollbarSize
    UI_STYLE_VAR_ScrollbarRounding,        // float     ScrollbarRounding
    UI_STYLE_VAR_ScrollbarPadding,         // float     ScrollbarPadding
    UI_STYLE_VAR_GrabMinSize,              // float     GrabMinSize
    UI_STYLE_VAR_GrabRounding,             // float     GrabRounding
    UI_STYLE_VAR_ImageBorderSize,          // float     ImageBorderSize
    UI_STYLE_VAR_TabRounding,              // float     TabRounding
    UI_STYLE_VAR_TabBorderSize,            // float     TabBorderSize
    UI_STYLE_VAR_TabMinWidthBase,          // float     TabMinWidthBase
    UI_STYLE_VAR_TabMinWidthShrink,        // float     TabMinWidthShrink
    UI_STYLE_VAR_TabBarBorderSize,         // float     TabBarBorderSize
    UI_STYLE_VAR_TabBarOverlineSize,       // float     TabBarOverlineSize
    UI_STYLE_VAR_TableAngledHeadersAngle,  // float     TableAngledHeadersAngle
    UI_STYLE_VAR_TableAngledHeadersTextAlign,// ImVec2  TableAngledHeadersTextAlign
    UI_STYLE_VAR_TreeLinesSize,            // float     TreeLinesSize
    UI_STYLE_VAR_TreeLinesRounding,        // float     TreeLinesRounding
    UI_STYLE_VAR_ButtonTextAlign,          // ImVec2    ButtonTextAlign
    UI_STYLE_VAR_SelectableTextAlign,      // ImVec2    SelectableTextAlign
    UI_STYLE_VAR_SeparatorTextBorderSize,  // float     SeparatorTextBorderSize
    UI_STYLE_VAR_SeparatorTextAlign,       // ImVec2    SeparatorTextAlign
    UI_STYLE_VAR_SeparatorTextPadding,     // ImVec2    SeparatorTextPadding
    UI_STYLE_VAR_COUNT
};

typedef int UI_CHECKBOX;
enum {
    UI_CHECKBOX_LEFT,
    UI_CHECKBOX_RIGHT,
};

typedef struct VECTOR2 {
    float x;
    float y;
} VECTOR2;

#ifdef __cplusplus
extern "C" {
#endif

UI_EXPORT void ui_create_renderer(SDL_Window* window, SDL_Renderer* renderer);
UI_EXPORT void ui_destroy(void);
UI_EXPORT void ui_new_frame(void);
UI_EXPORT void ui_render(void);
UI_EXPORT void ui_process_event(void* param, SDL_Event* e);

UI_EXPORT void ui_text(const char* fmt, ...);

UI_EXPORT int ui_button(const char* label);
UI_EXPORT int ui_checkbox(UI_CHECKBOX alignment, const char* label, int* state);

UI_EXPORT void ui_same_line(void);
UI_EXPORT void ui_same_line_spacing(float spacing);

UI_EXPORT void ui_begin(const char* name, int* state, UI_WINDOW_FLAGS window_flags);
UI_EXPORT void ui_end(void);

UI_EXPORT int ui_menu_item(const char* label);
UI_EXPORT int ui_menu_button(const char* label, int selected, int enabled);

UI_EXPORT int ui_menu_checkbox(const char* label, int* state);
UI_EXPORT int ui_menu_checkbox_u8(const char* label, uint8_t* state);
UI_EXPORT int ui_menu_checkbox_u16(const char* label, uint16_t* state);
UI_EXPORT int ui_menu_checkbox_u32(const char* label, uint32_t* state);

UI_EXPORT int ui_begin_menu(const char* label);
UI_EXPORT void ui_end_menu(void);

UI_EXPORT int ui_begin_menu_bar(void);
UI_EXPORT void ui_end_menu_bar(void);

UI_EXPORT int ui_begin_main_menu_bar(void);
UI_EXPORT void ui_end_main_menu_bar(void);

UI_EXPORT void ui_set_next_window_position(float x, float y);
UI_EXPORT void ui_set_next_window_size(float x, float y);

UI_EXPORT VECTOR2 ui_get_window_size(void);

UI_EXPORT void ui_push_style_color(UI_COLOR type, float r, float g, float b, float a);
UI_EXPORT void ui_pop_style_color(int count);

UI_EXPORT void ui_push_style_var_vec(UI_STYLE_VAR type, float x, float y);
UI_EXPORT void ui_push_style_var_float(UI_STYLE_VAR type, float x);
UI_EXPORT void ui_pop_style_var(int count);

UI_EXPORT void ui_begin_disabled(int disabled);
UI_EXPORT void ui_end_disabled(void);

UI_EXPORT VECTOR2 ui_get_display_size(void);
UI_EXPORT VECTOR2 ui_get_mouse_position(void);
UI_EXPORT float ui_get_frame_height(void);
UI_EXPORT float ui_get_delta_time(void);
UI_EXPORT float ui_get_text_line_height(void);
UI_EXPORT VECTOR2 ui_get_frame_padding(void);

UI_EXPORT float ui_lerp(float a, float b, float t);

UI_EXPORT int ui_is_popup_open(const char* str_id);

UI_EXPORT int ui_dipswitch(const char* label, int* state);
UI_EXPORT int ui_dipswitch_u8(const char* label, uint8_t* state, uint8_t enable_mask);
UI_EXPORT int ui_dipswitch_u16(const char* label, uint16_t* state, uint16_t enable_mask);
UI_EXPORT int ui_dipswitch_u32(const char* label, uint32_t* state, uint32_t enable_mask);

UI_EXPORT void ui_separator(void);

#ifdef __cplusplus
};
#endif

#endif
