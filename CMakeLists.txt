cmake_minimum_required(VERSION 3.13)
project(VedaRemoteControl C)

set(CMAKE_C_STANDARD 99)
add_compile_options(-Wall -Wextra -O2)
add_definitions(-D_GNU_SOURCE)

include_directories(include)
find_package(Threads REQUIRED)

# 빌드 타겟 분리 스위치 (기본값: CLIENT)
set(TARGET_BOARD "CLIENT" CACHE STRING "Build target: CLIENT(Ubuntu) or PI(Raspberry Pi)")

if(TARGET_BOARD STREQUAL "PI")
    message(STATUS "CMake: 라즈베리파이 서버 및 센서 라이브러리 빌드")

    # 우분투 환경의 크로스 컴파일러 경로에 wiringPi 라이브러리가 세팅되어 있어야 합니다.
    set(WIRINGPI_LIBRARIES wiringPi)

    # 1. 하드웨어 장치별 .so 모듈 
    add_library(led_control SHARED src/led.c)
    target_link_libraries(led_control PRIVATE ${WIRINGPI_LIBRARIES})

    add_library(buzzer_control SHARED src/buzzer.c)
    target_link_libraries(buzzer_control PRIVATE ${WIRINGPI_LIBRARIES})

    add_library(i2c_control SHARED src/i2c_light_temper.c)
    target_link_libraries(i2c_control PRIVATE ${WIRINGPI_LIBRARIES})

    add_library(seg7_control SHARED src/seg7.c)
    target_link_libraries(seg7_control PRIVATE ${WIRINGPI_LIBRARIES})

    # 라즈베리파이용 서버 본체 
    add_executable(remote_server src/main.c src/tcp_server.c)
    target_link_libraries(remote_server PRIVATE ${CMAKE_DL_LIBS} Threads::Threads ${WIRINGPI_LIBRARIES} crypt)
    
    # 메인 실행 파일의 전역 변수를 .so 파일과 공유 허용
    target_link_options(remote_server PRIVATE -rdynamic)

else()
    message(STATUS "CMake: 우분투 클라이언트 빌드")
    
    # 3. 우분투용 클라이언트 프로그램
    add_executable(remote_client src/client.c)
    target_link_libraries(remote_client PRIVATE Threads::Threads)
endif()