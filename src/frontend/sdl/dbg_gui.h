/* dbg_gui.h
* GitHub: https:\\github.com\tommojphillips
*/

#ifndef DBG_GUI_H
#define DBG_GUI_H

#include <stdint.h>

typedef struct WINDOW_INSTANCE WINDOW_INSTANCE;

typedef struct DBG_GUI {
	WINDOW_INSTANCE* win;
	char str[64];
} DBG_GUI;

extern void dbg_gui_render(WINDOW_INSTANCE* instance, DBG_GUI* gui);

#endif
