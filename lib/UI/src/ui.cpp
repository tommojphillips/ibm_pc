/* ui.cpp
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * ImGUI C Wrapper
 */

#include <stdio.h>

#include <SDL3/SDL_render.h>


#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
using namespace ImGui;

#include "ui.h"

/* Imgui state */
typedef struct {
	ImGuiContext* context;
	SDL_Window* window;
	SDL_Renderer* renderer;
} IMGUI_STATE;

static IMGUI_STATE imgui = { 0 };

void ui_create_renderer(SDL_Window* window, SDL_Renderer* renderer) {

	IMGUI_CHECKVERSION();
	imgui.context = CreateContext();
	if (imgui.context == NULL) {
		printf("Failed to create IMGUI Context");
		return;
	}

	StyleColorsDark();
	
	ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
	ImGui_ImplSDLRenderer3_Init(renderer);

	imgui.window = window;
	imgui.renderer = renderer;
}

void ui_destroy() {
	ImGui_ImplSDLRenderer3_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	DestroyContext(imgui.context);
}

void ui_new_frame() {
	ImGui_ImplSDLRenderer3_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	NewFrame();
}

void ui_render() {
	Render();
	ImGui_ImplSDLRenderer3_RenderDrawData(GetDrawData(), imgui.renderer);
}

void ui_process_event(void* param, SDL_Event* e) {
	(void)param;
	ImGui_ImplSDL3_ProcessEvent(e);
}

void ui_text(const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	TextV(fmt, args);
	va_end(args);
}
void ui_text_disabled(const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	TextDisabledV(fmt, args);
	va_end(args);
}
void ui_text_colored(float r, float g, float b, float a, const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	TextColoredV(ImVec4(r, g, b, a), fmt, args);
	va_end(args);
}
void ui_text_colored_vec(VECTOR4* vector, const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	TextColoredV(ImVec4(vector->x, vector->y, vector->z, vector->w), fmt, args);
	va_end(args);
}

int ui_button(const char* label) {
	if (Button(label)) {
		return 1;
	}
	return 0;
}

static bool IsRootOfOpenMenuSet() {
	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = g.CurrentWindow;
	if ((g.OpenPopupStack.Size <= g.BeginPopupStack.Size) || (window->Flags & ImGuiWindowFlags_ChildMenu)) {
		return false;
	}
	const ImGuiPopupData* upper_popup = &g.OpenPopupStack[g.BeginPopupStack.Size];
	if (window->DC.NavLayerCurrent != upper_popup->ParentNavLayer) {
		return false;
	}
	return upper_popup->Window && (upper_popup->Window->Flags & ImGuiWindowFlags_ChildMenu) && ImGui::IsWindowChildOf(upper_popup->Window, window, true);
}
bool MenuItem_Ex(const char* label, const char* icon, const char* shortcut, bool selected, bool enabled, ImGuiSelectableFlags extra_selectable_flags) {
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems) {
		return false;
	}

	ImGuiContext& g = *GImGui;
	ImGuiStyle& style = g.Style;
	ImVec2 pos = window->DC.CursorPos;
	ImVec2 label_size = CalcTextSize(label, NULL, true);

	const bool menuset_is_open = IsRootOfOpenMenuSet();
	if (menuset_is_open) {
		PushItemFlag(ImGuiItemFlags_NoWindowHoverableCheck, true);
	}
	bool pressed;
	PushID(label);
	if (!enabled) {
		BeginDisabled();
	}

	const ImGuiSelectableFlags selectable_flags = ImGuiSelectableFlags_SelectOnRelease | ImGuiSelectableFlags_NoSetKeyOwner | ImGuiSelectableFlags_SetNavIdOnHover | extra_selectable_flags;
	const ImGuiMenuColumns* offsets = &window->DC.MenuColumns;
	if (window->DC.LayoutType == ImGuiLayoutType_Horizontal) {
		// Mimic the exact layout spacing of BeginMenu() to allow MenuItem() inside a menu bar, which is a little misleading but may be useful
		// Note that in this situation: we don't render the shortcut, we render a highlight instead of the selected tick mark.
		float w = label_size.x;
		window->DC.CursorPos.x += IM_TRUNC(style.ItemSpacing.x * 0.5f);
		ImVec2 text_pos(window->DC.CursorPos.x + offsets->OffsetLabel, window->DC.CursorPos.y + window->DC.CurrLineTextBaseOffset);
		PushStyleVarX(ImGuiStyleVar_ItemSpacing, style.ItemSpacing.x * 2.0f);
		pressed = Selectable("", selected, selectable_flags, ImVec2(w, 0.0f));
		PopStyleVar();
		if (g.LastItemData.StatusFlags & ImGuiItemStatusFlags_Visible) {
			RenderText(text_pos, label);
		}
		window->DC.CursorPos.x += IM_TRUNC(style.ItemSpacing.x * (-1.0f + 0.5f)); // -1 spacing to compensate the spacing added when Selectable() did a SameLine(). It would also work to call SameLine() ourselves after the PopStyleVar().
	}
	else {
		// Menu item inside a vertical menu
		float icon_w = (icon && icon[0]) ? CalcTextSize(icon, NULL).x : 0.0f;
		float shortcut_w = (shortcut && shortcut[0]) ? CalcTextSize(shortcut, NULL).x : 0.0f;
		float checkmark_w = IM_TRUNC(g.FontSize * 1.20f);
		float min_w = window->DC.MenuColumns.DeclColumns(icon_w, label_size.x, shortcut_w, checkmark_w); // Feedback for next frame
		float stretch_w = ImMax(0.0f, GetContentRegionAvail().x - min_w);
		pressed = Selectable("", false, selectable_flags | ImGuiSelectableFlags_SpanAvailWidth, ImVec2(min_w, label_size.y));
		if (g.LastItemData.StatusFlags & ImGuiItemStatusFlags_Visible) {
			RenderText(pos + ImVec2(offsets->OffsetLabel, 0.0f), label);
			if (icon_w > 0.0f) {
				RenderText(pos + ImVec2(offsets->OffsetIcon, 0.0f), icon);
			}
			if (shortcut_w > 0.0f) {
				PushStyleColor(ImGuiCol_Text, style.Colors[ImGuiCol_TextDisabled]);
				LogSetNextTextDecoration("(", ")");
				RenderText(pos + ImVec2(offsets->OffsetShortcut + stretch_w, 0.0f), shortcut, NULL, false);
				PopStyleColor();
			}
			if (selected) {
				RenderCheckMark(window->DrawList, pos + ImVec2(offsets->OffsetMark + stretch_w + g.FontSize * 0.40f, g.FontSize * 0.134f * 0.5f), GetColorU32(ImGuiCol_Text), g.FontSize * 0.866f);
			}
		}
	}
	IMGUI_TEST_ENGINE_ITEM_INFO(g.LastItemData.ID, label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable | (selected ? ImGuiItemStatusFlags_Checked : 0));
	if (!enabled) {
		EndDisabled();
	}
	PopID();
	if (menuset_is_open) {
		PopItemFlag();
	}

	return pressed;
}

int ui_menu_item(const char* label) {
	if (MenuItem_Ex(label, NULL, NULL, false, true, ImGuiSelectableFlags_None)) {
		return 1;
	}
	return 0;
}

int ui_menu_button(const char* label, int selected, int enabled) {
	if (MenuItem_Ex(label, NULL, NULL, selected, enabled, ImGuiSelectableFlags_NoAutoClosePopups)) {
		return 1;
	}
	return 0;
}

int ui_menu_checkbox(const char* label, int* state) {
	bool s = *state;
	if (MenuItem_Ex(label, NULL, NULL, s, true, ImGuiSelectableFlags_NoAutoClosePopups)) {
		*state ^= 1;
		return 1;
	}
	return 0;
}
int ui_menu_checkbox_u8(const char* label, uint8_t* state) {
	bool s = *state;
	if (MenuItem_Ex(label, NULL, NULL, s, true, ImGuiSelectableFlags_NoAutoClosePopups)) {
		*state ^= 1;
		return 1;
	}
	return 0;
}
int ui_menu_checkbox_u16(const char* label, uint16_t* state) {
	bool s = *state;
	if (MenuItem_Ex(label, NULL, NULL, (bool)*state, true, ImGuiSelectableFlags_NoAutoClosePopups)) {
		*state ^= 1;
		return 1;
	}
	return 0;
}
int ui_menu_checkbox_u32(const char* label, uint32_t* state) {
	bool s = *state;
	if (MenuItem_Ex(label, NULL, NULL, s, true, ImGuiSelectableFlags_NoAutoClosePopups)) {
		*state ^= 1;
		return 1;
	}
	return 0;
}

int ui_begin_menu(const char* label) {
	if (BeginMenu(label)) {
		return 1;
	}
	return 0;
}

void ui_end_menu() {
	EndMenu();
}

int ui_begin_menu_bar() {
	if (BeginMenuBar()) {
		return 1;
	}
	return 0;
}

void ui_end_menu_bar() {
	EndMenuBar();
}

int ui_begin_main_menu_bar() {
	if (BeginMainMenuBar()) {
		return 1;
	}
	return 0;
}

void ui_end_main_menu_bar() {
	EndMainMenuBar();
}

VECTOR2 ui_get_window_size() {
	ImVec2 vec = GetWindowSize();
	VECTOR2 vector2 = { vec.x, vec.y };
	return vector2;
}

int ui_checkbox(UI_CHECKBOX alignment, const char* label, int* state) {
	int ret = 0;

	PushStyleColor(ImGuiCol_FrameBgHovered, GetStyle().Colors[ImGuiCol_FrameBg]);

	switch (alignment) {
		case UI_CHECKBOX_LEFT:
			if (Checkbox("##nolabel", (bool*)state)) {
				ret = 1;
				*state ^= 1;
			}
			SameLine();
			if (Button(label)) {
				ret = 1;
				*state ^= 1;
			}
		break;

		case UI_CHECKBOX_RIGHT:
			if (Button(label)) {
				ret = 1;
				*state ^= 1;
			}
			SameLine();
			if (Checkbox("##nolabel", (bool*)state)) {
				ret = 1;
			}
		break;

	}

	PopStyleColor(1);

	return ret;
}

void ui_begin_disabled(int disabled) {
	BeginDisabled(disabled);
}

void ui_end_disabled() {
	EndDisabled();
}

void ui_same_line() {
	SameLine();
}

void ui_same_line_spacing(float spacing) {
	SameLine(0, spacing);
}

void ui_begin(const char* name, int* state, UI_WINDOW_FLAGS window_flags) {
	Begin(name, (bool*)state, window_flags);
}

void ui_end() {
	End();
}

void ui_set_next_window_position(float x, float y) {
	const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
	SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + x, main_viewport->WorkPos.y + y), ImGuiCond_Always);
}

void ui_set_next_window_size(float x, float y) {
	SetNextWindowSize(ImVec2(x, y), ImGuiCond_Always);
}

void ui_push_style_color(UI_COLOR type, float r, float g, float b, float a) {
	PushStyleColor(type, ImVec4(r, g, b, a));
}
void ui_push_style_color_vec(UI_COLOR type, VECTOR4* vector) {
	PushStyleColor(type, ImVec4(vector->x, vector->y, vector->z, vector->w));
}

void ui_pop_style_color(int count) {
	PopStyleColor(count);
}

void ui_push_style_var_vec(UI_STYLE_VAR type, float x, float y) {
	PushStyleVar(type, ImVec2(x, y));
}

void ui_push_style_var_float(UI_STYLE_VAR type, float x) {
	PushStyleVar(type, x);
}

void ui_pop_style_var(int count) {
	PopStyleVar(count);
}

VECTOR2 ui_get_display_size() {
	ImVec2 display_size = GetIO().DisplaySize;
	VECTOR2 vector2 = { display_size.x, display_size.y };
	return vector2;
}

VECTOR2 ui_get_mouse_position() {
	ImVec2 mouse_pos = GetMousePos();
	VECTOR2 vector2 = { mouse_pos.x, mouse_pos.y };
	return vector2;
}

float ui_get_frame_height() {
	return GetFrameHeight();
}

float ui_get_delta_time() {
	return GetIO().DeltaTime;
}

float ui_get_text_line_height() {
	return GetTextLineHeight();
}

VECTOR2 ui_get_frame_padding() {
	ImGuiStyle& style = GetStyle();
	VECTOR2 vector2 = { style.FramePadding.x, style.FramePadding.y };
	return vector2;
}

float ui_lerp(float a, float b, float t) {
	return ImLerp(a, b, t);
}

int ui_is_popup_open(const char* str_id) {
	if (IsPopupOpen(str_id, ImGuiPopupFlags_AnyPopup)) {
		return 1;
	}
	return 0;
}

int ui_dipswitch(const char* label, int* state) {
	ImDrawList* draw = GetWindowDrawList();
	ImVec2 pos = GetCursorScreenPos();
	float width = 16.0f;
	float height = 20.0f;

	// Handle input
	InvisibleButton(label, ImVec2(width, height));
	bool hovered = IsItemHovered();
	bool clicked = IsItemClicked();

	if (clicked) {
		*state ^= 1;
	}

	// Colors
	ImU32 col_body = IM_COL32(60, 60, 60, 255); // dark plastic
	
	ImU32 col_switch = 0;
	if (hovered) {
		col_switch = GetColorU32(UI_COLOR_ButtonHovered);
	}
	else { // ON = ?; OFF = light gray
		if (*state) {
			col_switch = GetColorU32(UI_COLOR_ButtonActive);
		}
		else {
			col_switch = col_switch = GetColorU32(UI_COLOR_Text); //IM_COL32(180, 180, 180, 255);
		}
	}

	// Draw base rectangle
	draw->AddRectFilled(pos, ImVec2(pos.x + width, pos.y + height), col_body, 2.0f);

	// Draw switch lever rectangle
	float lever_h = height * 0.45f;
	float lever_y = *state ? pos.y + 2.0f : pos.y + height - lever_h - 2.0f;
	draw->AddRectFilled(ImVec2(pos.x + 3, lever_y), ImVec2(pos.x + width - 3, lever_y + lever_h), col_switch, 1.5f);

	// Optional: border lines for detail
	draw->AddRect(pos, ImVec2(pos.x + width, pos.y + height), IM_COL32(0, 0, 0, 255), 2.0f);

	return clicked;
}

int ui_dipswitch_u8(const char* label, uint8_t* state, uint8_t enable_mask) {
	int r = 0;
	const int size = sizeof(*state) * 8;
	for (int i = 0; i < size; ++i) {
		PushID(i);
		bool enabled = enable_mask & (1 << i);
		int bit = (*state >> i) & 0x01;
		ui_begin_disabled(!enabled);
		if (ui_dipswitch(label, &bit)) {
			*state = (*state & ~(1 << i)) | (bit << i);
			r = 1;
		}
		ui_end_disabled();
		PopID();
		if (i < size - 1) {
			ui_same_line_spacing(0);
		}
	}
	return r;
}

int ui_dipswitch_u16(const char* label, uint16_t* state, uint16_t enable_mask) {
	int r = 0;
	const int size = sizeof(*state) * 8;
	for (int i = 0; i < size; ++i) {
		PushID(i);
		bool enabled = enable_mask & (1 << i);
		int bit = (*state >> i) & 0x01;
		ui_begin_disabled(!enabled);
		if (ui_dipswitch(label, &bit)) {
			*state = (*state & ~(1 << i)) | (bit << i);
			r = 1;
		}
		ui_end_disabled();
		PopID();
		if (i < size - 1) {
			ui_same_line_spacing(0);
		}
	}
	return r;
}

int ui_dipswitch_u32(const char* label, uint32_t* state, uint32_t enable_mask) {
	int r = 0;
	const int size = sizeof(*state) * 8;
	for (int i = 0; i < size; ++i) {
		PushID(i);
		bool enabled = enable_mask & (1 << i);
		int bit = (*state >> i) & 0x01;
		ui_begin_disabled(!enabled);
		if (ui_dipswitch(label, &bit)) {
			*state = (*state & ~(1 << i)) | (bit << i);
			r = 1;
		}
		ui_end_disabled();
		PopID();
		if (i < size - 1) {
			ui_same_line_spacing(0);
		}
	}
	return r;
}

int ui_text_input(const char* label, char* buffer, size_t buffer_len) {
	if (InputText(label, buffer, buffer_len, 0, NULL, NULL)) {
		return 1;
	}
	return 0;
}

void ui_push_id(int id) {
	PushID(id);
}

void ui_pop_id() {
	PopID();
}

int ui_draw_circle(const char* id, float radius, int segments, int selected) {
	ImVec2 pos = GetCursorScreenPos();
	ImDrawList* dl = GetWindowDrawList();
	
	ImGuiStyle& style = GetStyle();
	float line_h = GetTextLineHeight();     // height of text line
	float diameter = radius * 2.0f;
	float offset_y = (line_h - diameter) * 0.5f;
	ImVec2 center = { pos.x + radius, pos.y + offset_y + radius };
	
	ImU32 col = 0;

	ImVec2 mouse = GetIO().MousePos;
	float dx = mouse.x - center.x;
	float dy = mouse.y - center.y;
	bool inside = (dx * dx + dy * dy) <= (radius * radius);

	if (selected) {
		col = ColorConvertFloat4ToU32(style.Colors[ImGuiCol_ButtonActive]);
	}
	else if (inside) {
		col = ColorConvertFloat4ToU32(style.Colors[ImGuiCol_ButtonHovered]);
	}
	else {
		col = ColorConvertFloat4ToU32(style.Colors[ImGuiCol_Button]);
	}

	InvisibleButton(id, { radius * 2, radius * 2 });
	dl->AddCircleFilled(center, radius, col, segments);
	

	return inside && IsMouseClicked(ImGuiMouseButton_Left);
}

void ui_separator(void) {
	Separator();
}

void ui_set_tooltip(const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	SetTooltipV(fmt, args);
	va_end(args);
}
void ui_set_item_tooltip(const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	SetItemTooltipV(fmt, args);
	va_end(args);
}
void ui_begin_tooltip(void) {
	BeginTooltipEx(ImGuiTooltipFlags_OverridePrevious, ImGuiWindowFlags_None);
}
void ui_end_tooltip(void) {
	EndTooltip();
}
int ui_begin_item_tooltip(void) {
	if (IsItemHovered(ImGuiHoveredFlags_ForTooltip)) {
		BeginTooltipEx(ImGuiTooltipFlags_OverridePrevious, ImGuiWindowFlags_None);
		return 1;
	}
	return 0;
}
void ui_end_item_tooltip(void) {
	EndTooltip();
}
