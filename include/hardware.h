#ifndef HARDWARE_H
#define HARDWARE_H

#include "common.h"

// 공통 하드웨어 초기화 및 해제
int init_hardware(void);         // WiringPi 및 핀 초기화
void cleanup_hardware(void);     // 종료 시 모든 장치 OFF 처리

// 출력 장치 제어
void set_led_brightness(int duty_cycle); // LED 밝기 제어 (0 ~ 100)
void set_buzzer(int state);              // 부저 제어 (1: 켬, 0: 끔)
void display_7segment(int number);       // 7세그먼트 숫자 출력 (0~9)

// 입력 장치 제어 (I2C PCF8591)
// 센서 데이터를 주기적으로 읽어 state 구조체에 저장하는 쓰레드 함수
void* i2c_sensor_thread(void* arg);

#endif // HARDWARE_H