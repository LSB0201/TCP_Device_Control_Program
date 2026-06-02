#include "common.h"
#include "hardware.h"
#include "server.h"
#include <wiringPi.h> // wiringPiSetup() 호출을 위해 포함

// 노래와 카운트 다운, 배타적 실행 제어를 위한 전역 변수
volatile int current_exclusive_task = 0; // common.h 헤더파일 참고
volatile int cancel_task_flag = 0;

// 안전 종료를 위한 종료 플래그
volatile sig_atomic_t keep_running = 1;

// 공유 상태 구조체 선언(센서 데이터, 상태, 뮤텍스 포함)
DeviceState device_state;

// 종료 시그널 핸들러
void handle_signal(int sig) {
    // 경고 회피용 (사용하지 않는 파라미터 처리)
    (void)sig; 
    keep_running = 0;
}

// 전체 하드웨어 초기화
int init_hardware(void) {
    // WiringPi 초기화 (루트 권한 필요)
    if (wiringPiSetup() == -1) {
        perror("wiringPiSetup failed");
        return -1;
    }
    
    // 각 출력 장치 초기 상태 설정
    set_led_brightness(0);
    set_buzzer(0);
    display_7segment(0);
    
    return 0;
}

// 전체 하드웨어 종료
void cleanup_hardware(void) {
    set_led_brightness(0);
    set_buzzer(0);
    display_7segment(0); 
}

int main(int argc, char *argv[]) {
    int daemon_mode = 0;
    pthread_t sensor_tid;

    // 실행 인자 파싱 (데몬 모드 확인) <- 디버그용 추후 삭제
    if (argc > 1 && strcmp(argv[1], "-d") == 0) {
        daemon_mode = 1;
    }

    // 데몬화 처리
    if (daemon_mode) {
        if (daemon(0, 0) == -1) {
            perror("Daemonization failed");
            exit(EXIT_FAILURE);
        }
    } else {
        printf("Starting Remote Control Server in Foreground...\n");
    }

    // 시그널 핸들러 등록 (Ctrl+C 종료)
    signal(SIGINT, handle_signal);

    // 공유 상태 구조체 및 Mutex 초기화
    pthread_mutex_init(&device_state.mutex, NULL);
    device_state.light_intensity = 0;
    device_state.temperature = 0.0;
    device_state.led_brightness = 0;
    device_state.buzzer_status = 0;
    device_state.current_seg_num = 0;

    // 하드웨어 초기화 (WiringPi)
    if (init_hardware() < 0) {
        exit(EXIT_FAILURE);
    }

    // I2C 센서 데이터 수집 쓰레드 생성 (백그라운드에서 계속 동작)
    if (pthread_create(&sensor_tid, NULL, i2c_sensor_thread, (void*)&device_state) != 0) {
        perror("Failed to create sensor thread");
        exit(EXIT_FAILURE);
    }

    // TCP 서버 초기화
    int server_fd = init_tcp_server(DEFAULT_PORT);
    if (server_fd < 0) {
        keep_running = 0; // 서버 생성 실패 시 바로 종료 수순으로 이동
    } else {
        if (!daemon_mode) {
            printf("Server initialized on port %d. Waiting for connections...\n", DEFAULT_PORT);
        }
        
        // 메인 이벤트 루프 실행 (epoll) - 함수 내부에서 무한 루프
        run_server_loop(server_fd, &device_state, &keep_running);
    }

    // 데몬 모드가 아닐 때
    if (!daemon_mode) printf("\nShutting down server safely...\n");

    // 센서 쓰레드 강제 종료 후 조인으로 자원 회수
    pthread_cancel(sensor_tid); 
    pthread_join(sensor_tid, NULL);

    // 서버 소켓 닫기
    if (server_fd >= 0) {
        close(server_fd);
    }

    // 모든 장치 끄기 및 Mutex 없애기
    cleanup_hardware();
    pthread_mutex_destroy(&device_state.mutex);

    if (!daemon_mode) printf("Shutdown complete.\n");
    
    return EXIT_SUCCESS;
}