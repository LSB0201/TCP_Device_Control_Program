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

// 노래 재생용 쓰래드 함수
void* buzzer_song_thread(void* arg) {
    (void)arg;
    current_exclusive_task = 1;
    
    // 노래 패턴
    for (int i = 0; i < 5; i++) {
        if (cancel_task_flag) break; // 즉시 종료 체크
        set_buzzer(1); delay(300);
        set_buzzer(0); delay(100);
    }
    
    set_buzzer(0); 
    if (current_exclusive_task == 1) current_exclusive_task = 0;
    return NULL;
}