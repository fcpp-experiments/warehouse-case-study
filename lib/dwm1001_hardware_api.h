#ifndef DWM1001_HARDWARE_API_H_
#define DWM1001_HARDWARE_API_H_

#ifdef __cplusplus
extern "C"
{
#endif

    int button_is_pressed(void);

    void set_led(int led_on);

#ifdef __cplusplus
}
#endif

#endif // DWM1001_HARDWARE_API_H_
