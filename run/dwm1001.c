#include "contiki.h"
/*---------------------------------------------------------------------------*/
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#include "fcpp-runner.h"

PROCESS(app_process, "App");
AUTOSTART_PROCESSES(&app_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(app_process, ev, data) {
  PROCESS_BEGIN();
  PROCESS_PAUSE();
  start_fcpp();
  PROCESS_END();
}
