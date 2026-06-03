#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>
#include <pthread.h>

#define SERVER_IP "100.95.241.94" // 서버로 구동할 라즈베리파이 IP로 변경
#define SERVER_PORT 8080

volatile int keep_running = 1;

void* receive_thread(void* arg) {
    int sock = *(int*)arg;
    char buf[256];

    while (keep_running) {
        memset(buf, 0, sizeof(buf));
        int n = read(sock, buf, sizeof(buf) - 1);
        
        if (n > 0) {
            // 서버 응답 출력
            printf("\n[서버 응답] %s\n", buf);
            printf("메뉴 선택 > "); 
            fflush(stdout); // 버퍼 강제 비우기로 UI 정렬
        } else {
            printf("\n[알림] 서버와의 연결이 끊어졌습니다. 프로그램을 종료합니다.\n");
            keep_running = 0;
            exit(0);
        }
    }
    return NULL;
}

int main() {
    int choice;
    char cmd[128];
    char input_str[20];

    sigset_t mask;

    // SIGINT만 허용
    sigfillset(&mask);
    sigdelset(&mask, SIGINT);
    sigprocmask(SIG_SETMASK, &mask, NULL);

    // 최초 1회 연결 
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    serv_addr.sin_port = htons(SERVER_PORT);

    printf("서버(%s)에 연결을 시도합니다...\n", SERVER_IP);
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        printf("[오류] 서버가 꺼져 있거나 연결할 수 없습니다.\n");
        return -1;
    }
    printf("서버 연결 성공!\n");

    // 상시 수신 쓰레드 구동
    pthread_t tid;
    if (pthread_create(&tid, NULL, receive_thread, &sock) != 0) {
        perror("수신 쓰레드 생성 실패");
        close(sock);
        return -1;
    }
    pthread_detach(tid);

    while (keep_running) {
        printf("\n=========================================\n");
        printf(" 📡 VEDA C-Client 메인 메뉴 (지속 연결)\n");
        printf("=========================================\n");
        printf(" 1. LED 상세 제어 (서브메뉴)\n");
        printf(" 2. 부저 상세 제어 (서브메뉴)\n");
        printf(" 3. 7-Segment 카운트다운 시작\n");
        printf(" 4. 센서 데이터 (온도/조도) 확인\n");
        printf(" 5. 모든 장치 긴급 정지\n");
        printf(" 0. 클라이언트 프로그램 종료\n");
        printf("=========================================\n");
        printf("메뉴 선택 > ");
        
        if (scanf("%d", &choice) != 1) {
            while(getchar() != '\n'); 
            continue;
        }

        switch (choice) {
            case 1:
                while(keep_running) {
                    printf("\n[LED 제어] on / off / low / mid / high / exit 입력 > ");
                    if (scanf("%19s", input_str) != 1) continue;

                    if (strcmp(input_str, "exit") == 0) {
                        printf("메인 메뉴로 복귀합니다.\n");
                        break; 
                    } 
                    else if (strcmp(input_str, "on") == 0 || strcmp(input_str, "high") == 0) {
                        if (write(sock, "CMD:LED_PWM:100", 15) < 0) break;
                    } 
                    else if (strcmp(input_str, "off") == 0) {
                        if (write(sock, "CMD:LED_PWM:0", 13) < 0) break;
                    } 
                    else if (strcmp(input_str, "low") == 0) {
                        if (write(sock, "CMD:LED_PWM:30", 14) < 0) break;
                    } 
                    else if (strcmp(input_str, "mid") == 0) {
                        if (write(sock, "CMD:LED_PWM:60", 14) < 0) break;
                    } 
                    else {
                        printf("잘못된 입력입니다. 다시 입력해주세요.\n");
                    }
                }
                break;

            case 2:
                while(keep_running) {
                    printf("\n[부저 제어] 노래 재생(on) / 중단(off) / exit 입력 > ");
                    if (scanf("%19s", input_str) != 1) continue;

                    if (strcmp(input_str, "exit") == 0) {
                        printf("메인 메뉴로 복귀합니다.\n");
                        break; 
                    } 
                    else if (strcmp(input_str, "on") == 0) {
                        if (write(sock, "CMD:BUZ_PLAY", 12) < 0) break;
                    } 
                    else if (strcmp(input_str, "off") == 0) {
                        if (write(sock, "CMD:BUZ_STOP", 12) < 0) break;
                    } 
                    else {
                        printf("잘못된 입력입니다. 다시 입력해주세요.\n");
                    }
                }
                break;

            case 3:
                printf("\n시작할 숫자 입력 (0~9): ");
                int num;
                if (scanf("%d", &num) != 1) {
                    while(getchar() != '\n');
                    printf("숫자를 올바르게 입력해주세요.\n");
                    break;
                }
                snprintf(cmd, sizeof(cmd), "CMD:SEG_START:%d", num);
                if (write(sock, cmd, strlen(cmd)) < 0) break;
                break;

            case 4:
                if (write(sock, "CMD:GET_SENSOR", 14) < 0) break;
                break;

            case 5:
                if (write(sock, "CMD:STOP_ALL", 12) < 0) break;
                break;

            case 0:
                printf("\n클라이언트를 종료합니다.\n");
                keep_running = 0;
                close(sock);
                return 0;

            default:
                printf("\n잘못된 메인 메뉴 번호입니다.\n");
        }
    }
    close(sock);
    return 0;
}