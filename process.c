#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NUM_PROCESSES 5000
#define MAX_PID 4999

int main()
{
    FILE *fp = fopen("processes.txt", "w");
    if (fp == NULL)
    {
        printf("파일 생성 실패\n");
        return 1;
    }

    srand(time(NULL)); // 랜덤 시드 초기화

    int used_pid[MAX_PID + 1] = {0}; // 중복 방지용

    int count = 0;
    while (count < NUM_PROCESSES)
    {
        int pid = rand() % (MAX_PID + 1);
        if (used_pid[pid])
            continue; // 이미 사용된 pid는 skip
        used_pid[pid] = 1;

        int arrival = rand() % 1000;   // 도착 시간: 0~999
        int burst = (rand() % 20) + 1; // 실행 시간: 1~20

        fprintf(fp, "%d %d %d\n", pid, arrival, burst);
        count++;
    }

    fclose(fp);
    printf("processes.txt 생성 완료\n");
    return 0;
}