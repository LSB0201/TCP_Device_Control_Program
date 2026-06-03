#include <wiringPi.h>
#include <wiringPiI2C.h>

#include "common.h"

#define P_ADDR 0x48
#define CH_LIGHT 0
#define CH_TEMP 1

static int read_pcf_adc(int fd, int channel) {
    wiringPiI2CWrite(fd, channel);

    wiringPiI2CRead(fd); // 쓰래기 값 버리기

    return wiringPiI2CRead(fd); // 실제 데이터 반환
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
        state->temperature = (double)raw_temp;

        pthread_mutex_unlock(&state->mutex);

        delay(500); // CPU 점유율을 위한 딜레이
    }

    return NULL;
}