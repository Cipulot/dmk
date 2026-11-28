#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include "keys.h"

static uint16_t state = 1;
static uint8_t system_id, modes[2];
static bool gpio[10], suspended;
static bool power_saving = true;
uint16_t layers_get_latched_state(void) { return state; }
uint16_t layers_get_state(void) { return state; }
void layers_set_state(uint16_t s) { state = s; }
uint16_t layers_get_default(void){return system_id?4:1;}
void layers_set_default(uint8_t i){state=(state & ~5u)|(1u<<i);system_id=i==2;}
uint8_t hhkb_get_system(void) { return system_id; }
void hhkb_set_system(uint8_t s) { system_id = s; }
uint8_t hhkb_get_led_mode(uint8_t i) { return modes[i]; }
void hal_gpio_put(int p, bool v) { gpio[p] = v; }
bool tud_suspended(void) { return suspended; }
bool tud_mounted(void) { return true; }
bool keyboard_power_saving_enabled(void) { return power_saving; }
#include "../../keyboards/hhkb/modules/indicators_system/indicators_system.c"

int main(void) {
 state=3;modes[0]=3;keyboard_platform_tick(500);assert(gpio[8]);
 system_id=1;modes[1]=4;keyboard_platform_tick(500);assert(gpio[9]);
 system_id=0;keyboard_platform_tick(500);assert(!gpio[9]);
 modes[0]=2;hhkb_hid_led_change(2);keyboard_platform_tick(500);assert(gpio[8]);
 suspended=true;keyboard_platform_tick(500);assert(!gpio[8] && !gpio[9]);
 return 0;
}
