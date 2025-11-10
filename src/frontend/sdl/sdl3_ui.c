#include <stdio.h>

#include "SDL3/SDL.h"

#include "sdl3_window.h"
#include "sdl3_display.h"
#include "sdl3_keys.h"
#include "sdl3_ui.h"

#include "ui.h"

#include "backend/ibm_pc.h"
#include "backend/fdd/fdd.h"
#include "backend/utility/ring_buffer.h"
#include "frontend/utility/file.h"

static void insert_disk(FDD_DISK* fdd, const char* const* filelist, int filter) {
	(void)filter;
	if (*filelist == NULL) return;
	fdd_eject_disk(fdd);
	fdd_insert_disk(fdd, *filelist);
}
static void save_disk(FDD_DISK* fdd, const char* const* filelist, int filter) {
	(void)filter;
	if (*filelist == NULL) return;
	fdd_save_as_disk(fdd, *filelist);
}

static void draw_new_disk_submenu(FDD_DISK* fdd) {
	char str[32] = { 0 };
	for (uint32_t i = 0; i < disk_geometry_count; ++i) {
		sprintf(&str[0], "%d KB", disk_geometry[i].size / 1024);
		if (ui_menu_button(str, 0, 1)) {
			fdd_eject_disk(fdd);
			fdd_new_disk(fdd, disk_geometry[i].size);
		}
	}
}
static void draw_disk_submenu(WINDOW_INSTANCE* instance, int disk) {
	static SDL_DialogFileFilter filter[2] = {
		{.name = ".img", .pattern = "img" },
		{.name = "All Files", .pattern = "*" },
	};

	ui_begin_disabled(1);
	if (ibm_pc->fdc.fdd[disk].status.inserted) {
		ui_text("%s (%d KB)", file_get_filename(ibm_pc->fdc.fdd[disk].path), ibm_pc->fdc.fdd[disk].buffer_size / 1024);
	}
	else {
		ui_text("No Disk Inserted");
	}
	ui_end_disabled();
	if (ibm_pc->fdc.fdd[disk].status.dirty) {
		ui_same_line();
		ui_push_style_color(UI_COLOR_Text, 1, 0, 0, 1);
		ui_text("*");
		ui_pop_style_color(1);
	}

	if (ui_menu_button("Insert", 0, 1)) {
		SDL_ShowOpenFileDialog(insert_disk, &ibm_pc->fdc.fdd[disk], instance->window, &filter[0], 2, ibm_pc->fdc.fdd[disk].path, 0);
	}
	
	if (ui_menu_button("Eject", 0, ibm_pc->fdc.fdd[disk].status.inserted)) {
		fdd_eject_disk(&ibm_pc->fdc.fdd[disk]);
	}

	if (ui_menu_button("Save", 0, ibm_pc->fdc.fdd[disk].status.inserted && ibm_pc->fdc.fdd[disk].status.dirty)) {
		fdd_save_disk(&ibm_pc->fdc.fdd[disk]);
	}
	
	if (ui_menu_button("Save As..", 0, ibm_pc->fdc.fdd[disk].status.inserted)) {
		SDL_ShowSaveFileDialog(save_disk, &ibm_pc->fdc.fdd[disk], instance->window, &filter[0], 2, ibm_pc->fdc.fdd[disk].path);
	}

	ui_menu_checkbox_u8("Write Protect", &ibm_pc->fdc.fdd[disk].status.write_protect);
		
	if (ui_begin_menu("New")) {
		draw_new_disk_submenu(&ibm_pc->fdc.fdd[disk]);
		ui_end_menu();
	}

	ui_separator();

	ui_begin_disabled(1);
	ui_menu_checkbox_u8("Ready", &ibm_pc->fdc.fdd[disk].status.ready);
	ui_end_disabled();
}
static void draw_display_submenu(DISPLAY_INSTANCE* display) {
	int sel = 0;
	if (ui_begin_menu("Change Adapter")) {

		sel = ibm_pc->config.video_adapter == VIDEO_ADAPTER_MDA_80X25;
		if (ui_menu_button("MDA", sel, !sel)) {
			display_on_video_adapter_changed(display, VIDEO_ADAPTER_MDA_80X25);
			ibm_pc->config.video_adapter = VIDEO_ADAPTER_MDA_80X25;
		}

		sel = ibm_pc->config.video_adapter == VIDEO_ADAPTER_CGA_80X25 || ibm_pc->config.video_adapter == VIDEO_ADAPTER_CGA_40X25;
		if (ui_menu_button("CGA", sel, !sel)) {
			display_on_video_adapter_changed(display, VIDEO_ADAPTER_CGA_80X25);
			ibm_pc->config.video_adapter = VIDEO_ADAPTER_CGA_80X25;
		}

		sel = ibm_pc->config.video_adapter == VIDEO_ADAPTER_RESERVED;
		if (ui_menu_button("Extension", sel, !sel)) {
			display_on_video_adapter_changed(display, VIDEO_ADAPTER_RESERVED);
			ibm_pc->config.video_adapter = VIDEO_ADAPTER_RESERVED;
		}

		ui_end_menu();
	}
	
	if (ui_begin_menu("Texture Scale Mode")) {

		sel = display->config.texture_scale_mode == SDL_SCALEMODE_NEAREST;
		if (ui_menu_button("Nearest", sel, 1)) {
			display->config.texture_scale_mode = SDL_SCALEMODE_NEAREST;
		}

		sel = display->config.texture_scale_mode == SDL_SCALEMODE_LINEAR;
		if (ui_menu_button("Linear", sel, 1)) {
			display->config.texture_scale_mode = SDL_SCALEMODE_LINEAR;
		}

		ui_end_menu();
	}

	if (ui_begin_menu("Display Scale Mode")) {

		sel = display->config.display_scale_mode == DISPLAY_SCALE_FIT;
		if (ui_menu_button("Fit", sel, 1)) {
			display->config.display_scale_mode = DISPLAY_SCALE_FIT;
		}

		sel = display->config.display_scale_mode == DISPLAY_SCALE_STRETCH;
		if (ui_menu_button("Stretch", sel, 1)) {
			display->config.display_scale_mode = DISPLAY_SCALE_STRETCH;
		}

		ui_end_menu();
	}

	if (ui_begin_menu("Display View Mode")) {

		sel = display->config.display_view_mode == DISPLAY_VIEW_CROPPED;
		if (ui_menu_button("Cropped", sel, 1)) {
			display->config.display_view_mode = DISPLAY_VIEW_CROPPED;
		}

		sel = display->config.display_view_mode == DISPLAY_VIEW_FULL;
		if (ui_menu_button("Full", sel, 1)) {
			display->config.display_view_mode = DISPLAY_VIEW_FULL;
		}

		ui_end_menu();
	}

	ui_menu_checkbox("Scanline Emulation", &display->config.scanline_emu);

	sel = display->config.display_scale_mode == DISPLAY_SCALE_FIT;
	if (ui_menu_button("Correct Aspect Ratio", display->config.correct_aspect_ratio, sel)) {
		display->config.correct_aspect_ratio ^= 1;
	}
	
	sel = window_instance_is_full_screen(display->window);
	if (ui_menu_button("Full Screen", sel, 1)) {
		window_instance_toggle_full_screen(display->window);
	}
}
static void draw_dipswitch_submenu(void) {

	uint8_t sw1_mask = 0;
	uint8_t sw1_dp_mask = 0;
	uint8_t sw2_mask = 0;
	uint8_t sw2_dp_mask = 0;
	
	uint20_t planar_ram_max = 0;
	uint20_t total_ram_max = 0;
	
	uint20_t total_ram_min = 0;

	uint20_t ram_inc_below_planar_max = 0;
	uint20_t ram_inc_above_planar_max = 0;

	char str[32] = { 0 };	

	uint8_t sw = 0;

	switch (ibm_pc->config.model) {
		case MODEL_5150_16_64:
			sw1_mask = 0xFF;
			sw2_mask = 0x0F;
			planar_ram_max = 64;
			total_ram_max = 736;
			total_ram_min = 16; 
			ram_inc_below_planar_max = 16;
			ram_inc_above_planar_max = 32;
			if (ui_menu_button("Model: IBM 5150 16KB-64KB ", 0, 1)) {
				ibm_pc->config.model = MODEL_5150_64_256;
				ibm_pc_set_config();
			}
			break;

		case MODEL_5150_64_256:
			sw1_mask = 0xFF;
			sw2_mask = 0x1F;
			planar_ram_max = 256;
			total_ram_max = 736;
			total_ram_min = 64;
			ram_inc_below_planar_max = 64;
			ram_inc_above_planar_max = 32;
			if (ui_menu_button("Model: IBM 5150 64KB-256KB", 0, 1)) {
				ibm_pc->config.model = MODEL_5150_16_64;
				ibm_pc_set_config();
			}
			break;
	}

	if (ibm_pc->config.sw1_provided) {
		sw1_dp_mask = sw1_mask; /* Enable sw1 dipswitch */
	}
	else {
		sw1_dp_mask = 0; /* Disable sw1 dipswitch */
	}

	if (ibm_pc->config.sw2_provided) {
		sw2_dp_mask = sw2_mask; /* Enable sw2 dipswitch */
	}
	else {
		sw2_dp_mask = 0; /* Disable sw2 dipswitch */
	}

	ui_separator();
	
	ui_button("SW1: ");
	ui_same_line_spacing(0);
	sw = ~ibm_pc->config.sw1; /* invert sw like on planar */
	if (ui_dipswitch_u8("##sw1_dp", &sw, sw1_dp_mask)) {
		ibm_pc->config.sw1 = ~sw; /* invert sw back if changed */
		ibm_pc_set_config();
	}
	ui_same_line_spacing(0);
	if (ibm_pc->config.sw1_provided) {
		if (ui_button("Manual##dp1")) {
			ibm_pc->config.sw1_provided = 0;
		}
	}
	else {
		if (ui_button("Auto##dp1")) {
			ibm_pc->config.sw1_provided = 1;
		}
	}

	ui_button("SW2: ");
	ui_same_line_spacing(0);
	sw = ~ibm_pc->config.sw2; /* invert sw like on planar */
	if (ui_dipswitch_u8("##sw2_dp", &sw, sw2_dp_mask)) {
		ibm_pc->config.sw2 = ~sw; /* invert sw back if changed */
		ibm_pc_set_config();
	}
	ui_same_line_spacing(0);
	if (ibm_pc->config.sw2_provided) {
		if (ui_button("Manual##dp2")) {
			ibm_pc->config.sw2_provided = 0;
		}
	}
	else {
		if (ui_button("Auto##dp2")) {
			ibm_pc->config.sw2_provided = 1;
		}
	}

	ui_separator();
	
	if (!ibm_pc->config.sw1_provided) {

		if (ui_begin_menu("Adapter")) {

			int sel = (ibm_pc->config.sw1 & SW1_DISPLAY_MASK) == SW1_DISPLAY_MDA_80X25;
			if (ui_menu_button("MDA", sel, !sel)) {
				ibm_pc->config.sw1 &= ~SW1_DISPLAY_MASK;
				ibm_pc->config.sw1 |= SW1_DISPLAY_MDA_80X25;
			}

			sel = (ibm_pc->config.sw1 & SW1_DISPLAY_MASK) == SW1_DISPLAY_CGA_80X25;
			if (ui_menu_button("CGA 80", sel, !sel)) {
				ibm_pc->config.sw1 &= ~SW1_DISPLAY_MASK;
				ibm_pc->config.sw1 |= SW1_DISPLAY_CGA_80X25;
			}

			sel = (ibm_pc->config.sw1 & SW1_DISPLAY_MASK) == SW1_DISPLAY_CGA_40X25;
			if (ui_menu_button("CGA 40", sel, !sel)) {
				ibm_pc->config.sw1 &= ~SW1_DISPLAY_MASK;
				ibm_pc->config.sw1 |= SW1_DISPLAY_CGA_40X25;
			}

			sel = (ibm_pc->config.sw1 & SW1_DISPLAY_MASK) == SW1_DISPLAY_RESERVED;
			if (ui_menu_button("Extension", sel, !sel)) {
				ibm_pc->config.sw1 &= ~SW1_DISPLAY_MASK;
				ibm_pc->config.sw1 |= SW1_DISPLAY_RESERVED;
			}

			ui_end_menu();
		}

		if (ui_begin_menu("Floppy drives")) {

			int sel = (ibm_pc->config.sw1 & SW1_HAS_FDC) == 0;
			if (ui_menu_button("0##floppy_drives", sel, !sel)) {
				ibm_pc->config.sw1 &= ~SW1_HAS_FDC;
				ibm_pc->config.sw1 &= ~SW1_DISKS_MASK;
				ibm_pc_set_config();
			}

			for (uint8_t k = 1; k <= 4; ++k) {
				sel = (ibm_pc->config.sw1 & SW1_HAS_FDC) == SW1_HAS_FDC && (ibm_pc->config.sw1 & SW1_DISKS_MASK) == (k - 1) << 6;
				sprintf(&str[0], "%d##floppy_drives", k);
				if (ui_menu_button(str, sel, !sel)) {
					ibm_pc->config.sw1 |= SW1_HAS_FDC;
					ibm_pc->config.sw1 &= ~SW1_DISKS_MASK;
					ibm_pc->config.sw1 |= (k - 1) << 6;
					ibm_pc_set_config();
				}
			}
			ui_end_menu();
		}

		if (ui_begin_menu("Planar RAM")) {
			for (uint20_t k = total_ram_min; k <= planar_ram_max; k += ram_inc_below_planar_max) {
				sprintf(&str[0], "%u KB##planar_ram", k);
				int sel = k * 1024 == determine_planar_ram_size(ibm_pc->config.sw1);
				if (ui_menu_button(str, sel, !sel)) {
					ibm_pc->config.sw1 &= ~SW1_MEMORY_MASK;
					ibm_pc->config.sw1 |= determine_planar_ram_sw(k * 1024);

					/* IO RAM should only be set if planar RAM >= 64 */
					if (k <= planar_ram_max) {
						uint20_t planar_ram = determine_planar_ram_size(ibm_pc->config.sw1);
						ibm_pc->config.sw2 = determine_io_ram_sw(planar_ram, 0);
					}

					ibm_pc_set_config();
				}
			}

			ui_end_menu();
		}
	}

	if (!ibm_pc->config.sw2_provided) {

		if (ui_begin_menu("IO RAM")) {
			for (uint20_t k = 0; k <= total_ram_max - planar_ram_max; k += 32) {
				sprintf(&str[0], "%u KB##io_ram", k);
				int sel = k * 1024 == determine_io_ram_size(ibm_pc->config.sw1, ibm_pc->config.sw2);
				if (ui_menu_button(str, sel, !sel)) {
					uint20_t planar_ram = determine_planar_ram_size(ibm_pc->config.sw1);
					ibm_pc->config.sw2 = determine_io_ram_sw(planar_ram, k * 1024);

					/* Planar RAM should be set to 4 Banks if IO RAM > 0 */
					if (k > 0) {
						ibm_pc->config.sw1 |= SW1_MEMORY_64K;
					}
					ibm_pc_set_config();
				}
			}

			ui_end_menu();
		}
	}

	if (!ibm_pc->config.sw1_provided && !ibm_pc->config.sw2_provided) {
		if (ui_begin_menu("Total RAM")) {
			for (uint20_t k = total_ram_min; k <= total_ram_max;) {
				sprintf(&str[0], "%u KB##total_ram", k);
				int sel = k * 1024 == determine_planar_ram_size(ibm_pc->config.sw1) + determine_io_ram_size(ibm_pc->config.sw1, ibm_pc->config.sw2);
				if (ui_menu_button(str, sel, !sel)) {

					ibm_pc->config.sw1 &= ~SW1_MEMORY_MASK;
					if (k >= planar_ram_max) {
						ibm_pc->config.sw1 |= determine_planar_ram_sw(planar_ram_max * 1024);
						ibm_pc->config.sw2 = determine_io_ram_sw(planar_ram_max * 1024, (k - planar_ram_max) * 1024);
					}
					else {
						ibm_pc->config.sw1 |= determine_planar_ram_sw(k * 1024);
						ibm_pc->config.sw2 = determine_io_ram_sw(k * 1024, 0);
					}
					ibm_pc_set_config();
				}

				if (k >= planar_ram_max) {
					k += ram_inc_above_planar_max;
				}
				else {
					k += ram_inc_below_planar_max;
				}
			}
			ui_end_menu();
		}
	}

	if (!ibm_pc->config.sw1_provided || !ibm_pc->config.sw2_provided) {		
		ui_separator();
	}
		
	ui_text("Has FPU:    %s", (ibm_pc->config.sw1 & SW1_HAS_FPU) == SW1_HAS_FPU ? "Yes" : "No");
	ui_separator();
	
	ui_text("Adapter:    %s", (ibm_pc->config.sw1 & SW1_DISPLAY_MASK) == SW1_DISPLAY_MDA_80X25 ? "MDA" : (ibm_pc->config.sw1 & SW1_DISPLAY_MASK) == SW1_DISPLAY_CGA_80X25 ? "CGA 80" : (ibm_pc->config.sw1 & SW1_DISPLAY_MASK) == SW1_DISPLAY_CGA_40X25 ? "CGA 40" : "Extension");
	ui_separator();
	
	if ((ibm_pc->config.sw1 & SW1_HAS_FDC) == SW1_HAS_FDC) {
		ui_text("Has FDC:    Yes");
		ui_text("Num Disks:  %s", (ibm_pc->config.sw1 & SW1_DISKS_MASK) == SW1_DISKS_1 ? "1" : (ibm_pc->config.sw1 & SW1_DISKS_MASK) == SW1_DISKS_2 ? "2" : (ibm_pc->config.sw1 & SW1_DISKS_MASK) == SW1_DISKS_3 ? "3" : "4");
	}
	else {
		ui_text("Has FDC:    No");
	}
	ui_separator();
	
	uint20_t io_ram = determine_io_ram_size(ibm_pc->config.sw1, ibm_pc->config.sw2) / 1024;
	uint20_t planar_ram = determine_planar_ram_size(ibm_pc->config.sw1) / 1024;

	ui_text("Planar RAM: %u KB", planar_ram);
	ui_text("IO RAM:     %u KB", io_ram);
	ui_text("Total RAM:  %u KB", planar_ram + io_ram);
}

static void draw_main_menu(UI_CONTEXT* ui_context, DISPLAY_INSTANCE* display) {
	display->offset_y = 0;

	// Auto-hiding main menu bar with slide animation

	// Menu bar height
	float menu_height = ui_get_text_line_height() + (ui_get_frame_padding().y * 2);

	float mx, my;
	SDL_GetGlobalMouseState(&mx, &my);
	WINDOW_TRANSFORM t = display->window->transform;

	SDL_WindowFlags flags = SDL_GetWindowFlags(display->window->window);
	
	int inside_window = mx >= t.x && mx < t.x + t.w && my >= t.y - 5 && my < t.y + t.h;
	int near_top = (my >= t.y - 5 && my <= t.y + menu_height + 5);
	int focused = (flags & SDL_WINDOW_INPUT_FOCUS) != 0;
	int hovering = inside_window && near_top && focused;

	// Keep visible if any menu popup is open
	int menu_open = ui_is_popup_open(NULL);

	// Target visibility
	float target = (hovering || menu_open) ? 1.0f : 0.0f;
	ui_context->menu_slide = ui_lerp(ui_context->menu_slide, target, ui_get_delta_time() * 7.0f);

	if (ui_context->menu_slide <= 0.005f) {
		ui_context->menu_slide = 0.0f;
		ui_context->slide_offset = -menu_height;
		// Exit early if menu slide is 0.
		return;
	}
	
	// Apply slide offset from the LAST frame.
	ui_set_next_window_position(0, ui_context->slide_offset);
	ui_set_next_window_size((float)display->window->transform.w, menu_height);

	// We compute the slide offset for the NEXT frame. Delay by 1 frame so the display has time to catch up.

	// Compute vertical offset for slide-out;
	ui_context->slide_offset = (-menu_height * (1.0f - ui_context->menu_slide));
	
	// Compute display offset height
	display->offset_y = menu_height + ui_context->slide_offset;

	ui_push_style_color(UI_COLOR_Button, 0, 0, 0, 0);
	ui_push_style_color(UI_COLOR_FrameBg, 0, 0, 0, .25f);

	UI_WINDOW_FLAGS menu_flags =
		UI_WINDOW_FLAGS_NoTitleBar | UI_WINDOW_FLAGS_NoResize | UI_WINDOW_FLAGS_NoMove | UI_WINDOW_FLAGS_NoScrollbar |
		UI_WINDOW_FLAGS_NoSavedSettings | UI_WINDOW_FLAGS_MenuBar | UI_WINDOW_FLAGS_NoBackground;

	ui_begin("##MainMenuBar", NULL, menu_flags);

	if (ui_begin_menu_bar()) {

		if (ui_begin_menu("Machine")) {
			if (ui_menu_item("Restart")) {
				ibm_pc_reset();
			}
			if (ui_menu_item("Ctrl-Alt-Del")) {
				ring_buffer_push(&ibm_pc->kbd.key_buffer, pc_scancode[SDL_SCANCODE_LCTRL]);
				ring_buffer_push(&ibm_pc->kbd.key_buffer, pc_scancode[SDL_SCANCODE_LALT]);
				ring_buffer_push(&ibm_pc->kbd.key_buffer, pc_scancode[SDL_SCANCODE_DELETE]);
			}
			if (ui_menu_item("Exit")) {
				window_instance_close(display->window);
				window_instance_destroy(display->window);
			}
			ui_end_menu();
		}

		if (ui_begin_menu("Disk")) {
			if (ibm_pc->config.fdc_disks == 0) {
				ui_text("No Disks");
			}
			if (ibm_pc->config.fdc_disks >= 1) {
				if (ui_begin_menu("A:")) {
					draw_disk_submenu(display->window, 0);
					ui_end_menu();
				}
			}
			if (ibm_pc->config.fdc_disks >= 2) {
				if (ui_begin_menu("B:")) {
					draw_disk_submenu(display->window, 1);
					ui_end_menu();
				}
			}
			if (ibm_pc->config.fdc_disks >= 3) {
				if (ui_begin_menu("C:")) {
					draw_disk_submenu(display->window, 2);
					ui_end_menu();
				}
			}
			if (ibm_pc->config.fdc_disks >= 4) {
				if (ui_begin_menu("D:")) {
					draw_disk_submenu(display->window, 3);
					ui_end_menu();
				}
			}

			ui_end_menu();
		}

		if (ui_begin_menu("Display")) {
			draw_display_submenu(display);
			ui_end_menu();
		}

		if (ui_begin_menu("Dip Switches")) {
			draw_dipswitch_submenu();
			ui_end_menu();
		}

		ui_end_menu_bar();
	}

	ui_end();

	ui_pop_style_color(2);
}

void ui_update(UI_CONTEXT* ui_context, DISPLAY_INSTANCE* display) {
	ui_new_frame();
	draw_main_menu(ui_context, display);
	ui_render();
}
