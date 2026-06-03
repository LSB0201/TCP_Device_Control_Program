#include <wiringPi.h>

#include "common.h"
#include "hardware.h"

// BCD 입력을 위한 4개의 핀 배열 (WiringPi 핀 번호 기준)
const int BCD_PINS[4] = {3, 4, 5, 6};

void display_7segment(int number) {
    static int initialized = 0;
    
    // 최초 1회 핀 모드 초기화 (4개 핀 모두 OUTPUT 설정)
    if (!initialized) {
        for (int i = 0; i < 4; i++) {
            pinMode(BCD_PINS[i], OUTPUT);
            digitalWrite(BCD_PINS[i], LOW);
        }
        initialized = 1;
    }

    // 정상 출력을 위한 예외 처리
    if (number < 0 || number > 9) return; 

    // 비트 마스킹을 이용하여 10진수 숫자를 4비트 2진수로 분리하여 출력
    for (int i = 0; i < 4; i++) {
        int bit_value = (number >> i) & 1; // number의 i번째 자리 비트값을 추출
        digitalWrite(BCD_PINS[i], bit_value);
    }
}

void* seg_countdown_thread(void* arg) {
    int start_num = *(int*)arg;
    free(arg); // 동적 할당 해제
    current_exclusive_task = 2;

    for (int i = start_num; i >= 0; i--) {
        if (cancel_task_flag) break;
        display_7segment(i);
        
        for (int j = 0; j < 10; j++) {
            if (cancel_task_flag) break;
            delay(100);
        }
    }

   display_7segment(0);

    // 정상 종료 시 부저 1초 울림
    if (!cancel_task_flag) {
        if (hw.set_buzzer) hw.set_buzzer(1); 
        delay(1000);
        if (hw.set_buzzer) hw.set_buzzer(0);
    }

    if (current_exclusive_task == 2) current_exclusive_task = 0;

    return NULL;
}