cmake_minimum_required(VERSION 3.10)
project(VedaRemoteControl C)

# C 표준 및 컴파일 옵션 설정
set(CMAKE_C_STANDARD 99)
add_compile_options(-Wall -Wextra -O2)

# 실행 파일이 저장될 경로 지정: 프로젝트_루트/build
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)

# 헤더 파일 디렉토리 지정
include_directories(include)

# 필수 라이브러리(POSIX 쓰레드) 로드
find_package(Threads REQUIRED)

# 라즈베리파이 외장 하드웨어 제어 라이브러리 명시
set(WIRINGPI_LIBRARIES wiringPi)


# ED 모듈: libled_control.so 생성
add_library(led_control SHARED src/led.c)
target_link_libraries(led_control PRIVATE ${WIRINGPI_LIBRARIES})

# 부저 모듈: libbuzzer_control.so 생성
add_library(buzzer_control SHARED src/buzzer.c)
target_link_libraries(buzzer_control PRIVATE ${WIRINGPI_LIBRARIES})

# 조도/온도 센서 모듈: libi2c_control.so 생성
add_library(i2c_control SHARED src/i2c_light_temper.c)
target_link_libraries(i2c_control PRIVATE ${WIRINGPI_LIBRARIES})

# 7세그먼트 모듈: libseg7_control.so 생성
add_library(seg7_control SHARED src/seg7.c)
target_link_libraries(seg7_control PRIVATE ${WIRINGPI_LIBRARIES})


# 서버 실행 파일 생성 (main.c 와 tcp_server.c 포함) - 라즈베리파이에서 실행
add_executable(remote_server src/main.c src/tcp_server.c)
target_link_libraries(remote_server PRIVATE ${CMAKE_DL_LIBS} Threads::Threads ${WIRINGPI_LIBRARIES})

# 클라이언트 실행 파일 생성 (client.c) - 우분투 리눅스에서 실행
add_executable(remote_client src/client.c)
target_link_libraries(remote_client PRIVATE Threads::Threads)