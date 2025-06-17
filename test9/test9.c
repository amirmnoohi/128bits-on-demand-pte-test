/*
 * test9.c - Tests PTE meta operations with 1000 iterations and statistics
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define SYS_enable_pte_meta   469
#define SYS_disable_pte_meta  470
#define SYS_set_pte_meta      471
#define SYS_get_pte_meta      472

#define ITERATIONS 10000
#define META_VALUE 0xCAFEBABEULL
#define META_TYPE  1  // Set bit 58 to 1 (pointer)

static void fill(uint8_t *b, size_t n)
{ for (size_t i = 0; i < n; ++i) b[i] = (uint8_t)(i & 0xFF); }

static void verify_pattern(const char *stage, const uint8_t *b, size_t n)
{
    for (size_t i = 0; i < n; ++i)
        if (b[i] != (uint8_t)(i & 0xFF)) {
            fprintf(stderr, "[%s] mismatch @%zu (0x%02x)\n", stage, i, b[i]);
            exit(EXIT_FAILURE);
        }
    printf("    ✓ verification successful after %s\n", stage);
}

static double get_time_diff(struct timespec *start, struct timespec *end)
{
    return ((end->tv_sec - start->tv_sec) * 1e9) + 
           (end->tv_nsec - start->tv_nsec);
}

static void print_timing(const char *operation, double nanoseconds)
{
    printf("    %-20s: %10.0f ns\n", operation, nanoseconds);
}

static void calculate_stats(double *times, int n, double *min, double *max, 
                          double *mean, double *stddev)
{
    double sum = 0.0, sum_sq = 0.0;
    *min = times[0];
    *max = times[0];

    for (int i = 0; i < n; i++) {
        sum += times[i];
        sum_sq += times[i] * times[i];
        if (times[i] < *min) *min = times[i];
        if (times[i] > *max) *max = times[i];
    }

    *mean = sum / n;
    *stddev = sqrt((sum_sq / n) - (*mean * *mean));
}

int main(void)
{
    struct timespec start, end;
    double time_taken;
    size_t ps = sysconf(_SC_PAGESIZE);
    uint8_t *buf;
    double enable_times[ITERATIONS];
    double set_times[ITERATIONS];
    double get_times[ITERATIONS];
    double disable_times[ITERATIONS];
    int i;

    printf("\n=== Test9: PTE Meta Operations Statistics Test ===\n\n");

    // Section 1: Initial Setup
    printf("--- Initial Setup ---\n");
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    int ret = posix_memalign((void **)&buf, ps, ps);
    if (ret != 0) {
        fprintf(stderr, "    ✗ Failed to allocate page aligned memory: %s\n", strerror(ret));
        exit(EXIT_FAILURE);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    time_taken = get_time_diff(&start, &end);
    print_timing("Page allocation", time_taken);

    mlock(buf, ps);

    clock_gettime(CLOCK_MONOTONIC, &start);
    fill(buf, ps);
    clock_gettime(CLOCK_MONOTONIC, &end);
    time_taken = get_time_diff(&start, &end);
    print_timing("Pattern writing", time_taken);

    // Section 2: Iterative Operations
    printf("\n--- Iterative Operations (%d iterations) ---\n", ITERATIONS);
    
    for (i = 0; i < ITERATIONS; i++) {
        // Enable
        clock_gettime(CLOCK_MONOTONIC, &start);
        ret = syscall(SYS_enable_pte_meta, (unsigned long)buf, 0, 0);
        clock_gettime(CLOCK_MONOTONIC, &end);
        enable_times[i] = get_time_diff(&start, &end);
        if (ret != 0) {
            fprintf(stderr, "    ✗ enable_pte_meta failed with error: %s\n", 
                    strerror(-ret));
            exit(EXIT_FAILURE);
        }

        // Set
        clock_gettime(CLOCK_MONOTONIC, &start);
        ret = syscall(SYS_set_pte_meta, (unsigned long)buf, META_VALUE, META_TYPE);
        clock_gettime(CLOCK_MONOTONIC, &end);
        set_times[i] = get_time_diff(&start, &end);
        if (ret != 0) {
            fprintf(stderr, "    ✗ set_pte_meta failed with error: %s\n", 
                    strerror(-ret));
            exit(EXIT_FAILURE);
        }

        // Get
        clock_gettime(CLOCK_MONOTONIC, &start);
        long raw = syscall(SYS_get_pte_meta, (unsigned long)buf);
        clock_gettime(CLOCK_MONOTONIC, &end);
        get_times[i] = get_time_diff(&start, &end);
        if (errno == EINVAL || errno == EPERM) {
            fprintf(stderr, "    ✗ get_pte_meta failed with error: %s\n", 
                    strerror(errno));
            exit(EXIT_FAILURE);
        }

        // Verify meta value
        int type = (raw >> 63) & 1;
        uint64_t meta = raw & ((1ULL << 63) - 1);
        if (type != META_TYPE || meta != META_VALUE) {
            fprintf(stderr, "    ✗ meta verification failed (type: %d, meta: 0x%llx)\n",
                    type, (unsigned long long)meta);
            exit(EXIT_FAILURE);
        }

        // Disable
        clock_gettime(CLOCK_MONOTONIC, &start);
        ret = syscall(SYS_disable_pte_meta, (unsigned long)buf, 0, 0);
        clock_gettime(CLOCK_MONOTONIC, &end);
        disable_times[i] = get_time_diff(&start, &end);
        if (ret != 0) {
            fprintf(stderr, "    ✗ disable_pte_meta failed with error: %s\n", 
                    strerror(-ret));
            exit(EXIT_FAILURE);
        }
    }

    // Section 3: Statistics
    printf("\n--- Statistics ---\n");
    
    double min, max, mean, stddev;

    calculate_stats(enable_times, ITERATIONS, &min, &max, &mean, &stddev);
    printf("Enable PTE Meta:\n");
    printf("    Min:     %10.0f ns\n", min);
    printf("    Max:     %10.0f ns\n", max);
    printf("    Mean:    %10.0f ns\n", mean);
    printf("    StdDev:  %10.0f ns\n", stddev);

    calculate_stats(set_times, ITERATIONS, &min, &max, &mean, &stddev);
    printf("\nSet PTE Meta:\n");
    printf("    Min:     %10.0f ns\n", min);
    printf("    Max:     %10.0f ns\n", max);
    printf("    Mean:    %10.0f ns\n", mean);
    printf("    StdDev:  %10.0f ns\n", stddev);

    calculate_stats(get_times, ITERATIONS, &min, &max, &mean, &stddev);
    printf("\nGet PTE Meta:\n");
    printf("    Min:     %10.0f ns\n", min);
    printf("    Max:     %10.0f ns\n", max);
    printf("    Mean:    %10.0f ns\n", mean);
    printf("    StdDev:  %10.0f ns\n", stddev);

    calculate_stats(disable_times, ITERATIONS, &min, &max, &mean, &stddev);
    printf("\nDisable PTE Meta:\n");
    printf("    Min:     %10.0f ns\n", min);
    printf("    Max:     %10.0f ns\n", max);
    printf("    Mean:    %10.0f ns\n", mean);
    printf("    StdDev:  %10.0f ns\n", stddev);

    // Section 4: Final Verification
    printf("\n--- Final Verification ---\n");
    verify_pattern("iterations", buf, ps);
    printf("    ✓ All %d iterations completed successfully\n", ITERATIONS);

    munlock(buf, ps);
    free(buf);

    return 0;
} 