#include "FreeRTOS.h"
#include "task.h"
#include "hal_gpio.h"
#include "hooks.h"
#include "keyboard.h"
#include "layers.h"
#include "classic_matrix.h"
#ifdef HHKB_HYBRID
#include "hybrid.h"
#endif
#include "tusb.h"
#include <stdbool.h>
#include <stdint.h>

/* One owner for PA8 and PA9: off, heartbeat, Caps Lock, Fn, DF(2). */
static const pin_t led_pins[2] = { A8, A9 };
static bool caps_lock;

static bool indicator_inputs_changed(void) {
    static uint16_t previous=UINT16_MAX;
    uint16_t state=(layers_get_state() & 15) | (hhkb_get_led_mode(0)<<4) |
        (hhkb_get_led_mode(1)<<7) | (caps_lock<<10) | (tud_mounted()<<11) |
        (tud_suspended()<<12) | (keyboard_power_saving_enabled()<<13) |
        ((layers_get_default() & 4)<<12);
    bool changed=state!=previous; previous=state; return changed;
}
TickType_t keyboard_platform_timeout(void) {
#ifdef HHKB_HYBRID
    return pdMS_TO_TICKS(hhkb_hybrid_indicator_wait_ms(xTaskGetTickCount()));
#else
    return pdMS_TO_TICKS(250);
#endif
}
static TickType_t indicator_updated;
static bool indicator_valid;
void keyboard_platform_tick(TickType_t now) {
    bool changed=indicator_inputs_changed();
    if(indicator_valid && !changed &&
       (TickType_t)(now-indicator_updated)<keyboard_platform_timeout()) return;
    indicator_valid=true; indicator_updated=now;
    bool heartbeat = ((now / pdMS_TO_TICKS(500)) & 1U) != 0;
    bool mounted = tud_mounted();
    bool suspended = tud_suspended();
    bool power_saving_active = keyboard_power_saving_enabled() && (!mounted || suspended);
#ifdef HHKB_HYBRID
    bool battery = hhkb_hybrid_on_battery();
    if (battery || hhkb_hybrid_output_bluetooth()) power_saving_active = false;
    if (!hhkb_hybrid_powered()) power_saving_active = false;
#endif
    for (uint8_t i = 0; i < 2; i++) {
        bool on = false;
        if (power_saving_active) {
            on = false;
        }
#ifdef HHKB_HYBRID
        else if (hhkb_hybrid_led_override(i, now, &on)) {
            // Hybrid status has priority over the configurable default mode.
        }
#endif
        else {
            switch (hhkb_get_led_mode(i)) {
            case 1: on = heartbeat; break;
            case 2: on = caps_lock; break;
            case 3: on = (layers_get_state() & ((1U << 1) | (1U << 3))) != 0; break;
            case 4: on = (layers_get_default() & (1U << 2)) != 0; break;
            default: break;
            }
        }
        hal_gpio_put(led_pins[i], on && !suspended && !power_saving_active);
    }
}

#ifdef HHKB_HYBRID
void hhkb_hybrid_refresh_indicators(void) { indicator_valid=false; keyboard_platform_tick(xTaskGetTickCount()); }
#endif

static void hhkb_hid_led_change(uint8_t state) { caps_lock = (state & 0x02) != 0; }
DMK_HOOK_SUBSCRIBER(hhkb_indicator_hooks,
    .hid_led_change = hhkb_hid_led_change
);
