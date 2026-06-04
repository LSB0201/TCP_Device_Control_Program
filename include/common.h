#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#define DEFAULT_PORT 8080
#define MAX_EVENTS 10
#define BUFFER_SIZE 1024

// 하드웨어 상태 및 센서 데이터를 저장(공유 구조체)
typedef struct {
    int light_intensity;    // 조도 센서 값
    double temperature;     // 온도 센서 값
    int led_brightness;     // LED 현재 밝기 (0~255)
    int buzzer_status;      // 부저 상태 (0: OFF, 1: ON)
    int current_seg_num;    // 7세그먼트 현재 출력 숫자
    int auto_led_mode;      // 조도 센서를 통한 LED 제어 모드 현황
    pthread_mutex_t mutex;  // 쓰레드 동기화를 위한 뮤텍스
} DeviceState;

// 노래와 카운트 다운, 배타적 실행 제어를 위한 전역 변수
extern volatile int current_exclusive_task; // 0: 없음, 1: 부저 노래, 2: 세그먼트 카운트다운
extern volatile int cancel_task_flag;       // 1이 되면 현재 실행중인 쓰레드가 루프를 탈출하여 즉시 종료

#endif // COMMON_H