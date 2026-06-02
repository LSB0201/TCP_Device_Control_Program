#include <wiringPi.h>
#include <softPwm.h>

#include "hardware.h"

#define LED_PIN 1 // WiringPi 1번 (BCM 18)

void set_led_brightness(int duty_cycle) {
    static int init_flag = 0;

    // init_flag를 사용해 최초 한 번만 softPwm 초기화
    if(!init_flag)
    {
        pinMode(LED_PIN, OUTPUT);
        softPwmCreate(LED_PIN, 0, 100);
        init_flag = 1;
    }

    // 예외 처리
    if (duty_cycle < 0) {
        duty_cycle = 0;
    }
    
    if (duty_cycle > 100){
        duty_cycle = 100;
    }
    
    softPwmWrite(LED_PIN, duty_cycle);
}