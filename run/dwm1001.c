#include "contiki.h"
/*---------------------------------------------------------------------------*/
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#include "fcpp-runner.h"
#include "dwm1001_hardware_api.h"

PROCESS(app_process, "App");
AUTOSTART_PROCESSES(&app_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(app_process, ev, data) {
  static struct etimer et;
  static int i = 0;
  PROCESS_BEGIN();
  PROCESS_PAUSE();
  for (i = 0; i < 20; i++) {
    set_led((i + 1) % 2);
    etimer_set(&et, 0.2 * CLOCK_SECOND);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
  }
  start_fcpp();
  PROCESS_END();
}
