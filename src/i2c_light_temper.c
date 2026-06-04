#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <math.h>
#include <stdio.h>

#include "common.h"

#define P_ADDR 0x48
#define CH_LIGHT 0
#define CH_TEMP 1

static int read_pcf_adc(int fd, int channel) {
    wiringPiI2CWrite(fd, channel);

    wiringPiI2CRead(fd); // 쓰래기 값 버리기

    return wiringPiI2CRead(fd); // 실제 데이터 반환
}

// 섭씨온도 변환 함수 추가
static double convert_to_celsius(int raw_adc) {
    // 예외 처리 (0이나 255일 때 분모가 0이 되는 것 방지)
    if (raw_adc <= 0) return -273.15; 
    if (raw_adc >= 255) return 125.0;

    // 서미스터의 현재 저항값(Rt) 계산
    double R1 = 10000.0; // 10k 옴
    double Rt = R1 * ((double)raw_adc / (255.0 - (double)raw_adc));

    // Steinhart-Hart 방정식을 이용해 켈빈(K) 온도 계산
    double T0 = 298.15;  // 기준 온도 (25도 = 298.15K)
    double B = 3950.0;   // NTC 서미스터 일반적인 베타(B) 상수
    double R0 = 100000.0; // 25도일 때 서미스터의 저항 (10k 옴)

    double kelvin = 1.0 / (1.0 / T0 + (1.0 / B) * log(Rt / R0));
    
    // 켈빈 온도를 섭씨온도로 변환
    return kelvin - 273.15;
}

void* i2c_sensor_thread(void* arg) {
    DeviceState* state = (DeviceState*)arg; // 상태 값 저장
    
    // I2C 장치 초기화
    int fd = wiringPiI2CSetup(P_ADDR);
    if (fd < 0) {
        perror("I2C Sertup");
        return NULL;
    }

    while(1) {
        int raw_light = read_pcf_adc(fd, CH_LIGHT);
        int raw_temp = read_pcf_adc(fd, CH_TEMP);

        pthread_mutex_lock(&state->mutex); // 다른 쓰레드가 값을 읽는 도중 데이터 변경 방지

        state->light_intensity = raw_light;
        state->temperature = convert_to_celsius(raw_temp);

        pthread_mutex_unlock(&state->mutex);

        delay(500); // CPU 점유율을 위한 딜레이
    }

    return NULL;
}