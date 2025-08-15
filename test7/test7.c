/*
 * test7.c - Tests disabling PTE metadata without prior enable
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

static void test_disable_without_enable(uint8_t *buf, size_t ps)
{
    struct timespec start, end;
    double time_taken;
    
    printf("\n--- Disable Without Enable Test ---\n");
    
    printf("    Attempting to disable metadata on non-expanded page table...\n");
    
    // Try to disable metadata when page table is not expanded
    clock_gettime(CLOCK_MONOTONIC, &start);
    long r = syscall(SYS_disable_pte_meta, (unsigned long)buf);
    clock_gettime(CLOCK_MONOTONIC, &end);
    time_taken = get_time_diff(&start, &end);
    print_timing("disable_pte_meta", time_taken);
    
    // Based on the syscall implementation, it should return -EINVAL when PEN bit is not set
    if (r == 0) {
        fprintf(stderr, "    ✗ Expected failure, but syscall succeeded\n");
        exit(EXIT_FAILURE);
    }
    
    if (r < 0) {
        if (errno == EINVAL) {
            printf("    ✓ disable_pte_meta returned EINVAL as expected (errno=%d)\n", errno);
            printf("    ✓ This confirms page table was not expanded\n");
        } else {
            fprintf(stderr, "    ✗ Expected EINVAL, got: %s (errno=%d)\n", 
                    strerror(errno), errno);
            exit(EXIT_FAILURE);
        }
    }
    
    verify_pattern("disable attempt", buf, ps);
}

int main(void)
{
    struct timespec start, end;
    double time_taken;
    size_t ps = sysconf(_SC_PAGESIZE);
    uint8_t *buf;

    printf("\n=== Test7: Disable Metadata Without Enable Test ===\n\n");

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
    
    printf("    Note: NOT calling enable_pte_meta - page table remains standard 4KiB\n");

    // Section 2: Test disabling without enable
    test_disable_without_enable(buf, ps);

    // Section 3: Final verification
    printf("\n--- Final Verification ---\n");
    verify_pattern("final check", buf, ps);
    printf("    ✓ Test completed successfully\n");

    munlock(buf, ps);
    free(buf);

    return 0;
} 