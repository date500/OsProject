#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NUM_PROCESSES 5000
#define MAX_PID 9999 // PID 범위를 더 확장

int main()
{
    FILE *fp = fopen("processes.txt", "w");
    if (fp == NULL)
    {
        printf("파일 생성 실패\n");
        return 1;
    }

    srand(time(NULL));

    int used_pid[MAX_PID + 1] = {0};
    int count = 0;

    while (count < NUM_PROCESSES)
    {
        int pid = rand() % (MAX_PID + 1);
        if (used_pid[pid])
            continue;
        used_pid[pid] = 1;

        int arrival = rand() % 3000;
        int burst = (rand() % 45) + 5;

        fprintf(fp, "%d %d %d\n", pid, arrival, burst);
        count++;
    }

    fclose(fp);
    printf("processes.txt 생성 완료\n");
    return 0;
}
