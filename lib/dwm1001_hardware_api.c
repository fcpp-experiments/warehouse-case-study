#include "dwm1001_hardware_api.h"

#include "dev/button-sensor.h"
#include "dev/leds.h"

int button_is_pressed(void) {
    return button_sensor.value(BUTTON_SENSOR_VALUE_STATE) == BUTTON_SENSOR_VALUE_PRESSED;
}

void set_led(int led_on) {
    if (led_on) {
        leds_on(LEDS_1 | LEDS_2 | LEDS_3 | LEDS_4);
    } else {
        leds_off(LEDS_1 | LEDS_2 | LEDS_3 | LEDS_4);
    }
}
