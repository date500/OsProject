#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>

#define MAX 6000
#define BIG_NUMBER 100000
#define TIME_QUANTUM 4
#define NUM_QUEUES 3

typedef struct
{
    int pid;
    int arrival, burst, remaining;
    int start, finish, waiting, turnaround;
    int tickets, stride, pass, vruntime, queue_level;
    bool completed;
} Process;

void calculate_averages(Process p[], int n)
{
    float total_waiting = 0, total_turnaround = 0, total_response = 0;
    for (int i = 0; i < n; i++)
    {
        total_waiting += p[i].waiting;
        total_turnaround += p[i].turnaround;
        total_response += p[i].start - p[i].arrival;
    }
    printf("Response: %.2f  Waiting: %.2f  Turnaround: %.2f\n",
           total_response / n, total_waiting / n, total_turnaround / n);
}

void print_context_switches(int count)
{
    printf("Context Switches: %d\n", count);
}

void fcfs(Process p[], int n, int *context_switch_count)
{
    int time = 0, prev = -1;
    for (int i = 0; i < n; i++)
        for (int j = i + 1; j < n; j++)
            if (p[i].arrival > p[j].arrival)
            {
                Process tmp = p[i];
                p[i] = p[j];
                p[j] = tmp;
            }

    for (int i = 0; i < n; i++)
    {
        if (time < p[i].arrival)
            time = p[i].arrival;
        if (prev != -1 && prev != p[i].pid)
            (*context_switch_count)++;
        p[i].start = time;
        p[i].finish = time + p[i].burst;
        p[i].turnaround = p[i].finish - p[i].arrival;
        p[i].waiting = p[i].turnaround - p[i].burst;
        p[i].completed = true;
        time = p[i].finish;
        prev = p[i].pid;
    }
}

void sjf(Process p[], int n, int *context_switch_count)
{
    int time = 0, done = 0, prev = -1;
    while (done < n)
    {
        int idx = -1, min = INT_MAX;
        for (int i = 0; i < n; i++)
            if (!p[i].completed && p[i].arrival <= time && p[i].burst < min)
                min = p[i].burst, idx = i;
        if (idx == -1)
        {
            time++;
            continue;
        }

        if (prev != -1 && prev != p[idx].pid)
            (*context_switch_count)++;
        p[idx].start = time;
        time += p[idx].burst;
        p[idx].finish = time;
        p[idx].turnaround = time - p[idx].arrival;
        p[idx].waiting = p[idx].turnaround - p[idx].burst;
        p[idx].completed = true;
        done++;
        prev = p[idx].pid;
    }
}

void srtf(Process p[], int n, int *context_switch_count)
{
    int time = 0, done = 0, prev = -1;
    for (int i = 0; i < n; i++)
        p[i].remaining = p[i].burst;

    while (done < n)
    {
        int idx = -1, min = INT_MAX;
        for (int i = 0; i < n; i++)
            if (!p[i].completed && p[i].arrival <= time && p[i].remaining < min)
                min = p[i].remaining, idx = i;
        if (idx == -1)
        {
            time++;
            continue;
        }

        if (prev != -1 && prev != idx)
            (*context_switch_count)++;
        if (p[idx].remaining == p[idx].burst)
            p[idx].start = time;
        p[idx].remaining--;
        time++;
        prev = idx;

        if (p[idx].remaining == 0)
        {
            p[idx].finish = time;
            p[idx].turnaround = time - p[idx].arrival;
            p[idx].waiting = p[idx].turnaround - p[idx].burst;
            p[idx].completed = true;
            done++;
        }
    }
}

void round_robin(Process p[], int n, int quantum, int *context_switch_count)
{
    int time = 0, done = 0, prev = -1;
    int queue[MAX * 10], front = 0, rear = 0;
    bool in_queue[MAX] = {false};
    for (int i = 0; i < n; i++)
        p[i].remaining = p[i].burst;

    for (int i = 0; i < n; i++)
        if (p[i].arrival == 0)
        {
            queue[rear++] = i;
            in_queue[i] = true;
        }

    while (done < n)
    {
        if (front == rear)
        {
            time++;
            for (int i = 0; i < n; i++)
                if (!in_queue[i] && p[i].arrival <= time)
                {
                    queue[rear++] = i;
                    in_queue[i] = true;
                }
            continue;
        }

        int idx = queue[front++];
        if (prev != -1 && prev != idx)
            (*context_switch_count)++;

        int exec = p[idx].remaining < quantum ? p[idx].remaining : quantum;
        if (p[idx].remaining == p[idx].burst)
            p[idx].start = time > p[idx].arrival ? time : p[idx].arrival;
        time = time > p[idx].arrival ? time : p[idx].arrival;
        time += exec;
        p[idx].remaining -= exec;

        for (int i = 0; i < n; i++)
            if (!in_queue[i] && p[i].arrival <= time)
            {
                queue[rear++] = i;
                in_queue[i] = true;
            }

        if (p[idx].remaining == 0)
        {
            p[idx].finish = time;
            p[idx].turnaround = p[idx].finish - p[idx].arrival;
            p[idx].waiting = p[idx].turnaround - p[idx].burst;
            p[idx].completed = true;
            done++;
        }
        else
        {
            queue[rear++] = idx;
        }
        prev = idx;
    }
}

void stride(Process p[], int n, int *context_switch_count)
{
    int time = 0, done = 0, prev = -1;
    for (int i = 0; i < n; i++)
    {
        p[i].remaining = p[i].burst;
        p[i].tickets = (p[i].tickets > 0) ? p[i].tickets : 1;
        p[i].stride = BIG_NUMBER / p[i].tickets;
        p[i].pass = 0;
        p[i].start = -1;
    }

    while (done < n)
    {
        // pass가 가장 작은 실행 가능한 프로세스 찾기
        int idx = -1, min_pass = INT_MAX;
        for (int i = 0; i < n; i++)
        {
            if (!p[i].completed && p[i].arrival <= time && p[i].pass < min_pass)
            {
                min_pass = p[i].pass;
                idx = i;
            }
        }

        if (idx == -1)
        { // 실행할 게 없으면 시간 경과
            time++;
            continue;
        }

        if (prev != -1 && prev != idx)
            (*context_switch_count)++;

        if (p[idx].start == -1)
            p[idx].start = time;

        // 한 번에 1만큼 실행 (preemptive)
        p[idx].remaining--;
        time++;
        p[idx].pass += p[idx].stride;
        prev = idx;

        if (p[idx].remaining == 0)
        {
            p[idx].finish = time;
            p[idx].turnaround = time - p[idx].arrival;
            p[idx].waiting = p[idx].turnaround - p[idx].burst;
            p[idx].completed = true;
            done++;
        }
    }
}

void lottery(Process p[], int n, int quantum, int *context_switch_count)
{
    int time = 0, done = 0, prev = -1;
    for (int i = 0; i < n; i++)
    {
        p[i].remaining = p[i].burst;
        p[i].tickets = (rand() % 10) + 1;
        p[i].start = -1;
    }

    while (done < n)
    {
        int total_tickets = 0;
        for (int i = 0; i < n; i++)
            if (!p[i].completed && p[i].arrival <= time)
                total_tickets += p[i].tickets;

        if (total_tickets == 0)
        {
            time++;
            continue;
        }

        int winner = rand() % total_tickets, sum = 0, idx = -1;
        for (int i = 0; i < n; i++)
        {
            if (!p[i].completed && p[i].arrival <= time)
            {
                sum += p[i].tickets;
                if (sum > winner)
                {
                    idx = i;
                    break;
                }
            }
        }
        if (idx == -1)
            continue;

        if (prev != -1 && prev != idx)
            (*context_switch_count)++;

        if (p[idx].remaining == p[idx].burst)
            p[idx].start = time;

        int exec = (p[idx].remaining < quantum) ? p[idx].remaining : quantum;
        p[idx].remaining -= exec;
        time += exec;

        if (p[idx].remaining == 0)
        {
            p[idx].finish = time;
            p[idx].turnaround = time - p[idx].arrival;
            p[idx].waiting = p[idx].turnaround - p[idx].burst;
            p[idx].completed = true;
            done++;
        }
        prev = idx;
    }
}

void mlfq(Process p[], int n, int *context_switch_count)
{
    int quantum[NUM_QUEUES] = {4, 8, 16};
    int time = 0, done = 0, prev = -1;
    for (int i = 0; i < n; i++)
    {
        p[i].remaining = p[i].burst;
        p[i].queue_level = 0;
    }

    while (done < n)
    {
        int idx = -1;
        for (int q = 0; q < NUM_QUEUES; q++)
        {
            for (int i = 0; i < n; i++)
                if (!p[i].completed && p[i].arrival <= time && p[i].queue_level == q)
                {
                    idx = i;
                    break;
                }
            if (idx != -1)
                break;
        }

        if (idx == -1)
        {
            time++;
            continue;
        }
        if (prev != -1 && prev != p[idx].pid)
            (*context_switch_count)++;

        p[idx].start = p[idx].start == -1 ? time : p[idx].start;
        int slice = p[idx].remaining < quantum[p[idx].queue_level] ? p[idx].remaining : quantum[p[idx].queue_level];
        p[idx].remaining -= slice;
        time += slice;

        if (p[idx].remaining == 0)
        {
            p[idx].finish = time;
            p[idx].turnaround = time - p[idx].arrival;
            p[idx].waiting = p[idx].turnaround - p[idx].burst;
            p[idx].completed = true;
            done++;
        }
        else if (p[idx].queue_level < NUM_QUEUES - 1)
            p[idx].queue_level++;
        prev = p[idx].pid;
    }
}

void cfs(Process p[], int n, int *context_switch_count)
{
    int time = 0, done = 0, prev = -1;
    for (int i = 0; i < n; i++)
    {
        p[i].remaining = p[i].burst;
        p[i].tickets = p[i].tickets > 0 ? p[i].tickets : 1;
        p[i].vruntime = 0;
    }

    while (done < n)
    {
        int idx = -1, min = INT_MAX;
        for (int i = 0; i < n; i++)
            if (!p[i].completed && p[i].arrival <= time && p[i].vruntime < min)
                min = p[i].vruntime, idx = i;
        if (idx == -1)
        {
            time++;
            continue;
        }

        if (prev != -1 && prev != p[idx].pid)
            (*context_switch_count)++;

        p[idx].start = p[idx].start == -1 ? time : p[idx].start;
        p[idx].remaining--;
        p[idx].vruntime += 1024 / p[idx].tickets;
        time++;

        if (p[idx].remaining == 0)
        {
            p[idx].finish = time;
            p[idx].turnaround = time - p[idx].arrival;
            p[idx].waiting = p[idx].turnaround - p[idx].burst;
            p[idx].completed = true;
            done++;
        }
        prev = p[idx].pid;
    }
}

int load_processes(Process p[], const char *filename)
{
    FILE *fp = fopen(filename, "r");
    if (!fp)
        return 0;
    int count = 0;
    while (fscanf(fp, "%d %d %d", &p[count].pid, &p[count].arrival, &p[count].burst) == 3)
    {
        p[count].remaining = p[count].burst;
        p[count].start = -1;
        p[count].completed = false;
        p[count].tickets = (rand() % 10) + 1;
        count++;
        if (count >= MAX)
            break;
    }
    fclose(fp);
    return count;
}

void run_and_report(void (*scheduler)(Process[], int, int *), Process original[], int n, const char *name)
{
    Process temp[MAX];
    memcpy(temp, original, sizeof(Process) * n);
    int context_switch_count = 0;
    scheduler(temp, n, &context_switch_count);
    printf("\n[%s Scheduling]\n", name);
    calculate_averages(temp, n);
    print_context_switches(context_switch_count);
}

void run_and_report_rr(void (*scheduler)(Process[], int, int, int *), Process original[], int n, const char *name, int quantum)
{
    Process temp[MAX];
    memcpy(temp, original, sizeof(Process) * n);
    int context_switch_count = 0;
    scheduler(temp, n, quantum, &context_switch_count);
    printf("\n[%s (Quantum=%d)]\n", name, quantum);
    calculate_averages(temp, n);
    print_context_switches(context_switch_count);
}

int main()
{
    srand(time(NULL));
    Process p[MAX];
    int n = load_processes(p, "processes.txt");
    if (n == 0)
    {
        printf("파일 읽기 실패\n");
        return 1;
    }

    run_and_report(fcfs, p, n, "FCFS");
    run_and_report(sjf, p, n, "SJF");
    run_and_report(srtf, p, n, "SRTF");
    run_and_report_rr(round_robin, p, n, "Round Robin", 4);
    run_and_report(stride, p, n, "Stride");
    run_and_report_rr(lottery, p, n, "Lottery", 4);
    run_and_report(mlfq, p, n, "MLFQ");
    run_and_report(cfs, p, n, "CFS");

    return 0;
}
