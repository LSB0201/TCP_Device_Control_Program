#!/bin/bash

# 사용법 안내
if [ -z "$1" ]; then
    echo "사용법: ./build.sh [client | server | clean]"
    exit 1
fi

if [ "$1" == "client" ]; then
    echo "우분투 클라이언트 빌드를 시작합니다..."
    cmake -S . -B build_client -DTARGET_BOARD=CLIENT
    cmake --build build_client

elif [ "$1" == "server" ]; then
    echo "라즈베리파이 서버용 크로스 컴파일을 시작합니다..."
    cmake -S . -B build_server -DTARGET_BOARD=PI -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc
    cmake --build build_server

elif [ "$1" == "clean" ]; then
    echo "기존 빌드 파일들을 삭제합니다..."
    rm -rf build_client build_pi
    echo "삭제 완료!"

else
    echo "올바르지 않은 옵션입니다. (client, server, clean 중 택 1)"
fi