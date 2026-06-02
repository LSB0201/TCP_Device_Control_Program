#include <wiringPi.h>

#include "hardware.h"

#define BUZZER_PIN 2 // WiringPi 2번 (BCM 27)

void set_buzzer(int state) {
    static int init_flag = 0;

    // init_flag를 사용해 최초 한 번만 softPwm 초기화
    if(!init_flag)
    {
        pinMode(BUZZER_PIN, OUTPUT);
        digitalWrite(BUZZER_PIN, LOW); // 초기 값 off
        init_flag = 1;
    }

    // state가 1이면 키고, 0이면 끄기
    digitalWrite(BUZZER_PIN, state ? HIGH : LOW);
}