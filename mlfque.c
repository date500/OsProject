#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define MAX 6000
#define NUM_QUEUES 3
#define BOOST_INTERVAL 150
#define STARVATION_THRESHOLD 1000

typedef struct
{
    int pid;
    int arrival, burst, remaining;
    int start, finish, waiting, turnaround;
    int queue_level;
    int used_time;
    bool completed;
} Process;

typedef struct
{
    int items[MAX];
    int front, rear;
} Queue;

void init_queue(Queue *q)
{
    q->front = q->rear = 0;
}

bool is_empty(Queue *q)
{
    return q->front == q->rear;
}

void enqueue(Queue *q, int value)
{
    q->items[q->rear++] = value;
}

int dequeue(Queue *q)
{
    return q->items[q->front++];
}

void calculate_averages(Process p[], int n, int context_switches)
{
    float total_waiting = 0, total_turnaround = 0, total_response = 0;
    int starvation_count = 0;

    for (int i = 0; i < n; i++)
    {
        total_waiting += p[i].waiting;
        total_turnaround += p[i].turnaround;
        total_response += p[i].start - p[i].arrival;

        if (p[i].waiting > STARVATION_THRESHOLD)
            starvation_count++;
    }

    printf("\n[MLFQ (진짜 RR 기반) 결과]\n");
    printf("Response: %.2f  Waiting: %.2f  Turnaround: %.2f\n",
           total_response / n, total_waiting / n, total_turnaround / n);
    printf("Total Context Switches: %d\n", context_switches);
    printf("Starvation Rate: %.2f%%\n", (float)starvation_count / n * 100);
}

void mlfq_with_rr_queues(Process p[], int n, int *context_switches)
{
    int quantum[NUM_QUEUES] = {10, 20, 40};
    int time = 0, done = 0;
    int last_boost_time = 0;

    Queue queues[NUM_QUEUES];
    for (int i = 0; i < NUM_QUEUES; i++)
        init_queue(&queues[i]);

    for (int i = 0; i < n; i++)
    {
        p[i].remaining = p[i].burst;
        p[i].queue_level = 0;
        p[i].start = -1;
        p[i].completed = false;
        p[i].used_time = 0;
    }

    int arrived[MAX] = {0}; // 프로세스가 큐에 들어갔는지 확인

    while (done < n)
    {
        // 새로 도착한 프로세스를 큐에 삽입
        for (int i = 0; i < n; i++)
        {
            if (!arrived[i] && p[i].arrival <= time)
            {
                enqueue(&queues[0], i);
                arrived[i] = 1;
            }
        }

        // Priority Boost
        if (time - last_boost_time >= BOOST_INTERVAL)
        {
            for (int i = 0; i < n; i++)
            {
                if (!p[i].completed)
                {
                    p[i].queue_level = 0;
                    enqueue(&queues[0], i);
                }
            }
            last_boost_time = time;

            for (int i = 1; i < NUM_QUEUES; i++)
                init_queue(&queues[i]); // 낮은 큐 비우기
        }

        bool executed = false;

        // 각 큐 순서대로 처리
        for (int q = 0; q < NUM_QUEUES; q++)
        {
            if (!is_empty(&queues[q]))
            {
                int idx = dequeue(&queues[q]);
                Process *current = &p[idx];

                int slice = (current->remaining < quantum[q]) ? current->remaining : quantum[q];

                if (current->start == -1)
                    current->start = time;

                current->remaining -= slice;
                current->used_time += slice;
                time += slice;

                if (current->remaining == 0)
                {
                    current->finish = time;
                    current->turnaround = current->finish - current->arrival;
                    current->waiting = current->start - current->arrival;
                    current->completed = true;
                    done++;
                }
                else
                {
                    // 강등 조건 확인
                    if (current->used_time >= quantum[q])
                    {
                        if (current->queue_level < NUM_QUEUES - 1)
                            current->queue_level++;
                        current->used_time = 0;
                    }
                    enqueue(&queues[current->queue_level], idx);
                }

                (*context_switches)++;
                executed = true;
                break; // 한 번에 하나의 프로세스만 처리
            }
        }

        if (!executed)
            time++; // 아무도 실행 안 했으면 시간 1 증가
    }
}

int load_processes(Process p[], const char *filename)
{
    FILE *fp = fopen(filename, "r");
    if (!fp)
    {
        perror("파일 열기 실패");
        return 0;
    }

    int count = 0;
    while (fscanf(fp, "%d %d %d", &p[count].pid, &p[count].arrival, &p[count].burst) == 3)
    {
        count++;
        if (count >= MAX)
            break;
    }

    fclose(fp);
    return count;
}

int main()
{
    Process p[MAX];
    int n = load_processes(p, "processes.txt");
    if (n == 0)
    {
        printf("프로세스를 불러오지 못했습니다.\n");
        return 1;
    }

    int context_switches = 0;
    mlfq_with_rr_queues(p, n, &context_switches);
    calculate_averages(p, n, context_switches);

    return 0;
}