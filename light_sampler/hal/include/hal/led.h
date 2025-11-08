#ifndef _LED_H_
#define _LED_H_

#include <stdint.h>

#define PWM_PATH_DUTY_CYCLE "/dev/hat/pwm/GPIO12/duty_cycle"
#define PWM_PATH_PERIOD "/dev/hat/pwm/GPIO12/period"
#define PWM_PATH_ENABLE "/dev/hat/pwm/GPIO12/enable"
#define PWM_PATH "/dev/hat/pwm/GPIO12"
// LED control functions
void led_init();
//duty_cycle <= period
// 0 <= period <= 469,754,879
void led_set_parameters(uint32_t period_ns, uint32_t duty_cycle_ns);
void led_cleanup();
// move update led here
#endif