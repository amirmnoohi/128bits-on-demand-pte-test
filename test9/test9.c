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
#define META_VALUE_BASE 0xCAFEBABEDEADBEEFULL

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

static void call_or_die_1(long nr, unsigned long a1, const char *name)
{
    long r = syscall(nr, a1);
    if (r < 0) { perror(name); exit(EXIT_FAILURE); }
}

static void call_or_die_3(long nr, unsigned long a1,
                          unsigned long a2, unsigned long a3,
                          const char *name)
{
    long r = syscall(nr, a1, a2, a3);
    if (r < 0) { perror(name); exit(EXIT_FAILURE); }
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
    double set_times[ITERATIONS];
    double get_times[ITERATIONS];
    int i;

    printf("\n=== Test9: %d-Iteration Statistics Test ===\n\n", ITERATIONS);

    // Section 1: Setup
    printf("--- Setup ---\n");
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    if (posix_memalign((void **)&buf, ps, ps) != 0) {
        perror("posix_memalign");
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
    
    verify_pattern("initial fill", buf, ps);

    // Enable metadata once (more efficient than repeated enable/disable)
    printf("    Enabling metadata for page table...\n");
    clock_gettime(CLOCK_MONOTONIC, &start);
    call_or_die_1(SYS_enable_pte_meta, (unsigned long)buf, "enable_pte_meta");
    clock_gettime(CLOCK_MONOTONIC, &end);
    time_taken = get_time_diff(&start, &end);
    print_timing("enable_pte_meta", time_taken);

    // Section 2: Iterative Set/Get Operations
    printf("\n--- %d Set/Get Iterations ---\n", ITERATIONS);
    printf("    Progress: ");
    fflush(stdout);
    
    for (i = 0; i < ITERATIONS; i++) {
        // Show progress every 1000 iterations
        if (i % 1000 == 0) {
            printf("%d ", i);
            fflush(stdout);
        }
        
        // Create unique metadata for each iteration
        uint64_t meta_value = META_VALUE_BASE + i;
        uint64_t retrieved_meta;

        // Set metadata (MDP=0)
        clock_gettime(CLOCK_MONOTONIC, &start);
        call_or_die_3(SYS_set_pte_meta, (unsigned long)buf, 0, 
                      (unsigned long)&meta_value, "set_pte_meta");
        clock_gettime(CLOCK_MONOTONIC, &end);
        set_times[i] = get_time_diff(&start, &end);

        // Get metadata
        clock_gettime(CLOCK_MONOTONIC, &start);
        long r = syscall(SYS_get_pte_meta, (unsigned long)buf, &retrieved_meta);
        clock_gettime(CLOCK_MONOTONIC, &end);
        get_times[i] = get_time_diff(&start, &end);
        
        if (r < 0) {
            fprintf(stderr, "\n    ✗ get_pte_meta failed at iteration %d: %s\n", 
                    i, strerror(errno));
            exit(EXIT_FAILURE);
        }

        // Verify metadata value
        if (retrieved_meta != meta_value) {
            fprintf(stderr, "\n    ✗ meta verification failed at iteration %d\n", i);
            fprintf(stderr, "      expected: 0x%llx, got: 0x%llx\n",
                    (unsigned long long)meta_value, (unsigned long long)retrieved_meta);
            exit(EXIT_FAILURE);
        }
    }
    
    printf("%d\n    ✓ All iterations completed successfully\n", ITERATIONS);

    // Section 3: Statistics Analysis
    printf("\n--- Performance Statistics ---\n");
    
    double min, max, mean, stddev;

    calculate_stats(set_times, ITERATIONS, &min, &max, &mean, &stddev);
    printf("Set PTE Meta (%d iterations):\n", ITERATIONS);
    printf("    Min:     %10.0f ns\n", min);
    printf("    Max:     %10.0f ns\n", max);
    printf("    Mean:    %10.0f ns\n", mean);
    printf("    StdDev:  %10.0f ns\n", stddev);

    calculate_stats(get_times, ITERATIONS, &min, &max, &mean, &stddev);
    printf("\nGet PTE Meta (%d iterations):\n", ITERATIONS);
    printf("    Min:     %10.0f ns\n", min);
    printf("    Max:     %10.0f ns\n", max);
    printf("    Mean:    %10.0f ns\n", mean);
    printf("    StdDev:  %10.0f ns\n", stddev);

    // Performance analysis
    double total_set_time = 0, total_get_time = 0;
    for (i = 0; i < ITERATIONS; i++) {
        total_set_time += set_times[i];
        total_get_time += get_times[i];
    }
    
    printf("\nPerformance Summary:\n");
    printf("    Total set time:  %10.0f ms\n", total_set_time / 1e6);
    printf("    Total get time:  %10.0f ms\n", total_get_time / 1e6);
    printf("    Set throughput:  %10.0f ops/sec\n", ITERATIONS / (total_set_time / 1e9));
    printf("    Get throughput:  %10.0f ops/sec\n", ITERATIONS / (total_get_time / 1e9));

    // Section 4: Cleanup
    printf("\n--- Cleanup ---\n");
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    call_or_die_1(SYS_disable_pte_meta, (unsigned long)buf, "disable_pte_meta");
    clock_gettime(CLOCK_MONOTONIC, &end);
    time_taken = get_time_diff(&start, &end);
    print_timing("disable_pte_meta", time_taken);
    
    verify_pattern("final check", buf, ps);
    printf("    ✓ Test completed successfully\n");

    munlock(buf, ps);
    free(buf);

    return 0;
} 