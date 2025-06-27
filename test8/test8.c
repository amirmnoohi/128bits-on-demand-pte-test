/*
 * test8.c - Tests comparing timing between first and second set operations
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

int main(void)
{
    struct timespec start, end;
    double time_taken;
    size_t ps = sysconf(_SC_PAGESIZE);
    uint8_t *buf;

    printf("\n=== Test8: PTE Meta Set/Get Timing Comparison Test ===\n\n");

    // Section 1: Timing Measurements
    printf("--- Timing Measurements ---\n");
    
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

    // First set operation
    clock_gettime(CLOCK_MONOTONIC, &start);
    ret = syscall(SYS_set_pte_meta, (unsigned long)buf, 0xcafebabe, 1);
    clock_gettime(CLOCK_MONOTONIC, &end);
    time_taken = get_time_diff(&start, &end);
    print_timing("First set_pte_meta", time_taken);

    if (ret != 0) {
        fprintf(stderr, "    ✗ set_pte_meta failed with error: %s\n", 
                strerror(-ret));
        exit(EXIT_FAILURE);
    }

    // First get operation
    clock_gettime(CLOCK_MONOTONIC, &start);
    long raw = syscall(SYS_get_pte_meta, (unsigned long)buf);
    clock_gettime(CLOCK_MONOTONIC, &end);
    time_taken = get_time_diff(&start, &end);
    print_timing("First get_pte_meta", time_taken);

    if (errno == EINVAL || errno == EPERM) {
        fprintf(stderr, "    ✗ get_pte_meta failed with error: %s\n", 
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    int type = (raw >> 63) & 1;
    uint64_t meta = raw & ((1ULL << 63) - 1);
    printf("    ✓ First PTE meta get successful (type: %d, meta: 0x%llx)\n", 
           type, (unsigned long long)meta);

    // Second set operation
    clock_gettime(CLOCK_MONOTONIC, &start);
    ret = syscall(SYS_set_pte_meta, (unsigned long)buf, 0xdeadbeef, 1);
    clock_gettime(CLOCK_MONOTONIC, &end);
    time_taken = get_time_diff(&start, &end);
    print_timing("Second set_pte_meta", time_taken);

    if (ret != 0) {
        fprintf(stderr, "    ✗ set_pte_meta failed with error: %s\n", 
                strerror(-ret));
        exit(EXIT_FAILURE);
    }

    // Second get operation
    clock_gettime(CLOCK_MONOTONIC, &start);
    raw = syscall(SYS_get_pte_meta, (unsigned long)buf);
    clock_gettime(CLOCK_MONOTONIC, &end);
    time_taken = get_time_diff(&start, &end);
    print_timing("Second get_pte_meta", time_taken);

    if (errno == EINVAL || errno == EPERM) {
        fprintf(stderr, "    ✗ get_pte_meta failed with error: %s\n", 
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    type = (raw >> 63) & 1;
    meta = raw & ((1ULL << 63) - 1);
    printf("    ✓ Second PTE meta get successful (type: %d, meta: 0x%llx)\n", 
           type, (unsigned long long)meta);

    // Section 2: Results and Verification
    printf("\n--- Results and Verification ---\n");
    
    verify_pattern("initial fill", buf, ps);

    munlock(buf, ps);
    free(buf);

    return 0;
} 