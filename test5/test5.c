/*
 * test5.c - Tests PTE metadata on multiple pages within same page table
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

#define NUM_PAGES 4  // Test 4 pages within same page table

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

static void test_multiple_pages(uint8_t **pages, size_t ps)
{
    struct timespec start, end;
    double time_taken;
    
    printf("\n--- Multiple Page Metadata Test ---\n");
    
    // Enable metadata for the first page (this expands the entire page table)
    printf("    Enabling metadata for page table (using page 0)...\n");
    clock_gettime(CLOCK_MONOTONIC, &start);
    call_or_die_1(SYS_enable_pte_meta, (unsigned long)pages[0], "enable_pte_meta");
    clock_gettime(CLOCK_MONOTONIC, &end);
    time_taken = get_time_diff(&start, &end);
    print_timing("enable_pte_meta", time_taken);
    
    // Set different metadata for each page
    printf("    Setting unique metadata for each page...\n");
    for (int i = 0; i < NUM_PAGES; i++) {
        uint64_t meta_value = 0xDEADBEEF00000000ULL | i;  // Unique value per page
        
        char syscall_name[32];
        snprintf(syscall_name, sizeof(syscall_name), "set_pte_meta page %d", i);
        
        clock_gettime(CLOCK_MONOTONIC, &start);
        call_or_die_3(SYS_set_pte_meta, (unsigned long)pages[i], 0, 
                      (unsigned long)&meta_value, syscall_name);
        clock_gettime(CLOCK_MONOTONIC, &end);
        time_taken = get_time_diff(&start, &end);
        
        char timing_name[32];
        snprintf(timing_name, sizeof(timing_name), "set_pte_meta pg%d", i);
        print_timing(timing_name, time_taken);
        
        printf("      Page %d: set metadata 0x%llx\n", i, (unsigned long long)meta_value);
        verify_pattern(syscall_name, pages[i], ps);
    }
    
    // Verify each page has its correct metadata
    printf("    Verifying metadata for each page...\n");
    for (int i = 0; i < NUM_PAGES; i++) {
        uint64_t expected_meta = 0xDEADBEEF00000000ULL | i;
        uint64_t retrieved_meta;
        
        char syscall_name[32];
        snprintf(syscall_name, sizeof(syscall_name), "get_pte_meta page %d", i);
        
        clock_gettime(CLOCK_MONOTONIC, &start);
        long r = syscall(SYS_get_pte_meta, (unsigned long)pages[i], &retrieved_meta);
        clock_gettime(CLOCK_MONOTONIC, &end);
        time_taken = get_time_diff(&start, &end);
        
        char timing_name[32];
        snprintf(timing_name, sizeof(timing_name), "get_pte_meta pg%d", i);
        print_timing(timing_name, time_taken);
        
        if (r < 0) {
            fprintf(stderr, "    ✗ Page %d: get_pte_meta failed: %s\n", i, strerror(errno));
            exit(EXIT_FAILURE);
        }
        
        if (retrieved_meta != expected_meta) {
            fprintf(stderr, "    ✗ Page %d: expected 0x%llx, got 0x%llx\n", 
                    i, (unsigned long long)expected_meta, (unsigned long long)retrieved_meta);
            exit(EXIT_FAILURE);
        }
        
        printf("      Page %d: verified metadata 0x%llx ✓\n", i, (unsigned long long)retrieved_meta);
        verify_pattern(syscall_name, pages[i], ps);
    }
    
    printf("    ✓ All pages have correct independent metadata\n");
}

int main(void)
{
    struct timespec start, end;
    double time_taken;
    size_t ps = sysconf(_SC_PAGESIZE);
    uint8_t *pages[NUM_PAGES];

    printf("\n=== Test5: Multiple Page Metadata Test ===\n\n");

    // Section 1: Setup
    printf("--- Setup ---\n");
    printf("    Allocating %d consecutive pages...\n", NUM_PAGES);
    
    // Allocate multiple consecutive pages
    clock_gettime(CLOCK_MONOTONIC, &start);
    uint8_t *base_addr;
    if (posix_memalign((void **)&base_addr, ps, ps * NUM_PAGES) != 0) {
        perror("posix_memalign");
        exit(EXIT_FAILURE);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    time_taken = get_time_diff(&start, &end);
    print_timing("Multi-page allocation", time_taken);
    
    // Set up page pointers
    for (int i = 0; i < NUM_PAGES; i++) {
        pages[i] = base_addr + (i * ps);
    }
    
    mlock(base_addr, ps * NUM_PAGES);

    // Fill each page with a unique pattern
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < NUM_PAGES; i++) {
        fill(pages[i], ps);
        printf("    Page %d at %p: filled with pattern\n", i, (void*)pages[i]);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    time_taken = get_time_diff(&start, &end);
    print_timing("Pattern writing all", time_taken);
    
    // Verify initial patterns
    for (int i = 0; i < NUM_PAGES; i++) {
        char stage_name[32];
        snprintf(stage_name, sizeof(stage_name), "initial fill page %d", i);
        verify_pattern(stage_name, pages[i], ps);
    }

    // Section 2: Test multiple page metadata
    test_multiple_pages(pages, ps);

    // Section 3: Cleanup
    printf("\n--- Cleanup ---\n");
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    call_or_die_1(SYS_disable_pte_meta, (unsigned long)pages[0], "disable_pte_meta");
    clock_gettime(CLOCK_MONOTONIC, &end);
    time_taken = get_time_diff(&start, &end);
    print_timing("disable_pte_meta", time_taken);
    
    // Verify patterns are still intact after disable
    for (int i = 0; i < NUM_PAGES; i++) {
        char stage_name[32];
        snprintf(stage_name, sizeof(stage_name), "post-disable page %d", i);
        verify_pattern(stage_name, pages[i], ps);
    }
    
    printf("    ✓ Test completed successfully\n");

    munlock(base_addr, ps * NUM_PAGES);
    free(base_addr);

    return 0;
} 