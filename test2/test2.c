/*
 * test2.c - Tests PTE metadata with MDP=0 (direct u64)
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <errno.h>
#include <time.h>
#include <string.h>

#define SYS_enable_pte_meta   469
#define SYS_disable_pte_meta  470
#define SYS_set_pte_meta      471
#define SYS_get_pte_meta      472

#define META_VALUE_DIRECT 0xCAFEBABEDEADBEEFULL

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

static double get_time_diff(struct timespec *start, struct timespec *end)
{
    return ((end->tv_sec - start->tv_sec) * 1e9) + 
           (end->tv_nsec - start->tv_nsec);
}

static void print_timing(const char *operation, double nanoseconds)
{
    printf("    %-20s: %10.0f ns\n", operation, nanoseconds);
}

static void test_mdp0(uint8_t *buf, size_t ps)
{
    struct timespec start, end;
    double time_taken;
    uint64_t retrieved_meta;
    uint64_t meta_value = META_VALUE_DIRECT;
    
    printf("\n--- MDP=0 Test (Direct u64) ---\n");
    
    // Enable metadata for this page table
    clock_gettime(CLOCK_MONOTONIC, &start);
    call_or_die_1(SYS_enable_pte_meta, (unsigned long)buf, "enable_pte_meta");
    clock_gettime(CLOCK_MONOTONIC, &end);
    time_taken = get_time_diff(&start, &end);
    print_timing("enable_pte_meta", time_taken);
    
    printf("    Setting metadata: 0x%llx\n", (unsigned long long)meta_value);
    
    // Set metadata with MDP=0
    clock_gettime(CLOCK_MONOTONIC, &start);
    call_or_die_3(SYS_set_pte_meta, (unsigned long)buf, 0, 
                  (unsigned long)&meta_value, "set_pte_meta MDP=0");
    clock_gettime(CLOCK_MONOTONIC, &end);
    time_taken = get_time_diff(&start, &end);
    print_timing("set_pte_meta MDP=0", time_taken);
    
    verify_pattern("set_pte_meta MDP=0", buf, ps);
    
    // Get metadata with MDP=0
    clock_gettime(CLOCK_MONOTONIC, &start);
    long r = syscall(SYS_get_pte_meta, (unsigned long)buf, &retrieved_meta);
    clock_gettime(CLOCK_MONOTONIC, &end);
    time_taken = get_time_diff(&start, &end);
    print_timing("get_pte_meta MDP=0", time_taken);
    
    if (r < 0) {
        perror("get_pte_meta MDP=0");
        exit(EXIT_FAILURE);
    }
    
    printf("    Retrieved metadata: 0x%llx\n", (unsigned long long)retrieved_meta);
    
    if (retrieved_meta != meta_value) {
        fprintf(stderr, "    ✗ MDP=0: expected meta=0x%llx, got meta=0x%llx\n",
                (unsigned long long)meta_value, (unsigned long long)retrieved_meta);
        exit(EXIT_FAILURE);
    }
    
    printf("    ✓ MDP=0 metadata verified successfully\n");
}

int main(void)
{
    struct timespec start, end;
    double time_taken;
    size_t ps = sysconf(_SC_PAGESIZE);
    uint8_t *buf;

    printf("\n=== Test2: PTE Metadata Test (MDP=0 Direct u64) ===\n\n");

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

    // Section 2: Test MDP=0
    test_mdp0(buf, ps);

    // Section 3: Cleanup
    printf("\n--- Cleanup ---\n");
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    call_or_die_1(SYS_disable_pte_meta, (unsigned long)buf, "disable_pte_meta");
    clock_gettime(CLOCK_MONOTONIC, &end);
    time_taken = get_time_diff(&start, &end);
    print_timing("disable_pte_meta", time_taken);
    
    verify_pattern("disable_pte_meta", buf, ps);
    printf("    ✓ Test completed successfully\n");

    munlock(buf, ps);
    free(buf);

    return 0;
} 