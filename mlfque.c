#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define MAX 6000
#define NUM_QUEUES 3
#define BOOST_INTERVAL 150
#define STARVATION_THRESHOLD 1000

// Process 구조체 정의
typedef struct
{
    int pid;                       // 프로세스 ID
    int arrival, burst, remaining; // 도착 시간, 실행 시간, 남은 실행 시간
    int start, finish;             // 시작 시간, 종료 시간
    int waiting, turnaround;       // 대기 시간, 반환 시간
    int queue_level;               // 큐 레벨
    int used_time;                 // 사용한 시간
    bool completed;                // 프로세스 완료 여부
} Process;

// Queue 구조체 정의
typedef struct
{
    int items[MAX];  // 큐 요소들
    int front, rear; // 큐의 앞, 뒤 포인터
} Queue;

// -------------------- Queue Utilities --------------------

// 큐 초기화 함수
void init_queue(Queue *q)
{
    q->front = q->rear = 0;
}

// 큐가 비었는지 확인하는 함수
bool is_empty(Queue *q)
{
    return q->front == q->rear;
}

// 큐에 값 추가하는 함수
void enqueue(Queue *q, int value)
{
    q->items[q->rear++] = value;
}

// 큐에서 값 제거하는 함수
int dequeue(Queue *q)
{
    return q->items[q->front++];
}

// -------------------- Round Robin 처리 --------------------

// Round Robin 스케줄링을 처리하는 함수
bool round_robin_queue(Process p[], Queue *q, int quantum, int *time, int *context_switches, Queue queues[])
{
    if (is_empty(q))
        return false;

    int idx = dequeue(q); // 큐에서 프로세스 인덱스 꺼내기
    Process *current = &p[idx];

    // 프로세스가 처음 실행되면 start 시간을 기록
    if (current->start == -1)
        current->start = *time;

    int slice = (current->remaining < quantum) ? current->remaining : quantum; // 슬라이스 크기 결정

    current->remaining -= slice;
    current->used_time += slice;
    *time += slice;

    // 프로세스 완료 처리
    if (current->remaining == 0)
    {
        current->finish = *time;
        current->turnaround = current->finish - current->arrival;
        current->waiting = current->start - current->arrival;
        current->completed = true;
    }
    else
    {
        // 큐 레벨 업그레이드 및 큐에 재삽입
        if (current->used_time >= quantum)
        {
            if (current->queue_level < NUM_QUEUES - 1)
                current->queue_level++;
            current->used_time = 0;
        }
        enqueue(&queues[current->queue_level], idx);
    }

    (*context_switches)++; // 컨텍스트 스위치 증가
    return true;
}

// -------------------- MLFQ 컨트롤러 --------------------

// MLFQ 스케줄링 알고리즘을 처리하는 함수
void mlfq_with_boost(Process p[], int n, int *context_switches)
{
    int quantum[NUM_QUEUES] = {10, 20, 40}; // 각 큐별 quantum 값
    int time = 0, done = 0, last_boost_time = 0;

    int first_process_arrival = p[0].arrival;
    time = first_process_arrival;

    Queue queues[NUM_QUEUES];
    for (int i = 0; i < NUM_QUEUES; i++)
        init_queue(&queues[i]);

    // 프로세스 초기화
    for (int i = 0; i < n; i++)
    {
        p[i].remaining = p[i].burst;
        p[i].queue_level = 0;
        p[i].start = -1;
        p[i].completed = false;
        p[i].used_time = 0;
    }

    int arrived[MAX] = {0};

    while (done < n)
    {
        // 도착한 프로세스를 큐에 삽입
        for (int i = 0; i < n; i++)
        {
            if (!arrived[i] && p[i].arrival <= time)
            {
                enqueue(&queues[0], i);
                arrived[i] = 1;
            }
        }

        // Priority Boost 처리
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
            // 큐 리셋
            for (int i = 1; i < NUM_QUEUES; i++)
                init_queue(&queues[i]);
            last_boost_time = time;
        }

        bool executed = false;
        // 각 큐에서 프로세스를 실행
        for (int q = 0; q < NUM_QUEUES; q++)
        {
            if (round_robin_queue(p, &queues[q], quantum[q], &time, context_switches, queues))
            {
                executed = true;
                if (p[queues[q].items[queues[q].front - 1]].completed)
                    done++;
                break;
            }
        }

        if (!executed)
            time++; // 아무도 실행하지 않았다면 시간 증가
    }
}

// -------------------- 통계 출력 --------------------

// 평균 대기 시간, 반환 시간, 응답 시간 계산 및 출력
void calculate_averages(Process p[], int n, int context_switches)
{
    float total_waiting = 0, total_turnaround = 0, total_response = 0;
    int starvation_count = 0;

    for (int i = 0; i < n; i++)
    {
        total_waiting += p[i].waiting;
        total_turnaround += p[i].turnaround;
        total_response += p[i].start - p[i].arrival; // Response = start - arrival

        if (p[i].waiting > STARVATION_THRESHOLD)
            starvation_count++;
    }

    // 결과 출력
    printf("\n[MLFQ 결과]\n");
    printf("Response: %.2f  Waiting: %.2f  Turnaround: %.2f\n",
           total_response / n, total_waiting / n, total_turnaround / n);
    printf("Total Context Switches: %d\n", context_switches);
    printf("Starvation Rate: %.2f%%\n", (float)starvation_count / n * 100);
}

// -------------------- 프로세스 로드 --------------------

// 파일에서 프로세스를 로드하는 함수
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

// -------------------- main --------------------

// 메인 함수
int main()
{
    Process p[MAX];
    int n = load_processes(p, "processes.txt"); // 프로세스 로드
    if (n == 0)
    {
        printf("프로세스를 불러오지 못했습니다.\n");
        return 1;
    }

    int context_switches = 0;
    mlfq_with_boost(p, n, &context_switches);   // MLFQ 스케줄링 실행
    calculate_averages(p, n, context_switches); // 통계 출력

    return 0;
}
