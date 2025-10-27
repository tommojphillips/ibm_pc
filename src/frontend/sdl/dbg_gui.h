/* dbg_gui.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
*/

#ifndef DBG_GUI_H
#define DBG_GUI_H

#include <stdint.h>

typedef struct WINDOW_INSTANCE WINDOW_INSTANCE;

#define FRAME_HISTORY 60
typedef struct {
	double frame_times[FRAME_HISTORY]; // ms per frame
	int index;                         // current write position
	int count;                         // number of frames stored (<= FRAME_HISTORY)
	double sum;                        // rolling sum of frame times
} AVG_FRAME_TIMER;

typedef struct DBG_GUI {
	WINDOW_INSTANCE* win;
	char str[64];
	AVG_FRAME_TIMER emu_avg_fps;
	AVG_FRAME_TIMER win_avg_fps;
} DBG_GUI;

extern void dbg_gui_render(WINDOW_INSTANCE* instance, DBG_GUI* gui);

#endif
