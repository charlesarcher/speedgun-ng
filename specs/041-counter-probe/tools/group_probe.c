
// group_probe.c - the requirement-1 measurement of specs/041-counter-probe.
//
// It opens one group of k copies of one general-purpose event and asks the
// kernel what it grants. It prints the table the spec states as a fact, the
// size where a group open refuses, and the cost of the linear probe.
//
// Build and run:
//   cc -O2 -o group_probe group_probe.c
//   taskset -c 0,1 ./group_probe [event_config] [max_k]
//
// Real perf_event_open calls. No speedgun-ng code. No unit runs it.
#define _GNU_SOURCE
#include <cpuid.h>
#include <errno.h>
#include <linux/perf_event.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

#define MAX_K 96

struct counters { long open_ok; long open_fail; long enable; long disable; long read; long close; };
static struct counters tally;

static int perf_open(uint64_t config, int group_fd)
{
    struct perf_event_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.type = PERF_TYPE_HARDWARE;
    attr.size = sizeof(attr);
    attr.config = config;
    attr.exclude_kernel = 1;
    attr.exclude_hv = 1;
    attr.disabled = 1;
    attr.pinned = 0;
    attr.read_format = PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING;
    int fd = (int)syscall(__NR_perf_event_open, &attr, 0, -1, group_fd, 0);
    if (fd < 0) { ++tally.open_fail; } else { ++tally.open_ok; }
    return fd;
}

static void proc_line(const char *path, const char *label)
{
    FILE *f = fopen(path, "r");
    char line[128];
    line[0] = 0;
    if (f != NULL) {
        if (fgets(line, sizeof(line), f) == NULL) { line[0] = 0; }
        fclose(f);
    }
    line[strcspn(line, "\n")] = 0;
    printf("%s %s\n", label, line[0] ? line : "unreadable");
}

static long long now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

static int probe_size(int k, uint64_t config, unsigned long long *enabled, unsigned long long *running)
{
    int fd[MAX_K];
    int opened = 0;
    *enabled = 0;
    *running = 0;
    for (int i = 0; i < k; ++i) {
        int one = perf_open(config, opened == 0 ? -1 : fd[0]);
        if (one < 0) {
            for (int j = 0; j < opened; ++j) { close(fd[j]); ++tally.close; }
            return -1;
        }
        fd[opened++] = one;
    }
    for (int i = 0; i < opened; ++i) {
        if (ioctl(fd[i], PERF_EVENT_IOC_ENABLE, 0) == 0) { ++tally.enable; }
    }
    unsigned long long buf[3];
    buf[0] = 0; buf[1] = 0; buf[2] = 0;
    ssize_t n = (ssize_t)read(fd[0], buf, sizeof(buf));
    if (n == (ssize_t)sizeof(buf)) { *enabled = buf[1]; *running = buf[2]; ++tally.read; }
    for (int i = 0; i < opened; ++i) {
        if (ioctl(fd[i], PERF_EVENT_IOC_DISABLE, 0) == 0) { ++tally.disable; }
    }
    for (int i = 0; i < opened; ++i) { close(fd[i]); ++tally.close; }
    if (n != (ssize_t)sizeof(buf)) { return -1; }
    if (*enabled > 0 && *running == *enabled) { return 1; }
    return 0;
}

int main(int argc, char **argv)
{
    uint64_t config = PERF_COUNT_HW_BRANCH_INSTRUCTIONS;
    int max_k = 32;
    char hostname[HOST_NAME_MAX];
    unsigned eax, ebx, ecx, edx;
    int last_fit = 0;
    int refused_at = 0;
    int answer = 0;
    int opens_used = 0;
    long long t0, cost_ns;
    struct counters before;

    if (argc > 1) { config = strtoull(argv[1], NULL, 0); }
    if (argc > 2) { max_k = atoi(argv[2]); if (max_k > MAX_K) { max_k = MAX_K; } }

    hostname[0] = 0;
    gethostname(hostname, sizeof(hostname) - 1);
    printf("# Requirement-1 measurement: what one group of k copies gets\n\n");
    printf("host %s\n", hostname[0] ? hostname : "unknown");
    proc_line("/proc/sys/kernel/ostype", "kernel.ostype");
    proc_line("/proc/sys/kernel/osrelease", "kernel.osrelease");
    proc_line("/proc/sys/kernel/perf_event_paranoid", "kernel.perf_event_paranoid");
    proc_line("/proc/sys/kernel/nmi_watchdog", "kernel.nmi_watchdog");
    printf("event config %llu, type PERF_TYPE_HARDWARE, exclude_kernel 1, disabled 1, pinned 0\n",
           (unsigned long long)config);
    if (__get_cpuid(0x80000022, &eax, &ebx, &ecx, &edx)) {
        printf("cpuid leaf 0x80000022 eax 0x%08x ebx 0x%08x ecx 0x%08x edx 0x%08x (raw words; %s decodes them)\n",
               eax, ebx, ecx, edx, "counter_limits.cpp");
    }
    if (__get_cpuid(0x0A, &eax, &ebx, &ecx, &edx)) {
        printf("cpuid leaf 0x0A eax 0x%08x ebx 0x%08x ecx 0x%08x edx 0x%08x (raw words)\n", eax, ebx, ecx, edx);
    }

    printf("\n## Table 1: one group of k copies, k = 1 to %d\n\n", max_k);
    printf("| k | open | errno | time_enabled_ns | time_running_ns | runs fully |\n");
    printf("|---|------|-------|-----------------|-----------------|------------|\n");
    for (int k = 1; k <= max_k; ++k) {
        unsigned long long en, rn;
        int r = probe_size(k, config, &en, &rn);
        if (r < 0) { printf("| %d | refused | %s | - | - | - |\n", k, strerror(errno)); continue; }
        if (r == 1) { last_fit = k; }
        printf("| %d | ok | - | %llu | %llu | %s |\n", k, en, rn, (r == 1) ? "yes" : "no");
    }
    printf("\nLargest size that runs fully: %d\n", last_fit);

    printf("\n## Table 2: where does a group open refuse?\n\n");
    for (int k = 1; k <= MAX_K && refused_at == 0; ++k) {
        int fd[MAX_K];
        int opened = 0;
        for (int i = 0; i < k; ++i) {
            int one = perf_open(config, opened == 0 ? -1 : fd[0]);
            if (one < 0) {
                printf("First refusal at k = %d, member %d: %s\n", k, i, strerror(errno));
                refused_at = k;
                break;
            }
            fd[opened++] = one;
        }
        for (int i = 0; i < opened; ++i) { close(fd[i]); ++tally.close; }
    }
    if (refused_at == 0) { printf("No refusal up to k = %d. Every group open answered 0.\n", MAX_K); }

    printf("\n## Table 3: the linear probe, and what it costs\n\n");
    t0 = now_ns();
    before = tally;
    for (int k = 1; k <= max_k; ++k) {
        unsigned long long en, rn;
        long before_ok = tally.open_ok;
        int r = probe_size(k, config, &en, &rn);
        opens_used += (int)(tally.open_ok - before_ok);
        if (r != 1) { answer = k - 1; break; }
    }
    cost_ns = now_ns() - t0;
    printf("Probe answer (largest size that runs fully): %d\n", answer);
    printf("Sizes opened: %d, opens paid: %d, enables %ld, reads %ld, closes %ld\n",
           answer + 1, opens_used, tally.enable - before.enable, tally.read - before.read,
           tally.close - before.close);
    printf("Probe wall time: %.3f ms (CLOCK_MONOTONIC; a script reads the clock, no unit does)\n",
           (double)cost_ns / 1000000.0);
    printf("\nTotals: opens ok %ld, opens refused %ld, enables %ld, disables %ld, reads %ld, closes %ld\n",
           tally.open_ok, tally.open_fail, tally.enable, tally.disable, tally.read, tally.close);
    printf("Descriptors left open at exit: %ld (closes minus opens; must be 0)\n",
           tally.close - tally.open_ok - tally.open_fail);
    return 0;
}
