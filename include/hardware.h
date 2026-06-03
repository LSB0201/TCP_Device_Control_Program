#ifndef HARDWARE_H
#define HARDWARE_H

#include "common.h"

// 종료 시 모든 장치 OFF
void cleanup_hardware(void);

// 출력 장치 제어
void set_led_brightness(int duty_cycle); // LED 밝기 제어 (0 ~ 100)
void set_buzzer(int state);              // 부저 제어 (1: 켬, 0: 끔)
void display_7segment(int number);       // 7세그먼트 숫자 출력 (0~9)

// 입력 장치 제어 (I2C PCF8591)
// 센서 데이터를 주기적으로 읽어 state 구조체에 저장하는 쓰레드 함수
void* i2c_sensor_thread(void* arg);

// 부저를 다루는 기능에 대한 배타적 실행을 담당하는 쓰래드
void* buzzer_song_thread(void* arg);
void* seg_countdown_thread(void* arg);

// 하드웨어 제어 함수 포인터들을 담을 구조체
typedef struct {
    void (*set_led_brightness)(int);
    void (*set_buzzer)(int);
    void (*display_7segment)(int);
    void* (*i2c_sensor_thread)(void*);
    void* (*buzzer_song_thread)(void*);
    void* (*seg_countdown_thread)(void*);
} HardwareAPI;

extern HardwareAPI hw; // 전역 구조체 선언, main.c에서 초기화

// 동적 로딩 관련 함수
int load_hardware_library(void);
void cleanup_hardware(void);

#endif // HARDWARE_H