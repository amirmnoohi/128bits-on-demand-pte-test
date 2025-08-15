/*
 * test4.c - Tests getting PTE metadata when page table is not expanded
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

static void test_get_without_expansion(uint8_t *buf, size_t ps)
{
    struct timespec start, end;
    double time_taken;
    uint64_t buffer = 0;  // Buffer for get_pte_meta
    
    printf("\n--- Test: Get Metadata Without Expansion ---\n");
    
    printf("    Attempting to get metadata from non-expanded page table...\n");
    
    // Try to get metadata when page table is not expanded
    clock_gettime(CLOCK_MONOTONIC, &start);
    long r = syscall(SYS_get_pte_meta, (unsigned long)buf, &buffer);
    clock_gettime(CLOCK_MONOTONIC, &end);
    time_taken = get_time_diff(&start, &end);
    print_timing("get_pte_meta", time_taken);
    
    // Based on the updated syscall implementation, it should return -ENODATA when PEN bit is not set
    if (r == 0) {
        fprintf(stderr, "    ✗ Expected failure, but syscall succeeded\n");
        fprintf(stderr, "    ✗ Buffer value: 0x%llx\n", (unsigned long long)buffer);
        exit(EXIT_FAILURE);
    }
    
    if (r < 0) {
        if (errno == ENODATA) {
            printf("    ✓ get_pte_meta returned ENODATA as expected (errno=%d)\n", errno);
            printf("    ✓ This confirms page table was not expanded\n");
        } else {
            fprintf(stderr, "    ✗ Expected ENODATA, got: %s (errno=%d)\n", 
                    strerror(errno), errno);
            exit(EXIT_FAILURE);
        }
    }
    
    verify_pattern("get_pte_meta attempt", buf, ps);
}

int main(void)
{
    struct timespec start, end;
    double time_taken;
    size_t ps = sysconf(_SC_PAGESIZE);
    uint8_t *buf;

    printf("\n=== Test4: PTE Metadata Test (Not Expanded) ===\n\n");

    // Section 1: Setup
    printf("--- Setup ---\n");
    
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
    
    verify_pattern("initial fill", buf, ps);
    
    printf("    Note: NOT calling enable_pte_meta - page table remains standard 4KiB\n");

    // Section 2: Test getting metadata without expansion
    test_get_without_expansion(buf, ps);

    // Section 3: Cleanup (no disable needed since we never enabled)
    printf("\n--- Cleanup ---\n");
    printf("    No disable_pte_meta needed - page table was never expanded\n");
    printf("    ✓ Test completed successfully\n");

    munlock(buf, ps);
    free(buf);

    return 0;
} 