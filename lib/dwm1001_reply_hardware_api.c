#include "dwm1001_hardware_api.h"

#include "dev/button-sensor.h"
#include "dev/leds.h"

#define VIBRATOR_PIN        29

int button_is_pressed(void) {
    return button_sensor.value(BUTTON_SENSOR_VALUE_STATE) == BUTTON_SENSOR_VALUE_PRESSED;
}

void set_led(int led_on) {
    if (led_on) {
        nrf_gpio_pin_set(VIBRATOR_PIN);
    } else {
        nrf_gpio_pin_clear(VIBRATOR_PIN);
    }
}
