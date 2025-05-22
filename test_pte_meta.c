/*
 * stepwise_pte_meta_test.c  – interactive, with zero-check after enable
 *
 * Order: enable → zero-check → disable → set → get → disable
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <errno.h>

#define SYS_enable_pte_meta   469
#define SYS_disable_pte_meta  470
#define SYS_set_pte_meta      471
#define SYS_get_pte_meta      472

#define META_VALUE 0xCAFEBABEULL
#define META_TYPE  1

static void wait_enter(const char *msg)
{ printf("→ %s <Enter>…", msg); fflush(stdout); getchar(); }

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

int main(void)
{
    size_t ps = sysconf(_SC_PAGESIZE);
    uint8_t *buf;
    posix_memalign((void **)&buf, ps, ps);
    mlock(buf, ps);

    fill(buf, ps);
    puts("[0] page allocated & pattern written");
    verify_pattern("initial fill", buf, ps);

    /* 1) enable -------------------------------------------------- */
    wait_enter("call enable_pte_meta()");
    call_or_die(SYS_enable_pte_meta, (unsigned long)buf, 0, 0,
                "enable_pte_meta");
    puts("[1] enable_pte_meta() OK");
    verify_pattern("enable_pte_meta", buf, ps);

    /* immediate get → expect zeros -------------------------------- */
    long long raw = syscall(SYS_get_pte_meta, (unsigned long)buf);
    if (raw < 0) { perror("get_pte_meta"); exit(EXIT_FAILURE); }
    int type0 = (raw >> 63) & 1;
    uint64_t meta0 = raw & ((1ULL << 63) - 1);
    if (type0 || meta0) {
        fprintf(stderr,
                "    ✗ expected zero meta/type after enable, got meta=0x%llx type=%d\n",
                (unsigned long long)meta0, type0);
        exit(EXIT_FAILURE);
    }
    puts("    ✓ get_pte_meta right after enable → meta=0 type=0 (as expected)");

    /* 2) disable -------------------------------------------------- */
    wait_enter("call first disable_pte_meta()");
    call_or_die(SYS_disable_pte_meta, (unsigned long)buf, 0, 0,
                "disable_pte_meta");
    puts("[2] disable_pte_meta() OK");
    verify_pattern("first disable_pte_meta", buf, ps);

    /* 3) set ------------------------------------------------------ */
    wait_enter("call set_pte_meta()");
    call_or_die(SYS_set_pte_meta, (unsigned long)buf,
                META_VALUE, META_TYPE, "set_pte_meta");
    puts("[3] set_pte_meta() OK");
    verify_pattern("set_pte_meta", buf, ps);

    /* 4) get ------------------------------------------------------ */
    wait_enter("call get_pte_meta()");
    raw = syscall(SYS_get_pte_meta, (unsigned long)buf);


    if (raw == -EINVAL || raw == -EPERM) { 
        perror("get_pte_meta"); 
        exit(EXIT_FAILURE); 
    }
    int type = (raw >> 63) & 1;
    uint64_t meta = raw & ((1ULL << 63) - 1);
    printf("[4] get_pte_meta() → meta=0x%llx type=%d\n",
        (unsigned long long)meta, type);
    if (meta != META_VALUE || type != META_TYPE) {
        fprintf(stderr, "meta/type mismatch!\n"); exit(EXIT_FAILURE);
    }
    puts("    ✓ meta / type match expected values");
    verify_pattern("get_pte_meta", buf, ps);

    /* 5) disable again ------------------------------------------- */
    wait_enter("call second disable_pte_meta()");
    call_or_die(SYS_disable_pte_meta, (unsigned long)buf, 0, 0,
                "disable_pte_meta");
    puts("[5] disable_pte_meta() OK");
    verify_pattern("second disable_pte_meta", buf, ps);

    munlock(buf, ps);
    free(buf);
    puts("[✓] All tests passed – page survived every syscall.");
    return 0;
}
