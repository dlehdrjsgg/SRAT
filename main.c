#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
    #include <psapi.h>
#elif defined(__APPLE__) && defined(__MACH__)
    #include <sys/sysctl.h>
    #include <mach/mach.h>
    #include <sys/mount.h>
#endif

#define CPU_THRESHOLD 75.0
#define MEM_THRESHOLD 75.0
#define DISK_THRESHOLD 75.0
#define ERROR_MARGIN 5.0
#define MONITOR_INTERVAL 1

#define RESET_COLOR "\x1B[0m"
#define GREEN_COLOR "\x1B[32m"
#define YELLOW_COLOR "\x1B[33m"
#define RED_COLOR "\x1B[31m"

void checkCpuUsage();
void checkMemoryUsage();
void checkDiskUsage();
void printUsageBar(double usage, double threshold, char* resourceName);

int main() {
    printf("[시스템 자원 모니터링 - AirDrop ]\n\n");

    printf("모니터링할 시스템 자원을 선택하세요:\n");
    printf("1. CPU 사용량\n");
    printf("2. 메모리 사용량\n");
    printf("3. 디스크 사용량\n");
    printf("선택(예시: 123): ");
    
    char userInput[4];
    scanf("%s", userInput);

    bool monitorCpu = false, monitorMemory = false, monitorDisk = false;

    for (int i = 0; i < strlen(userInput); ++i) {
        switch (userInput[i]) {
            case '1':
                monitorCpu = true;
                break;
            case '2':
                monitorMemory = true;
                break;
            case '3':
                monitorDisk = true;
                break;
            default:
                printf("잘못된 선택입니다: %c\n", userInput[i]);
                break;
        }
    }

    while (true) {
        #if defined(_WIN32) || defined(_WIN64)
            system("cls");
        #elif defined(__APPLE__) && defined(__MACH__)
            system("clear");
        #endif

        if (monitorCpu) {
            checkCpuUsage();
        }
        if (monitorMemory) {
            checkMemoryUsage();
        }
        if (monitorDisk) {
            checkDiskUsage();
        }

        printf("\n============================================================================\n");
        sleep(MONITOR_INTERVAL);
    }

    return 0;
}

void printUsageBar(double usage, double threshold, char* resourceName) {
    int barLength = 50;
    int usageLow = (int)(usage - ERROR_MARGIN);
    int usageHigh = (int)(usage + ERROR_MARGIN);
    const char* color;

    if (usageHigh < threshold) {
        color = GREEN_COLOR;
    } else if (usageLow <= threshold && usageHigh >= threshold) {
        color = YELLOW_COLOR;
    } else {
        color = RED_COLOR;
    }

    printf("============================================================================\n");
    printf("%s%s 사용량: %.2f%% (오차 범위: %d%% - %d%%)%s\n", color, resourceName, usage, usageLow, usageHigh, RESET_COLOR);

    int numBlocks = (int)(usage / 100.0 * barLength);
    printf("%s [", color);
    for (int i = 0; i < barLength; ++i) {
        putchar(i < numBlocks ? '#' : ' ');
    }
    printf("] %.2f%%%s\n", usage, RESET_COLOR);

    if (strcmp(resourceName, "CPU") == 0) {
        printf("\n[추가 정보]\n");
        printf("CPU 사용량이 높을 경우, 운영 체제는 프로세스 실행 우선순위와 자원 할당을 조절하여 시스템 성능을 최적화합니다.\n");
        printf("Windows에서는 라운드 로빈, 최소 작업 우선순위, 다중 큐 등의 알고리즘을 사용하여 CPU 자원을 효율적으로 관리합니다.\n");
    } else if (strcmp(resourceName, "Memory") == 0) {
        printf("\n[추가 정보]\n");
        printf("메모리 사용량이 높을 경우, 가상 메모리를 효율적으로 사용하고 페이징 기법을 통해 메모리 사용량을 최적화할 수 있습니다.\n");
    } else if (strcmp(resourceName, "Disk") == 0) {
        printf("\n[추가 정보]\n");
        printf("디스크 사용량이 높을 경우, 파일 시스템의 성능을 최적화하기 위해 디스크 조각 모음을 사용할 수 있습니다.\n");
    }
}

void checkCpuUsage() {
#if defined(_WIN32) || defined(_WIN64)
    FILETIME idleTime, kernelTime, userTime;
    static ULONGLONG prevIdle = 0, prevKernel = 0, prevUser = 0;
    ULONGLONG idleDelta, kernelDelta, userDelta, totalDelta;

    if (!GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
        printf("CPU 시간 정보를 가져오지 못했습니다.\n");
        return;
    }

    ULONGLONG idle = ((ULONGLONG)idleTime.dwHighDateTime << 32) | idleTime.dwLowDateTime;
    ULONGLONG kernel = ((ULONGLONG)kernelTime.dwHighDateTime << 32) | kernelTime.dwLowDateTime;
    ULONGLONG user = ((ULONGLONG)userTime.dwHighDateTime << 32) | userTime.dwLowDateTime;

    idleDelta = idle - prevIdle;
    kernelDelta = kernel - prevKernel;
    userDelta = user - prevUser;
    totalDelta = kernelDelta + userDelta;

    prevIdle = idle;
    prevKernel = kernel;
    prevUser = user;

    float cpuUsage = ((kernelDelta + userDelta - idleDelta) * 100.0) / totalDelta;

    printUsageBar(cpuUsage, CPU_THRESHOLD, "CPU");
#elif defined(__APPLE__) && defined(__MACH__)
    host_cpu_load_info_data_t cpuInfo;
    mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
    kern_return_t kr = host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO, (host_info_t)&cpuInfo, &count);

    if (kr != KERN_SUCCESS) {
        printf("CPU 시간 정보를 가져오지 못했습니다.\n");
        return;
    }

    static unsigned long long prevIdleTicks = 0, prevTotalTicks = 0;
    unsigned long long idleTicks = cpuInfo.cpu_ticks[CPU_STATE_IDLE];
    unsigned long long totalTicks = idleTicks + cpuInfo.cpu_ticks[CPU_STATE_USER] + cpuInfo.cpu_ticks[CPU_STATE_SYSTEM] + cpuInfo.cpu_ticks[CPU_STATE_NICE];

    unsigned long long idleTicksDelta = idleTicks - prevIdleTicks;
    unsigned long long totalTicksDelta = totalTicks - prevTotalTicks;

    prevIdleTicks = idleTicks;
    prevTotalTicks = totalTicks;

    float cpuUsage = (1.0 - (idleTicksDelta / (float)totalTicksDelta)) * 100.0;

    printUsageBar(cpuUsage, CPU_THRESHOLD, "CPU");
#endif
}

void checkMemoryUsage() {
#if defined(_WIN32) || defined(_WIN64)
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);

    if (!GlobalMemoryStatusEx(&memInfo)) {
        printf("메모리 통계 정보를 가져오지 못했습니다.\n");
        return;
    }

    float memUsage = ((float)(memInfo.ullTotalPhys - memInfo.ullAvailPhys) / memInfo.ullTotalPhys) * 100.0;

    printUsageBar(memUsage, MEM_THRESHOLD, "Memory");
#elif defined(__APPLE__) && defined(__MACH__)
    mach_msg_type_number_t count = HOST_VM_INFO_COUNT;
    vm_statistics64_data_t vmStats;
    kern_return_t kr = host_statistics64(mach_host_self(), HOST_VM_INFO, (host_info_t)&vmStats, &count);

    if (kr != KERN_SUCCESS) {
        printf("메모리 통계 정보를 가져오지 못했습니다.\n");
        return;
    }

    int64_t freeMemory = vm_page_size * vmStats.free_count;
    int64_t totalMemory = 0;
    size_t len = sizeof(totalMemory);
    sysctlbyname("hw.memsize", &totalMemory, &len, NULL, 0);

    float memUsage = ((float)(totalMemory - freeMemory) / totalMemory) * 100.0;

    printUsageBar(memUsage, MEM_THRESHOLD, "Memory");
#endif
}

void checkDiskUsage() {
#if defined(_WIN32) || defined(_WIN64)
    ULARGE_INTEGER freeBytesAvailable, totalBytes, totalFreeBytes;

    if (!GetDiskFreeSpaceEx("C:\\", &freeBytesAvailable, &totalBytes, &totalFreeBytes)) {
        printf("디스크 정보를 가져오지 못했습니다.\n");
        return;
    }

    float diskUsage = ((totalBytes.QuadPart - totalFreeBytes.QuadPart) / (float)totalBytes.QuadPart) * 100.0;

    printUsageBar(diskUsage, DISK_THRESHOLD, "Disk");
#elif defined(__APPLE__) && defined(__MACH__)
    struct statfs stats;

    if (statfs("/", &stats) != 0) {
        printf("디스크 정보를 가져오지 못했습니다.\n");
        return;
    }

    float diskUsage = ((stats.f_blocks - stats.f_bfree) / (float)stats.f_blocks) * 100.0;

    printUsageBar(diskUsage, DISK_THRESHOLD, "Disk");
#endif
}