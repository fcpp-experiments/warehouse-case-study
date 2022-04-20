#include "contiki.h"
/*---------------------------------------------------------------------------*/
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#include "fcpp-runner.h"
#include "dwm1001_hardware_api.h"

#if REPLY_PLATFORM == 1

#include "flash_mem_manager.h"

#define VIBRATOR_PIN        29
#define SW_POWER_OFF_PIN    22
#define LED2_PIN            30

#endif

PROCESS(app_process, "App");
AUTOSTART_PROCESSES(&app_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(app_process, ev, data) {
  static struct etimer et;
  static int i = 0;
  PROCESS_BEGIN();
  PROCESS_PAUSE();
#if REPLY_PLATFORM == 1
	nrf_gpio_cfg_output(SW_POWER_OFF_PIN);
	nrf_gpio_pin_set(SW_POWER_OFF_PIN);

	nrf_gpio_cfg_output(LED2_PIN);
	nrf_gpio_pin_clear(LED2_PIN);

	nrf_gpio_cfg_output(VIBRATOR_PIN);
	nrf_gpio_pin_clear(VIBRATOR_PIN);
#endif
  for (i = 0; i < 20; i++) {
    set_led((i + 1) % 2);
    etimer_set(&et, 0.2 * CLOCK_SECOND);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
  }
  start_fcpp();
  PROCESS_END();
}
