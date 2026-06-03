#include <wiringPi.h>
#include <dlfcn.h>
#include <signal.h>

#include "common.h"
#include "hardware.h"
#include "server.h"

// 부저 배타적 실행 제어를 위한 전역 변수
volatile int current_exclusive_task = 0;
volatile int cancel_task_flag = 0;

// 안전 종료를 위한 종료 플래그
volatile sig_atomic_t keep_running = 1;

// 공유 상태 구조체 선언(센서 데이터, 상태, 뮤텍스 포함)
DeviceState device_state;

// 하드웨어 API 구조체 및 라이브러리 핸들
HardwareAPI hw;
void *led_lib_handle = NULL;
void *buzzer_lib_handle = NULL;
void *i2c_lib_handle = NULL;
void *seg7_lib_handle = NULL;

// 종료 시그널 핸들러
void handle_signal(int sig) {
    // 경고 회피용 (사용하지 않는 파라미터 처리)
    (void)sig; 
    keep_running = 0;
}

// 동적 라이브러리 로드 및 함수 매핑
int load_hardware_library(void) {
    // LED 라이브러리 로드
    led_lib_handle = dlopen("./libled_control.so", RTLD_LAZY);
    if (!led_lib_handle) { fprintf(stderr, "LED SO Load Error: %s\n", dlerror()); return -1; }
    hw.set_led_brightness = dlsym(led_lib_handle, "set_led_brightness");

    // 부저 라이브러리 로드
    buzzer_lib_handle = dlopen("./libbuzzer_control.so", RTLD_LAZY);
    if (!buzzer_lib_handle) { fprintf(stderr, "Buzzer SO Load Error: %s\n", dlerror()); return -1; }
    hw.set_buzzer = dlsym(buzzer_lib_handle, "set_buzzer");
    hw.buzzer_song_thread = dlsym(buzzer_lib_handle, "buzzer_song_thread");

    // 조도/온도 센서 라이브러리 로드
    i2c_lib_handle = dlopen("./libi2c_control.so", RTLD_LAZY);
    if (!i2c_lib_handle) { fprintf(stderr, "I2C SO Load Error: %s\n", dlerror()); return -1; }
    hw.i2c_sensor_thread = dlsym(i2c_lib_handle, "i2c_sensor_thread");

    // 7세그먼트 라이브러리 로드
    seg7_lib_handle = dlopen("./libseg7_control.so", RTLD_LAZY);
    if (!seg7_lib_handle) { fprintf(stderr, "SEG7 SO Load Error: %s\n", dlerror()); return -1; }
    hw.display_7segment = dlsym(seg7_lib_handle, "display_7segment");
    hw.seg_countdown_thread = dlsym(seg7_lib_handle, "seg_countdown_thread");

    return 0;
}

// 전체 하드웨어 종료
void cleanup_hardware(void) {
    if (hw.set_led_brightness) hw.set_led_brightness(0);
    if (hw.set_buzzer) hw.set_buzzer(0);
    if (hw.display_7segment) hw.display_7segment(0);

    // 각각의 라이브러리 핸들 닫기
    if (led_lib_handle) dlclose(led_lib_handle);
    if (buzzer_lib_handle) dlclose(buzzer_lib_handle);
    if (i2c_lib_handle) dlclose(i2c_lib_handle);
    if (seg7_lib_handle) dlclose(seg7_lib_handle);
}

int main(int argc, char *argv[]) {
    int daemon_mode = 0;
    pthread_t sensor_tid;

    sigset_t mask;

    // SIGINT만 허용
    sigfillset(&mask);
    sigdelset(&mask, SIGINT);
    sigprocmask(SIG_SETMASK, &mask, NULL);

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

    // Mutex 초기화
    pthread_mutex_init(&device_state.mutex, NULL);

    // 공유 상태 구조체 초기화
    device_state.light_intensity = 0;
    device_state.temperature = 0.0;
    device_state.led_brightness = 0;
    device_state.buzzer_status = 0;
    device_state.current_seg_num = 0;

    // 하드웨어 초기화(WiringPi)
    if (wiringPiSetup() == -1) exit(EXIT_FAILURE);

    // 동적 라이브러리(.so) 로드
    if (load_hardware_library() < 0) exit(EXIT_FAILURE);

    hw.set_led_brightness(0);
    hw.set_buzzer(0);
    hw.display_7segment(0);

    // I2C 센서 데이터 수집 쓰레드 생성 (백그라운드에서 계속 동작)
    if (pthread_create(&sensor_tid, NULL, hw.i2c_sensor_thread, (void*)&device_state) != 0) {
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