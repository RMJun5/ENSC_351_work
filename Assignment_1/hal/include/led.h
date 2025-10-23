#pragma once

#define G_trigger_path     "/sys/class/leds/ACT/trigger"
#define G_brightness_path  "/sys/class/leds/ACT/brightness"
#define R_trigger_path "/sys/class/leds/PWR/trigger"
#define R_brightness_path "/sys/class/leds/PWR/brightness"

int g_led_set_trigger(const char *led_name, const char *trigger); 
int r_led_set_trigger(const char *led_name, const char *trigger); 
//"none", "heartbeat", "timer"
int g_led_set_brightness(const char *led_name, int power); //"0" or "1"
int r_led_set_brightness(const char *led_name, int power); //"0" or "1"
int g_led_set_timer(const char *led_name, int delay_on_ms, int delay_off_ms);
int r_led_set_timer(const char *led_name, int delay_on_ms, int delay_off_ms);
//requires trigger = timer
int g_set_act_trigger(const char *trigger);   // e.g., "none", "heartbeat", "time>
int r_set_act_trigger(const char *trigger);   // e.g., "none", "heartbeat", "time>
int g_set_act_on(int on);                     // 1 = on, 0 = o
int r_set_act_on(int on);                     // 1 = on, 0 = o