/* input.c
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * Input
 */

#include <stdint.h>

#include "input.h"

static INPUT_RESET_CB reset_input_cb = NULL;
static INPUT_GET_CB get_input_cb = NULL;
static INPUT_SET_CB set_input_cb = NULL;
static INPUT_HAS_CB has_input_cb = NULL;

void input_set_cb_reset_input(INPUT_RESET_CB cb) {
	reset_input_cb = cb;
}

void input_set_cb_set_input(INPUT_SET_CB cb) {
	set_input_cb = cb;
}

void input_set_cb_get_input(INPUT_GET_CB cb) {
	get_input_cb = cb;
}

void input_set_cb_has_input(INPUT_HAS_CB cb) {
	has_input_cb = cb;
}

void input_reset_input(void) {
	reset_input_cb();
}

void input_set_input(uint8_t key) {
	set_input_cb(key);
}

uint8_t input_get_input(void) {
	return get_input_cb();
}

int input_has_input(void) {
	return has_input_cb();
}
