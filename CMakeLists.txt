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


# 라이브러리 빌드 (하드웨어 제어 및 네트워크 공통 모듈)
add_library(remote_core STATIC
    src/tcp_server.c
    src/led.c
    src/buzzer.c
    src/i2c_light_temper.c
    src/seg7.c
)

# 라이브러리에 필요한 의존성 링크
target_link_libraries(remote_core PRIVATE ${WIRINGPI_LIBRARIES} Threads::Threads)

# 서버 실행 파일 생성 (main.c) - 라즈베리파이에서 실행
add_executable(remote_server src/main.c)
target_link_libraries(remote_server PRIVATE remote_core ${WIRINGPI_LIBRARIES} Threads::Threads)


# 클라이언트 실행 파일 생성 (client.c) - 우분투 실행
add_executable(remote_client src/client.c)
target_link_libraries(remote_client PRIVATE Threads::Threads)