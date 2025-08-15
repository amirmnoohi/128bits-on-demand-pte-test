/*
 * test6.c - Tests enabling and disabling PTE metadata
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

static void test_enable_disable_cycle(uint8_t *buf, size_t ps)
{
    struct timespec start, end;
    double time_taken;
    
    printf("\n--- Enable/Disable Cycle Test ---\n");
    
    // Test multiple enable/disable cycles
    for (int cycle = 1; cycle <= 3; cycle++) {
        printf("    Cycle %d:\n", cycle);
        
        // Enable metadata
        clock_gettime(CLOCK_MONOTONIC, &start);
        call_or_die_1(SYS_enable_pte_meta, (unsigned long)buf, "enable_pte_meta");
        clock_gettime(CLOCK_MONOTONIC, &end);
        time_taken = get_time_diff(&start, &end);
        
        char timing_name[32];
        snprintf(timing_name, sizeof(timing_name), "enable cycle %d", cycle);
        print_timing(timing_name, time_taken);
        
        printf("      ✓ Enabled successfully\n");
        verify_pattern("enable cycle", buf, ps);
        
        // Disable metadata  
        clock_gettime(CLOCK_MONOTONIC, &start);
        call_or_die_1(SYS_disable_pte_meta, (unsigned long)buf, "disable_pte_meta");
        clock_gettime(CLOCK_MONOTONIC, &end);
        time_taken = get_time_diff(&start, &end);
        
        snprintf(timing_name, sizeof(timing_name), "disable cycle %d", cycle);
        print_timing(timing_name, time_taken);
        
        printf("      ✓ Disabled successfully\n");
        verify_pattern("disable cycle", buf, ps);
    }
    
    printf("    ✓ All enable/disable cycles completed successfully\n");
}

static void test_double_enable_error(uint8_t *buf, size_t ps)
{
    struct timespec start, end;
    double time_taken;
    
    printf("\n--- Double Enable Error Test ---\n");
    
    // First enable should succeed
    printf("    First enable (should succeed)...\n");
    clock_gettime(CLOCK_MONOTONIC, &start);
    call_or_die_1(SYS_enable_pte_meta, (unsigned long)buf, "enable_pte_meta");
    clock_gettime(CLOCK_MONOTONIC, &end);
    time_taken = get_time_diff(&start, &end);
    print_timing("first enable", time_taken);
    printf("      ✓ First enable succeeded\n");
    
    // Second enable should fail with -EEXIST
    printf("    Second enable (should fail with EEXIST)...\n");
    clock_gettime(CLOCK_MONOTONIC, &start);
    long r = syscall(SYS_enable_pte_meta, (unsigned long)buf);
    clock_gettime(CLOCK_MONOTONIC, &end);
    time_taken = get_time_diff(&start, &end);
    print_timing("second enable", time_taken);
    
    if (r == 0) {
        fprintf(stderr, "    ✗ Second enable should have failed but succeeded\n");
        exit(EXIT_FAILURE);
    }
    
    if (errno == EEXIST) {
        printf("      ✓ Second enable failed with EEXIST as expected\n");
    } else {
        fprintf(stderr, "    ✗ Expected EEXIST, got: %s (errno=%d)\n", 
                strerror(errno), errno);
        exit(EXIT_FAILURE);
    }
    
    verify_pattern("double enable test", buf, ps);
    
    // Clean up - disable metadata
    call_or_die_1(SYS_disable_pte_meta, (unsigned long)buf, "cleanup disable");
    printf("    ✓ Cleanup completed\n");
}

int main(void)
{
    struct timespec start, end;
    double time_taken;
    size_t ps = sysconf(_SC_PAGESIZE);
    uint8_t *buf;

    printf("\n=== Test6: PTE Metadata Enable/Disable Test ===\n\n");

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

    // Section 2: Test enable/disable cycles
    test_enable_disable_cycle(buf, ps);

    // Section 3: Test double enable error
    test_double_enable_error(buf, ps);

    // Section 4: Final verification
    printf("\n--- Final Verification ---\n");
    verify_pattern("final check", buf, ps);
    printf("    ✓ All tests completed successfully\n");

    munlock(buf, ps);
    free(buf);

    return 0;
} 