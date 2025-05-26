# OS Projects

# 🧠 CPU 스케줄링 시뮬레이터

C 언어로 작성된 CPU 스케줄러 시뮬레이터입니다.  
FCFS, SJF, SRTF, Round Robin, Stride, Lottery, MLFQ, CFS 등 다양한 스케줄링 알고리즘을 구현하였으며, 각 알고리즘의 응답 시간, 대기 시간, 반환 시간, 컨텍스트 스위치 횟수, starvation 비율 등을 비교할 수 있습니다.

---

## 📦 주요 기능

- 🔁 FCFS (선입선출)
- ⏱ SJF / SRTF (최단 작업 우선 / 잔여 시간 최소 우선)
- 🔄 Round Robin (타임 퀀텀 조정 가능)
- 🎫 Stride / Lottery 스케줄링 (티켓 기반 공정 스케줄링)
- 📚 MLFQ (다단계 피드백 큐)
- ⚖️ CFS (공정 스케줄링)
- 📈 평균 응답 시간 / 대기 시간 / 반환 시간 출력
- 🔍 Starvation 감지 및 비율 출력
- 🔁 컨텍스트 스위치 횟수 추적

---

## 🚀 사용 방법

### 1. 요구 사항
- GCC 컴파일러
- 프로세스 입력 파일 process.c 실행 후 processes.txt 생성
- main.c를 실행 시킴
