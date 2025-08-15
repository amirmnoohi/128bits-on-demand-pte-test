/*
 * test8.c - Tests set/get timing comparison and performance analysis
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

#define SYS_enable_pte_meta   469
#define SYS_disable_pte_meta  470
#define SYS_set_pte_meta      471
#define SYS_get_pte_meta      472

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

static void test_timing_comparison(uint8_t *buf, size_t ps)
{
    struct timespec start, end;
    double first_set_time, second_set_time, first_get_time, second_get_time;
    uint64_t first_meta = 0xCAFEBABE12345678ULL;
    uint64_t second_meta = 0xDEADBEEF87654321ULL;
    uint64_t retrieved_meta;
    
    printf("\n--- Set/Get Timing Comparison ---\n");
    
    // First set operation (includes page table expansion)
    printf("    First set operation (with page table expansion)...\n");
    clock_gettime(CLOCK_MONOTONIC, &start);
    call_or_die_3(SYS_set_pte_meta, (unsigned long)buf, 0, 
                  (unsigned long)&first_meta, "first set_pte_meta");
    clock_gettime(CLOCK_MONOTONIC, &end);
    first_set_time = get_time_diff(&start, &end);
    print_timing("First set (expand)", first_set_time);
    
    verify_pattern("first set", buf, ps);
    
    // First get operation
    clock_gettime(CLOCK_MONOTONIC, &start);
    long r = syscall(SYS_get_pte_meta, (unsigned long)buf, &retrieved_meta);
    clock_gettime(CLOCK_MONOTONIC, &end);
    first_get_time = get_time_diff(&start, &end);
    print_timing("First get", first_get_time);
    
    if (r < 0) {
        fprintf(stderr, "    ✗ First get_pte_meta failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    if (retrieved_meta != first_meta) {
        fprintf(stderr, "    ✗ First get: expected 0x%llx, got 0x%llx\n",
                (unsigned long long)first_meta, (unsigned long long)retrieved_meta);
        exit(EXIT_FAILURE);
    }
    
    printf("      ✓ First metadata verified: 0x%llx\n", (unsigned long long)retrieved_meta);
    
    // Second set operation (page table already expanded)
    printf("    Second set operation (page table already expanded)...\n");
    clock_gettime(CLOCK_MONOTONIC, &start);
    call_or_die_3(SYS_set_pte_meta, (unsigned long)buf, 0, 
                  (unsigned long)&second_meta, "second set_pte_meta");
    clock_gettime(CLOCK_MONOTONIC, &end);
    second_set_time = get_time_diff(&start, &end);
    print_timing("Second set (update)", second_set_time);
    
    verify_pattern("second set", buf, ps);
    
    // Second get operation
    clock_gettime(CLOCK_MONOTONIC, &start);
    r = syscall(SYS_get_pte_meta, (unsigned long)buf, &retrieved_meta);
    clock_gettime(CLOCK_MONOTONIC, &end);
    second_get_time = get_time_diff(&start, &end);
    print_timing("Second get", second_get_time);
    
    if (r < 0) {
        fprintf(stderr, "    ✗ Second get_pte_meta failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    if (retrieved_meta != second_meta) {
        fprintf(stderr, "    ✗ Second get: expected 0x%llx, got 0x%llx\n",
                (unsigned long long)second_meta, (unsigned long long)retrieved_meta);
        exit(EXIT_FAILURE);
    }
    
    printf("      ✓ Second metadata verified: 0x%llx\n", (unsigned long long)retrieved_meta);
    
    // Performance analysis
    printf("\n    --- Performance Analysis ---\n");
    printf("      First set time:  %10.0f ns (includes expansion)\n", first_set_time);
    printf("      Second set time: %10.0f ns (update only)\n", second_set_time);
    printf("      First get time:  %10.0f ns\n", first_get_time);
    printf("      Second get time: %10.0f ns\n", second_get_time);
    
    if (first_set_time > second_set_time) {
        double speedup = first_set_time / second_set_time;
        printf("      ✓ Second set is %.1fx faster (expansion overhead removed)\n", speedup);
    } else {
        printf("      ⚠ Second set not significantly faster (timing variation)\n");
    }
    
    double avg_get_time = (first_get_time + second_get_time) / 2.0;
    printf("      Average get time: %10.0f ns\n", avg_get_time);
    
    printf("    ✓ Timing comparison completed successfully\n");
}

int main(void)
{
    struct timespec start, end;
    double time_taken;
    size_t ps = sysconf(_SC_PAGESIZE);
    uint8_t *buf;

    printf("\n=== Test8: Set/Get Timing Comparison Test ===\n\n");

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

    // Section 2: Timing comparison test
    test_timing_comparison(buf, ps);

    // Section 3: Cleanup
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