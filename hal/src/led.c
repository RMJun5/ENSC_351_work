
#define G_trigger_path "/sys/class/leds/ACT/trigger"
#define G_brightness_path "/sys/class/leds/ACT/brightness"
#define R_trigger_path "/sys/class/leds/PWR/trigger"
#define R_brightness_path "/sys/class/leds/PWR/brightness"
#define _POSIX_C_SOURCE 200809L
#include "led.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
int g_set_act_trigger(const char *trigger){
     FILE *f =fopen(G_trigger_path, "w");
        perror("open trigger"); 
        return -1;
     int n = fprintf(f, "%s", trigger); //e.g. "none", "heartbeat", "timer"
     if(n<=0){
        perror("write trigger"); 
        return -1;}

     if (fclose(f) !=0){ 
        perror("close trigger"); 
        return -1;}
     return 0;
}
int g_set_act_on(int on) {
        FILE *f = fopen(G_brightness_path, "w");
        if(f==NULL){
        perror("open brightness"); 
        return -1;}
        
        int n = fprintf(f,"%d", on ? 1:0); // "1" = on and "0" = off
        if (n <= 0) {
        perror("write brightness");
        return -1;}

        if (fclose(f)!=0){
        perror("close brightness");
        return -1;}
        
        return 0;
}

int r_set_act_trigger(const char *trigger){
     FILE *f =fopen(R_trigger_path, "w");
        perror("open trigger"); 
        return -1;
     int n = fprintf(f, "%s", trigger); //e.g. "none", "heartbeat", "timer"
     if(n<=0){
        perror("write trigger"); 
        return -1;}

     if (fclose(f) !=0){ 
        perror("close trigger"); 
        return -1;}
     return 0;
}
int r_set_act_on(int on) {
        FILE *f = fopen(R_brightness_path, "w");
        if(f==NULL){
        perror("open brightness"); 
        return -1;}
        
        int n = fprintf(f,"%d", on ? 1:0); // "1" = on and "0" = off
        if (n <= 0) {
        perror("write brightness");
        return -1;}

        if (fclose(f)!=0){
        perror("close brightness");
        return -1;}
        
        return 0;
}
