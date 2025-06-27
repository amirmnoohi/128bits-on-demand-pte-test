/*
 * test3.c - Tests setting PTE meta with bit 58=1 (pointer)
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

#define SYS_set_pte_meta      471
#define SYS_get_pte_meta      472

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

static void call_or_die(long nr, unsigned long a1,
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

int main(void)
{
    struct timespec start, end;
    double time_taken;
    size_t ps = sysconf(_SC_PAGESIZE);
    uint8_t *buf;

    printf("\n=== Test3: PTE Meta Pointer Test ===\n\n");

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
    call_or_die(SYS_set_pte_meta, (unsigned long)buf,
                META_VALUE, META_TYPE, "set_pte_meta");
    clock_gettime(CLOCK_MONOTONIC, &end);
    time_taken = get_time_diff(&start, &end);
    print_timing("set_pte_meta", time_taken);

    clock_gettime(CLOCK_MONOTONIC, &start);
    long long raw = syscall(SYS_get_pte_meta, (unsigned long)buf);
    clock_gettime(CLOCK_MONOTONIC, &end);
    time_taken = get_time_diff(&start, &end);
    print_timing("get_pte_meta", time_taken);

    // Section 2: Results and Verification
    printf("\n--- Results and Verification ---\n");
    
    verify_pattern("initial fill", buf, ps);
    printf("    ✓ PTE meta set with bit 58=1\n");
    verify_pattern("set_pte_meta", buf, ps);

    if (raw == -EINVAL || raw == -EPERM) { 
        fprintf(stderr, "    ✗ get_pte_meta failed with error: %s\n", strerror(errno));
        exit(EXIT_FAILURE); 
    }

    int type = (raw >> 63) & 1;
    uint64_t meta = raw & ((1ULL << 63) - 1);
    
    if (type != META_TYPE) {
        fprintf(stderr, "    ✗ expected type=1 (pointer), got type=%d\n", type);
        exit(EXIT_FAILURE);
    }
    if (meta != META_VALUE) {
        fprintf(stderr, "    ✗ expected meta=0x%llx, got meta=0x%llx\n",
                (unsigned long long)META_VALUE, (unsigned long long)meta);
        exit(EXIT_FAILURE);
    }
    printf("    ✓ PTE meta verified (type: %d, meta: 0x%llx)\n", 
           type, (unsigned long long)meta);

    munlock(buf, ps);
    free(buf);

    return 0;
}