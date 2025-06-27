/*
 * test4.c - Tests getting PTE meta when not expanded
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

int main(void)
{
    struct timespec start, end;
    double time_taken;
    size_t ps = sysconf(_SC_PAGESIZE);
    uint8_t *buf;

    printf("\n=== Test4: PTE Meta Not Expanded Test ===\n\n");

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
    syscall(SYS_get_pte_meta, (unsigned long)buf);
    clock_gettime(CLOCK_MONOTONIC, &end);
    time_taken = get_time_diff(&start, &end);
    print_timing("get_pte_meta", time_taken);

    // Section 2: Results and Verification
    printf("\n--- Results and Verification ---\n");
    
    verify_pattern("initial fill", buf, ps);

    if (errno != ENODATA) {
        fprintf(stderr, "    ✗ expected ENODATA (61), got errno: %d (%s)\n", 
                errno, strerror(errno));
        exit(EXIT_FAILURE);
    }
    printf("    ✓ PTE meta get returned ENODATA as expected\n");

    munlock(buf, ps);
    free(buf);

    return 0;
} 