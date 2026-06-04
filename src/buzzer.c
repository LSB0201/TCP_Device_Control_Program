#include <wiringPi.h>
#include <softTone.h>

#include "common.h"

#define BUZZER_PIN 2 // WiringPi 2번 (BCM 27)

// 음계 주파수 매크로
#define C5  523
#define D5  587
#define Eb5 622
#define F5  698
#define G5  784
#define Ab5 831
#define C6  1047
#define D6  1175
#define Eb6 1245
#define REST 0 // 쉼표

// 주파수와 음의 길이(ms)를 묶어서 관리하는 구조체 선언
typedef struct {
    int freq;
    int duration;
} Note;

Note notes[] = {
    // 19 ~ 22 마디
    {C6, 945}, {G5, 315},              // 솔(3박) 레(1박) -> (상대음감 기준)
    {Eb6, 630}, {D6, 315}, {C6, 315},  // 미(2) 레(1) 도(1)
    {D6, 945}, {C6, 315},              // 레(3) 도(1)
    {G5, 1260},                        // 솔(4)

    // 23 ~ 26 마디 (반복)
    {C6, 945}, {G5, 315},
    {Eb6, 630}, {D6, 315}, {C6, 315},
    {D6, 945}, {C6, 315},
    {G5, 1260},

    // 28 ~ 31 마디 (분위기 고조 구간)
    {Ab5, 945}, {G5, 315},
    {F5, 630}, {Eb5, 315}, {D5, 315},
    {Eb5, 945}, {D5, 315},
    {C5, 1260}
};

void set_buzzer(int state) {
    static int init_flag = 0;

    // init_flag를 사용해 최초 한 번만 softPwm 초기화
    if(!init_flag)
    {
        softToneCreate(BUZZER_PIN); 
        init_flag = 1;
    }

    if (state) {
        softToneWrite(BUZZER_PIN, 800); // 소리 on
    } else {
        softToneWrite(BUZZER_PIN, 0); // 소리 off
    }
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

        if (notes[i].freq == 0) {
            softToneWrite(BUZZER_PIN, 0);
            delay(200); // 쉼표 대기
        }
        else {
            softToneWrite(BUZZER_PIN, notes[i].freq); // 해당 주파수(계이름) 출력 
        }

        // 구조체에 저장된 박자만큼 대기
        delay(notes[i].duration);

        // 음 구분을 위한 딜레이 추가
        softToneWrite(BUZZER_PIN, 0);
        delay(30);
    }
    
    // 노래가 끝났거나 중단되었을 때 부저 소리 끄기
    softToneWrite(BUZZER_PIN, 0); 

    // 작업이 자신이 실행 중인 상태로 끝났다면 플래그 초기화
    if (current_exclusive_task == 1) {
        current_exclusive_task = 0;
    }
    
    return NULL;
}