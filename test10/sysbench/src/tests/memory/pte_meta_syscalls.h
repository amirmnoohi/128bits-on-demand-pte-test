/* pte_meta_syscalls.h - PTE metadata syscall definitions for sysbench */

#ifndef PTE_META_SYSCALLS_H
#define PTE_META_SYSCALLS_H

#include <unistd.h>
#include <sys/syscall.h>
#include <stdint.h>

/* Syscall numbers - adjust these based on your kernel implementation */
#define SYS_enable_pte_meta  469
#define SYS_disable_pte_meta 470
#define SYS_set_pte_meta     471
#define SYS_get_pte_meta     472

/* Metadata header structure for MDP=1 */
struct metadata_header {
    uint32_t version;
    uint32_t type;
    uint32_t length;
    uint32_t reserved;
};

/* Syscall wrappers aligned with new design */
static inline int enable_pte_meta(unsigned long addr) {
    return syscall(SYS_enable_pte_meta, addr);
}

static inline int disable_pte_meta(unsigned long addr) {
    return syscall(SYS_disable_pte_meta, addr);
}

static inline int set_pte_meta(unsigned long addr, int mdp, unsigned long meta_ptr) {
    return syscall(SYS_set_pte_meta, addr, mdp, meta_ptr);
}

static inline int get_pte_meta(unsigned long addr, void *buffer) {
    return syscall(SYS_get_pte_meta, addr, buffer);
}

#endif /* PTE_META_SYSCALLS_H */