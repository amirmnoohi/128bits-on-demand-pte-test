/*
 * test6.c - Tests enabling and disabling PTE meta
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
    return (end->tv_sec - start->tv_sec) + 
           (end->tv_nsec - start->tv_nsec) / 1e9;
}

static void print_timing(const char *operation, double seconds)
{
    printf("    %-20s: %10.6f seconds\n", operation, seconds);
}

int main(void)
{
    struct timespec start, end;
    double time_taken;
    size_t ps = sysconf(_SC_PAGESIZE);
    uint8_t *buf;

    printf("\n=== Test6: PTE Meta Enable/Disable Test ===\n\n");

    // Section 1: Timing Measurements
    printf("--- Timing Measurements ---\n");
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    posix_memalign((void **)&buf, ps, ps);
    clock_gettime(CLOCK_MONOTONIC, &end);
    time_taken = get_time_diff(&start, &end);
    print_timing("Page allocation", time_taken);

    mlock(buf, ps);

    clock_gettime(CLOCK_MONOTONIC, &start);
    fill(buf, ps);
    clock_gettime(CLOCK_MONOTONIC, &end);
    time_taken = get_time_diff(&start, &end);
    print_timing("Pattern writing", time_taken);

    clock_gettime(CLOCK_MONOTONIC, &start);
    syscall(SYS_enable_pte_meta, (unsigned long)buf, 0, 0);
    clock_gettime(CLOCK_MONOTONIC, &end);
    time_taken = get_time_diff(&start, &end);
    print_timing("enable_pte_meta", time_taken);

    clock_gettime(CLOCK_MONOTONIC, &start);
    syscall(SYS_disable_pte_meta, (unsigned long)buf, 0, 0);
    clock_gettime(CLOCK_MONOTONIC, &end);
    time_taken = get_time_diff(&start, &end);
    print_timing("disable_pte_meta", time_taken);

    // Section 2: Results and Verification
    printf("\n--- Results and Verification ---\n");
    
    verify_pattern("initial fill", buf, ps);

    long ret = syscall(SYS_enable_pte_meta, (unsigned long)buf, 0, 0);
    if (ret != 0) {
        fprintf(stderr, "    ✗ enable_pte_meta failed with error: %s\n", 
                strerror(-ret));
        exit(EXIT_FAILURE);
    }
    printf("    ✓ PTE meta enabled successfully\n");
    verify_pattern("enable_pte_meta", buf, ps);

    ret = syscall(SYS_disable_pte_meta, (unsigned long)buf, 0, 0);
    if (ret != 0) {
        fprintf(stderr, "    ✗ disable_pte_meta failed with error: %s\n", 
                strerror(-ret));
        exit(EXIT_FAILURE);
    }
    printf("    ✓ PTE meta disabled successfully\n");
    verify_pattern("disable_pte_meta", buf, ps);

    munlock(buf, ps);
    free(buf);

    return 0;
} 