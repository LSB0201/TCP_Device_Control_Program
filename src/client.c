#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define SERVER_IP "100.95.241.94" // 서버로 구동할 라즈베리파이 IP로 변경
#define SERVER_PORT 8080

void send_command_to_server(const char *cmd) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    serv_addr.sin_port = htons(SERVER_PORT);

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        printf("[오류] 서버 연결 실패\n");
        close(sock);
        return;
    }

    if (write(sock, cmd, strlen(cmd)) < 0) {
        printf("[오류] 명령 전송 실패\n");
    }

    char buf[256] = {0};
    if (read(sock, buf, sizeof(buf) - 1) > 0) {
        printf("[서버 응답] %s\n", buf);
    }
    else {
        printf("[오류] 서버 응답을 읽지 못했습니다.\n");
    }

    close(sock); 
}

int main() {
    int choice;
    char cmd[128];
    char input_str[20];

    while (1) {
        printf("\n=========================================\n");
        printf(" VEDA C-Client 메인 메뉴\n");
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
                while(1) {
                    printf("\n[LED 제어] on / off / low / mid / high / exit 입력 > ");
                    if (scanf("%19s", input_str) != 1) continue;    // 버퍼 오버플로우 방지

                    if (strcmp(input_str, "exit") == 0) {
                        printf("메인 메뉴로 복귀합니다.\n");
                        break; 
                    } 
                    else if (strcmp(input_str, "on") == 0 || strcmp(input_str, "high") == 0) {
                        send_command_to_server("CMD:LED_PWM:100"); // 100% 밝기
                    } 
                    else if (strcmp(input_str, "off") == 0) {
                        send_command_to_server("CMD:LED_PWM:0");   // 소등
                    } 
                    else if (strcmp(input_str, "low") == 0) {
                        send_command_to_server("CMD:LED_PWM:30");  // 30% 밝기
                    } 
                    else if (strcmp(input_str, "mid") == 0) {
                        send_command_to_server("CMD:LED_PWM:60");  // 60% 밝기
                    } 
                    else {
                        printf("잘못된 입력입니다. 다시 입력해주세요.\n");
                    }
                }
                break;

            case 2:
                while(1) {
                    printf("\n[부저 제어] 노래 재생(on) / 중단(off) / exit 입력 > ");
                    if (scanf("%19s", input_str) != 1) continue;    // 버퍼 오버플로우 방지

                    if (strcmp(input_str, "exit") == 0) {
                        printf("메인 메뉴로 복귀합니다.\n");
                        break; 
                    } 
                    else if (strcmp(input_str, "on") == 0) {
                        send_command_to_server("CMD:BUZ_PLAY");
                    } 
                    else if (strcmp(input_str, "off") == 0) {
                        send_command_to_server("CMD:BUZ_STOP");
                    } 
                    else {
                        printf("잘못된 입력입니다. 다시 입력해주세요.\n");
                    }
                }
                break;

            case 3:
                printf("\n시작할 숫자 입력 (0~9): ");
                int num;
                // scanf 예외 처리
                if (scanf("%d", &num) != 1) {
                    while(getchar() != '\n'); // 잘못된 문자 입력 시 버퍼 비우기
                    printf("숫자를 올바르게 입력해주세요.\n");
                    break;
                }
                snprintf(cmd, sizeof(cmd), "CMD:SEG_START:%d", num);
                send_command_to_server(cmd);
                break;

            case 4:
                send_command_to_server("CMD:GET_SENSOR");
                break;

            case 5:
                send_command_to_server("CMD:STOP_ALL");
                break;

            case 0:
                printf("\n클라이언트를 종료합니다.\n");
                return 0;

            default:
                printf("\n잘못된 메인 메뉴 번호입니다.\n");
        }
    }
    return 0;
}