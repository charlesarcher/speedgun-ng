
// probe_window.c: how short an enabled window still tells a fitting group from
// an oversize one. No clock reads: the window is a fixed-iteration spin.
#define _GNU_SOURCE
#include <errno.h>
#include <linux/perf_event.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <unistd.h>

static uint64_t sink;

static int perf_open(uint64_t config, int group_fd, uint64_t rf)
{
    struct perf_event_attr a;
    memset(&a, 0, sizeof(a));
    a.type = PERF_TYPE_HARDWARE; a.size = sizeof(a); a.config = config;
    a.exclude_kernel = 1; a.exclude_hv = 1; a.disabled = 1; a.pinned = 0;
    a.read_format = rf;
    return (int)syscall(__NR_perf_event_open, &a, 0, -1, group_fd, 0);
}

static void spin(unsigned n)
{
    for (unsigned i = 0; i < n; ++i) { sink = sink * 6364136223846793005ULL + 1; }
}

/* Returns: 1 fit (running == enabled > 0), 0 no-fit (running < enabled), -1 error, -2 zero window. */
static int trial(int k, unsigned spin_iters, uint64_t rf)
{
    int fds[40]; int opened = 0;
    for (int i = 0; i < k; ++i) {
        int fd = perf_open(PERF_COUNT_HW_BRANCH_INSTRUCTIONS, i == 0 ? -1 : fds[0], rf);
        if (fd < 0) { for (int j = 0; j < opened; ++j) close(fds[j]); return -1; }
        fds[opened++] = fd;
    }
    for (int i = 0; i < opened; ++i) { ioctl(fds[i], PERF_EVENT_IOC_ENABLE, 0); }
    spin(spin_iters);
    uint64_t buf[3] = {0,0,0};
    ssize_t n = read(fds[0], buf, sizeof(buf));
    for (int i = 0; i < opened; ++i) { ioctl(fds[i], PERF_EVENT_IOC_DISABLE, 0); }
    n = read(fds[0], buf, sizeof(buf));
    for (int i = 0; i < opened; ++i) { close(fds[i]); }
    if (n != 24) { return -1; }
    if (buf[1] == 0) { return -2; }
    return (buf[2] == buf[1]) ? 1 : 0;
}

int main(void)
{
    const uint64_t rf = PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING;
    const unsigned spins[] = {0, 100, 1000, 10000};
    const int ks[] = {1, 4, 5, 6, 7, 8, 12};
    const int runs = 50;

    printf("## F: enable, spin N iterations, disable, read (50 trials per cell)\n");
    printf("spin_iters\tk\tfit\tno_fit\terror\tzero_window\n");
    for (size_t s = 0; s < sizeof(spins)/sizeof(*spins); ++s) {
        for (size_t ki = 0; ki < sizeof(ks)/sizeof(*ks); ++ki) {
            int fit = 0, nofit = 0, err = 0, zero = 0;
            for (int r = 0; r < runs; ++r) {
                int t = trial(ks[ki], spins[s], rf);
                if (t == 1) ++fit; else if (t == 0) ++nofit; else if (t == -2) ++zero; else ++err;
            }
            printf("%u\t%d\t%d\t%d\t%d\t%d\n", spins[s], ks[ki], fit, nofit, err, zero);
        }
    }

    printf("\n## G: one read while still enabled, right after the enable ioctl\n");
    printf("k\tread_while_enabled_running_eq_enabled_gt0\trunning_lt_enabled\n");
    for (size_t ki = 0; ki < sizeof(ks)/sizeof(*ks); ++ki) {
        int eq = 0, lt = 0;
        for (int r = 0; r < runs; ++r) {
            int k = ks[ki]; int fds[40]; int opened = 0; int bad = 0;
            for (int i = 0; i < k; ++i) {
                int fd = perf_open(PERF_COUNT_HW_BRANCH_INSTRUCTIONS, i == 0 ? -1 : fds[0], rf);
                if (fd < 0) { bad = 1; break; }
                fds[opened++] = fd;
            }
            if (bad) { for (int i = 0; i < opened; ++i) close(fds[i]); continue; }
            for (int i = 0; i < opened; ++i) { ioctl(fds[i], PERF_EVENT_IOC_ENABLE, 0); }
            uint64_t buf[3] = {0,0,0};
            ssize_t n = read(fds[0], buf, sizeof(buf));
            for (int i = 0; i < opened; ++i) { ioctl(fds[i], PERF_EVENT_IOC_DISABLE, 0); }
            for (int i = 0; i < opened; ++i) { close(fds[i]); }
            if (n != 24) { continue; }
            if (buf[1] > 0 && buf[2] == buf[1]) ++eq; else if (buf[2] < buf[1]) ++lt;
        }
        printf("%d\t%d\t%d\n", ks[ki], eq, lt);
    }
    return 0;
}
