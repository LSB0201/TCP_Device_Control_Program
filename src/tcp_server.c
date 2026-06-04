#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <pthread.h>
#include <wiringPi.h>

#include "server.h"
#include "hardware.h"

// 이전 작업 취소 및 쓰레드 교체 함수
void start_exclusive_task(int task_type, int arg_val) {
    // 기존에 실행 중인 작업이 있으면 취소
    if (current_exclusive_task != 0) {
        cancel_task_flag = 1;
        while (current_exclusive_task != 0) { delay(10); } 
    }
    cancel_task_flag = 0; // 플래그 초기화

    pthread_t tid;
    if (task_type == 1) {
        // buzzer.c에 있는 함수 호출
        pthread_create(&tid, NULL, hw.buzzer_song_thread, NULL);
        pthread_detach(tid); 
    }
    else if (task_type == 2) {
        // seg7.c에 있는 함수 호출
        int* start_val = malloc(sizeof(int));
        *start_val = arg_val;
        pthread_create(&tid, NULL, hw.seg_countdown_thread, (void*)start_val);
        pthread_detach(tid);
    }
}

// TCP 서버 초기화 - socket()부터 listen()까지
int init_tcp_server(int port) {
    int server_fd;
    struct sockaddr_in server_addr;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) return -1;
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) return -1;
    if (listen(server_fd, 10) < 0) return -1;
    return server_fd;
}

// HTTP 라우팅 및 처리
int handle_client_request(int client_fd, DeviceState* state) {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    
    int n = read(client_fd, buffer, BUFFER_SIZE - 1);
    if (n <= 0) return 0; // 상대방이 소켓을 닫았거나 에러 시 0 반환 (연결 종료)

    // 디버그용
    printf("\n소켓 FD(%d), %d 바이트\n", client_fd, n);
    printf("-----------------------------------------\n");
    printf("%s\n", buffer);
    printf("-----------------------------------------\n");
    fflush(stdout); // 터미널 출력 버퍼 즉시 비우기

    // client.c TCP 명령어 처리
    if (strncmp(buffer, "CMD:", 4) == 0) {
        char resp[256] = "Command Executed Successfully.";
        
        if (strncmp(buffer, "CMD:LED_PWM:", 12) == 0) {
            state->auto_led_mode = 0; // 수동 제어 시 자동 모드 해제
            int val = atoi(buffer + 12);
            hw.set_led_brightness(val);
        }
        else if (strcmp(buffer, "CMD:BUZ_PLAY") == 0) {
            start_exclusive_task(1, 0);
        }
        else if (strcmp(buffer, "CMD:BUZ_STOP") == 0) {
            if (current_exclusive_task == 1) cancel_task_flag = 1;
            hw.set_buzzer(0);
        }
        else if (strncmp(buffer, "CMD:SEG_START:", 14) == 0) {
            int val = atoi(buffer + 14);
            start_exclusive_task(2, val);
        }
        else if (strcmp(buffer, "CMD:STOP_ALL") == 0) {
            cancel_task_flag = 1;
            state->auto_led_mode = 0; 
            hw.set_led_brightness(0);
            hw.set_buzzer(0);
            hw.display_7segment(0);
        }
        else if (strcmp(buffer, "CMD:GET_SENSOR") == 0) {
            pthread_mutex_lock(&state->mutex);
            snprintf(resp, sizeof(resp), "현재 온도: %.1f C | 조도 값: %d",
                     state->temperature, state->light_intensity);
            state->auto_led_mode = 1; // 자동 모드 켜기
            pthread_mutex_unlock(&state->mutex);
        }
        else {
            snprintf(resp, sizeof(resp), "Unknown Command.");
        }
        
        if (write(client_fd, resp, strlen(resp)) < 0) return 0;
        return 1; // Client.c는 지속 연결이므로 소켓을 유지(1 반환)
    }

    // 웹 브라우저에서 보낸 HTTP 명령어(AJAX) 처리
    else if (strstr(buffer, "GET /api/cmd?") != NULL) {
        char *ptr;
        if ((ptr = strstr(buffer, "led_pwm=")) != NULL) {
            state->auto_led_mode = 0;
            hw.set_led_brightness(atoi(ptr + 8));
        }

        if (strstr(buffer, "buzzer=play") != NULL) start_exclusive_task(1, 0);

        if (strstr(buffer, "buzzer=stop") != NULL) {
            if (current_exclusive_task == 1) cancel_task_flag = 1;
            hw.set_buzzer(0);
        }
        if ((ptr = strstr(buffer, "seg_count=")) != NULL) start_exclusive_task(2, atoi(ptr + 10));

        if (strstr(buffer, "auto_led=0") != NULL) {
            state->auto_led_mode = 0;
            hw.set_led_brightness(0); 
        }

        if (strstr(buffer, "stop_all=1") != NULL) {
            cancel_task_flag = 1;
            state->auto_led_mode = 0;
            hw.set_led_brightness(0);
            hw.set_buzzer(0);
            hw.display_7segment(0);
        }
        const char* resp = "HTTP/1.1 200 OK\r\nConnection: close\r\n\r\n{\"status\":\"ok\"}";
        if (write(client_fd, resp, strlen(resp)) < 0) return 0;

        return 0;
    }

    // 웹 브라우저 센서 데이터 요청 처리 (JSON 반환)
    else if (strstr(buffer, "GET /api/sensor") != NULL) {
        char resp[256];
        pthread_mutex_lock(&state->mutex);

        snprintf(resp, sizeof(resp),
            "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n"
            "{\"light\":%d, \"temp\":%.1f}", state->light_intensity, state->temperature);

        pthread_mutex_unlock(&state->mutex);

        if (write(client_fd, resp, strlen(resp)) < 0) return 0;

        return 0;
    }

    // 웹 브라우저 최초 접속 시 HTML 화면 전송
    else if (strncmp(buffer, "GET / ", 6) == 0 || strncmp(buffer, "GET /HTTP", 9) == 0) {
        const char *header = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\nConnection: close\r\n\r\n";
        if (write(client_fd, header, strlen(header)) < 0) return 0;

        FILE *fp = fopen("../client_remote_control.html", "r");
        if (!fp) fp = fopen("client_remote_control.html", "r");
        
        if (fp != NULL) {
            char file_buf[1024];
            size_t bytes_read;
            while ((bytes_read = fread(file_buf, 1, sizeof(file_buf), fp)) > 0) {
                if (write(client_fd, file_buf, bytes_read) < 0) break;
            }
            fclose(fp);
        }
        else {
            const char *err = "HTML file not found.";
            if (write(client_fd, err, strlen(err)) < 0) return 0;
        }
        return 0;
    }
    return 0;
}

// epoll 이벤트 루프
void run_server_loop(int server_fd, DeviceState* state, volatile sig_atomic_t* keep_running) {
    int epfd = epoll_create1(0);
    struct epoll_event ev, events[MAX_EVENTS];
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &ev);

    while (*keep_running) {
        int nfds = epoll_wait(epfd, events, MAX_EVENTS, 1000);
        if (nfds == -1) continue;

        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == server_fd) {
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);

                int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
                if (client_fd >= 0) {
                    // 지속적인 스트림 체크를 위해 Level Triggered(EPOLLIN) 방식으로 안정성 확보
                    ev.events = EPOLLIN; 
                    ev.data.fd = client_fd;
                    epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev);
                }
            }
            else {
                int client_fd = events[i].data.fd;
                // handle_client_request 결과가 0(종료 요청)일 때만 epoll에서 지우고 소켓을 닫음
                if (handle_client_request(client_fd, state) == 0) {
                    epoll_ctl(epfd, EPOLL_CTL_DEL, client_fd, NULL);
                    close(client_fd);
                }
            }
        }
    }
    close(epfd);
}