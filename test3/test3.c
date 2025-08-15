/*
 * test3.c - Tests PTE metadata with MDP=1 (structured buffer)
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

// Test payload for MDP=1 structured buffer
#define PAYLOAD_SIZE 16

/* Structure for MDP=1 buffer header */
struct metadata_header {
    uint16_t version;
    uint16_t type;
    uint32_t length;
} __attribute__((packed));

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

static void print_payload(const char *label, const uint8_t *payload, size_t length)
{
    printf("    %s payload: ", label);
    for (size_t i = 0; i < length; i++) {
        printf("%02x", payload[i]);
        if (i < length - 1) printf(" ");
    }
    printf("\n");
}



static void test_mdp1(uint8_t *buf, size_t ps)
{
    struct timespec start, end;
    double time_taken;
    
    printf("\n--- MDP=1 Test (Structured Buffer) ---\n");
    
    // Enable metadata for this page table
    clock_gettime(CLOCK_MONOTONIC, &start);
    call_or_die_1(SYS_enable_pte_meta, (unsigned long)buf, "enable_pte_meta");
    clock_gettime(CLOCK_MONOTONIC, &end);
    time_taken = get_time_diff(&start, &end);
    print_timing("enable_pte_meta", time_taken);
    
    // Prepare structured buffer for MDP=1
    struct metadata_header header = {
        .version = 1,
        .type = 0x1234,
        .length = PAYLOAD_SIZE
    };
    
    uint8_t payload[PAYLOAD_SIZE] = {0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE,
                                     0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
    
    // Create complete buffer (header + payload)
    size_t total_size = sizeof(header) + header.length;
    uint8_t *meta_buffer = malloc(total_size);
    if (!meta_buffer) {
        perror("malloc meta_buffer");
        exit(EXIT_FAILURE);
    }
    
    memcpy(meta_buffer, &header, sizeof(header));
    memcpy(meta_buffer + sizeof(header), payload, header.length);
    
    printf("    Setting metadata:\n");
    printf("      Header: version=%d, type=0x%x, length=%d\n", 
           header.version, header.type, header.length);
    print_payload("Original", payload, header.length);
    
    // Set metadata with MDP=1
    clock_gettime(CLOCK_MONOTONIC, &start);
    call_or_die_3(SYS_set_pte_meta, (unsigned long)buf, 1, 
                  (unsigned long)meta_buffer, "set_pte_meta MDP=1");
    clock_gettime(CLOCK_MONOTONIC, &end);
    time_taken = get_time_diff(&start, &end);
    print_timing("set_pte_meta MDP=1", time_taken);
    
    verify_pattern("set_pte_meta MDP=1", buf, ps);
    
    // Get metadata with MDP=1
    uint8_t *retrieved_buffer = malloc(total_size);
    if (!retrieved_buffer) {
        perror("malloc retrieved_buffer");
        exit(EXIT_FAILURE);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    long r = syscall(SYS_get_pte_meta, (unsigned long)buf, retrieved_buffer);
    clock_gettime(CLOCK_MONOTONIC, &end);
    time_taken = get_time_diff(&start, &end);
    print_timing("get_pte_meta MDP=1", time_taken);
    
    if (r < 0) {
        perror("get_pte_meta MDP=1");
        exit(EXIT_FAILURE);
    }
    
    // Verify retrieved structured buffer
    struct metadata_header *retrieved_header = (struct metadata_header *)retrieved_buffer;
    uint8_t *retrieved_payload = retrieved_buffer + sizeof(struct metadata_header);
    
    printf("    Retrieved metadata:\n");
    printf("      Header: version=%d, type=0x%x, length=%d\n", 
           retrieved_header->version, retrieved_header->type, retrieved_header->length);
    print_payload("Retrieved", retrieved_payload, retrieved_header->length);
    
    if (retrieved_header->version != header.version ||
        retrieved_header->type != header.type ||
        retrieved_header->length != header.length) {
        fprintf(stderr, "    ✗ MDP=1: header mismatch\n");
        fprintf(stderr, "      expected: version=%d, type=0x%x, length=%d\n",
                header.version, header.type, header.length);
        fprintf(stderr, "      got:      version=%d, type=0x%x, length=%d\n",
                retrieved_header->version, retrieved_header->type, retrieved_header->length);
        exit(EXIT_FAILURE);
    }
    
    if (memcmp(retrieved_payload, payload, header.length) != 0) {
        fprintf(stderr, "    ✗ MDP=1: payload mismatch\n");
        exit(EXIT_FAILURE);
    }
    
    printf("    ✓ MDP=1 metadata verified successfully\n");
    
    free(meta_buffer);
    free(retrieved_buffer);
}

int main(void)
{
    struct timespec start, end;
    double time_taken;
    size_t ps = sysconf(_SC_PAGESIZE);
    uint8_t *buf;

    printf("\n=== Test3: PTE Metadata Test (MDP=1 Structured Buffer) ===\n\n");

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

    // Section 2: Test MDP=1
    test_mdp1(buf, ps);

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