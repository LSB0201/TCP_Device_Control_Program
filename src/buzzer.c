#include <wiringPi.h>

#include "common.h"

#define BUZZER_PIN 2 // WiringPi 2번 (BCM 27)

int notes[] = {
    392, 392, 440, 440, 392, 392, 330, 0,
    392, 392, 330, 330, 294, 0,
    392, 392, 440, 440, 392, 392, 330, 0,
    392, 330, 294, 330, 262, 0
};

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
    int total_notes = sizeof(notes) / sizeof(notes[0]);

    // 초기화 보장
    set_buzzer(0); 

    // 노래 배열 루프
    for (int i = 0; i < total_notes; i++) {
        // OFF 또는 전체 종료를 누르면 즉시 루프 탈출
        if (cancel_task_flag) break; 

        if (notes[i] == 0) {
            softToneWrite(BUZZER_PIN, 0);
            delay(200); // 쉼표 대기
        } else {
            softToneWrite(BUZZER_PIN, notes[i]); // 해당 주파수(계이름) 출력
            delay(280); // 음의 길이 대기
            
            // 음과 음 사이 구분을 위한 정지
            softToneWrite(BUZZER_PIN, 0);
            delay(20); 
        }
    }
    
    // 노래가 끝났거나 중단되었을 때 부저 소리 끄기
    softToneWrite(BUZZER_PIN, 0); 

    // 작업이 자신이 실행 중인 상태로 끝났다면 플래그 초기화
    if (current_exclusive_task == 1) {
        current_exclusive_task = 0;
    }
    
    return NULL;
}