# 📡 VEDA - 센서 원격 제어 시스템(Remote Sensor Control System)

라즈베리파이 서버와 우분투 PC(또는 웹 브라우저) 클라이언트 간의 다중 원격 하드웨어 제어 시스템

---

## 🛠️ 1. 사전 준비 (Prerequisites)

### 1-1. veda_remote_control/src/client.c 파일의 10번째 줄 코드를 서버로 구동할 개인 라즈베리파이 ip 주소로 변경해주세요.
`#define SERVER_IP "100.95.241.94" //" " 부분을 수정!`

### 1-2. 크로스 컴파일 환경 빌드 시 `libcrypt` 의존성 에러가 발생할 경우, 타겟 보드(라즈베리파이)에서 우분투 호스트 PC로 라이브러리를 복사해야 합니다.
```bash
# 라즈베리파이의 libcrypt 라이브러리들을 우분투의 크로스 컴파일 환경으로 복사
sudo scp 라즈베리파이_유저_명@개인_라즈베리파이_ip주소:/lib/aarch64-linux-gnu/libcrypt.so* /usr/aarch64-linux-gnu/lib/

# (만약 위 경로에서 파일을 찾을 수 없다고 나오면 아래 경로로 시도)
sudo scp 라즈베리파이_유저_명@개인_라즈베리파이_ip주소:/usr/lib/aarch64-linux-gnu/libcrypt.so* /usr/aarch64-linux-gnu/lib/
```

### 1-3. wiringPi 라이브러리 및 헤더 파일 복사
라즈베리파이의 하드웨어 제어(GPIO, I2C, PWM 등) 코드를 우분투에서 크로스 컴파일하기 위해, 타겟 보드의 `wiringPi` 라이브러리(`.so`)와 헤더 파일(`.h`)들을 우분투 환경으로 가져와야 합니다.

```bash
# 1. wiringPi 공유 라이브러리(.so) 복사
sudo scp 라즈베리파이_유저_명@개인_라즈베리파이_ip주소:/usr/lib/libwiringPi.so* /usr/aarch64-linux-gnu/lib/

# 2. wiringPi 관련 헤더 파일(.h) 복사
# (wiringPi.h, wiringPiI2C.h 등)
sudo scp 라즈베리파이_유저_명@개인_라즈베리파이_ip주소:/usr/include/wiringPi*.h /usr/aarch64-linux-gnu/include/

# (softTone.h, softPwm.h 등 소프트웨어 PWM/톤 관련 헤더)
sudo scp 라즈베리파이_유저_명@개인_라즈베리파이_ip주소:/usr/include/soft*.h /usr/aarch64-linux-gnu/include/
---

## 🏗️ 2. 빌드 방법 (Build)
최상위 폴더에 위치한 `build.sh` 자동화 스크립트를 이용하여 타겟별로 빌드를 수행합니다. 최초 실행 시 스크립트에 실행 권한을 부여해야 합니다.

```bash
# 스크립트 실행 권한 부여 (최초 1회)
chmod +x build.sh

# Server 빌드 (라즈베리파이용 크로스 컴파일)
./build.sh server

# Client 빌드 (우분투 PC용 네이티브 컴파일)
./build.sh client

# 기존 빌드 디렉토리 초기화 (Clean)
./build.sh clean
```

---

## 📦 3. 파일 전송 방법 (Deployment)
우분투 PC에서 빌드 완료된 서버 바이너리 파일과 동적 라이브러리(`.so`), 그리고 웹 제어용 HTML 파일을 라즈베리파이 타겟 보드로 전송합니다.

```bash
# 라즈베리파이로 server 및 라이브러리 파일 전송(프로젝트 루트 디렉토리(veda_remote_control)에서 진행)
scp build_server/remote_server build_server/lib*.so 라즈베리파이_유저_명@개인_라즈베리파이_ip주소:/저장할_파일_경로

# 라즈베리파이로 HTML 파일 전송
scp client_remote_control.html 라즈베리파이_유저_명@개인_라즈베리파이_ip주소:/저장할_파일_경로
```

---

## 🚀 4. 실행 방법 (Running)

### 1) Server 실행 (라즈베리파이 타겟 보드 측)
라즈베리파이 터미널에서 전송받은 디렉토리로 이동한 후, 하드웨어 제어(GPIO) 권한을 위해 `sudo`를 사용하여 실행합니다.
```bash
sudo ./remote_server # 데몬 모드 X

sudo ./remote_server -d # 데몬 모드 실행
```
서버 종료: Ctrl + c

※ 데몬으로 서버 실행시 PID 확인 방법
```bash
ps -ef | grep remote_server

pgrep remote_server // PID만 확인
```

### 2) C-Client 실행 (우분투 호스트 PC 측)
우분투 터미널에서 빌드 완료된 클라이언트 프로그램을 실행하여 텍스트 메뉴 기반으로 원격 제어 명령을 내립니다.
```bash
./build_client/remote_client
```
클라이언트 종료: Ctrl + c 또는 '0' 입력

### 3) Web-Client 실행 (웹 브라우저 제어)
동일한 네트워크 환경에 연결된 PC 또는 모바일 기기의 웹 브라우저 주소창에 아래 주소를 입력하여 GUI 원격 제어 화면에 접속합니다.
```text
[http://개인_라즈베리파이_ip주소:8080]
```
웹 브라우저 종료는 브라우저 창을 닫아주세요.