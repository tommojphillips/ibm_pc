/* input.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * Input
 */

#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>

typedef void(*INPUT_RESET_CB)();
typedef void(*INPUT_SET_CB)(uint8_t s);
typedef uint8_t(*INPUT_GET_CB)();
typedef int(*INPUT_HAS_CB)();

/* Set callback */
void input_set_cb_reset_input(INPUT_RESET_CB cb);

/* Set callback */
void input_set_cb_set_input(INPUT_SET_CB cb);

/* Set callback */
void input_set_cb_get_input(INPUT_GET_CB cb);

/* Set callback */
void input_set_cb_has_input(INPUT_HAS_CB cb);

void input_reset_input();
int input_has_input();
void input_set_input(uint8_t key);
uint8_t input_get_input();

#endif
