# PTE Metadata Test Suite

This comprehensive test suite verifies the functionality and performance of PTE (Page Table Entry) metadata operations in the kernel. The suite includes 10 tests covering functional verification, timing analysis, and comprehensive performance benchmarking with the new syscall design.

## Overview

The PTE metadata system enables storing additional data alongside page table entries through four custom syscalls:
- **`sys_enable_pte_meta` (469)**: Expands 4KiB PTE page to 8KiB and sets PMD PEN bit
- **`sys_disable_pte_meta` (470)**: Collapses back to 4KiB and clears PMD PEN bit  
- **`sys_set_pte_meta` (471)**: Stores 64-bit metadata with MDP bit (0/1)
- **`sys_get_pte_meta` (472)**: Retrieves metadata using buffer parameter

### Metadata Types
- **MDP=0**: Direct u64 metadata storage
- **MDP=1**: Structured metadata with header + payload (version, type, length, data)

## Test Structure

### Tests 1-9: Functional and Timing Verification
Each test focuses on specific aspects of PTE metadata operations:

- **Test1**: Basic page allocation and write operations
- **Test2**: PTE metadata content with MDP=0 (direct u64)
- **Test3**: PTE metadata pointer with MDP=1 (structured buffer)
- **Test4**: Get metadata when page table not expanded (expects ENODATA)
- **Test5**: Multiple page metadata test (independent storage verification)
- **Test6**: Enable/disable metadata lifecycle with error handling
- **Test7**: Disable metadata without enable (expects EINVAL)
- **Test8**: Set/get timing comparison (expansion vs pure update)
- **Test9**: 10,000-iteration statistical analysis with optimized flow

Each test measures precise timing, verifies correctness, and validates error handling (ENODATA, EINVAL, EPERM, EEXIST).

### Test10: Comprehensive Performance Benchmark
Located in `test10/`, this uses a custom `sysbench.sh` script that automatically compiles sysbench with PTE metadata support and runs comprehensive benchmarks.

**Build Process:**
1. 🧹 Clean previous build (`make distclean`)
2. 🔧 Generate configure script (`./autogen.sh`)
3. ⚙️ Configure build (`./configure --without-mysql`)
4. 🔨 Compile with maximum parallelism (`make -j$(nproc)`)
5. 📦 Install binary (`./sysbench_binary`)

**Test Matrix (12 Tests Total):**
- **4 Scenarios**: Write Sequential, Write Random, Read Sequential, Read Random
- **3 Conditions**: No PTE, MDP=0 (direct u64), MDP=1 (structured)

## Building and Running

### Prerequisites
- GCC compiler and build tools (`build-essential`)
- Autotools (`autotools-dev`, `autoconf`, `autoreconf`)
- Optional: `pkg-config`, `libluajit-5.1-dev`
- Linux kernel with PTE metadata syscalls loaded
- Root privileges for syscall operations

### Build All Tests
```bash
make
```

### Run All Tests
```bash
make test
```

### Run Individual Tests
```bash
make test_1   # Run Test1
make test_2   # Run Test2
# ...
make test_9   # Run Test9
make test_10  # Run Test10 (performance benchmark)
```

### Run Test10 Performance Benchmark
```bash
cd test10
./sysbench.sh
```

This automatically handles:
- Prerequisites checking
- Sysbench compilation from source
- Syscall availability verification
- 12 comprehensive performance tests
- Results analysis and reporting

### Clean Build Files
```bash
make clean
```

## Sample Output

### Test10 Performance Benchmark
```
=== PTE Metadata Performance Testing Suite ===
12 Tests Total: 4 scenarios x 3 conditions (no PTE, MDP=0, MDP=1)
Date: Fri Aug 15 23:16:36 UTC 2025
Test duration: --time=30

🔧 Checking prerequisites...
✅ Prerequisites check passed

🔨 Compiling sysbench with PTE metadata support...
  🔧 Running autogen.sh...
  ⚙️  Running configure...
  🔨 Compiling with 8 parallel jobs...
  📦 Installing binary...
✅ Sysbench compiled and installed successfully
   Version: sysbench 1.1.0-3ceba0b

🔍 Verifying PTE metadata syscalls...
✅ Found: sys_enable_pte_meta
✅ Found: sys_disable_pte_meta
✅ Found: sys_set_pte_meta
✅ Found: sys_get_pte_meta
✅ All PTE metadata syscalls available

🔹 Starting Write Tests...
🏃 Running Test: 01_write_seq_no_pte
✅ Completed: 01_write_seq_no_pte

🏃 Running Test: 02_write_seq_with_pte_mdp0
✅ Completed: 02_write_seq_with_pte_mdp0

🏃 Running Test: 03_write_seq_with_pte_mdp1
✅ Completed: 03_write_seq_with_pte_mdp1

[... 9 more tests ...]

🎉 All 12 tests completed successfully!

Results Summary:
==================
Write Sequential: 461745.49 → 985.58 → 876.66 ops/sec
  MDP=0 impact: 468.5x slower (99.7% overhead)
  MDP=1 impact: 526.7x slower (99.8% overhead)

Write Random:     66339.83 → 918.17 → 788.97 ops/sec
  MDP=0 impact: 72.2x slower (98.6% overhead)
  MDP=1 impact: 84.0x slower (98.8% overhead)

Read Sequential:  508987.33 → 2674.77 → 2659.98 ops/sec
  MDP=0 impact: 190.2x slower (99.4% overhead)
  MDP=1 impact: 191.3x slower (99.4% overhead)

Read Random:      66015.00 → 2473.57 → 2470.17 ops/sec
  MDP=0 impact: 26.6x slower (96.2% overhead)
  MDP=1 impact: 26.7x slower (96.2% overhead)

Files created in current directory:
- Compiled sysbench binary: ./sysbench_binary
- 12 individual test results: *_20250815_231636.txt
- Summary report: summary_20250815_231636.txt
- CSV data: results_20250815_231636.csv
```

### Test9 Statistical Analysis
```
=== Test9: PTE Meta Operations Statistics Test ===

--- Initial Setup ---
    Page allocation     :     111328 ns
    Pattern writing     :     153200 ns

--- Iterative Operations (10000 iterations) ---

--- Statistics ---
Enable PTE Meta:
    Min:          46208 ns
    Max:         480368 ns
    Mean:         51097 ns
    StdDev:       21410 ns

Set PTE Meta:
    Min:          11392 ns
    Max:         221200 ns
    Mean:         12883 ns
    StdDev:       10638 ns

Get PTE Meta:
    Min:           4160 ns
    Max:         216608 ns
    Mean:          5217 ns
    StdDev:        6185 ns

Disable PTE Meta:
    Min:          39440 ns
    Max:         510080 ns
    Mean:         46801 ns
    StdDev:       22370 ns

--- Final Verification ---
    ✓ verification successful after iterations
    ✓ All 10000 iterations completed successfully
```

## Performance Analysis

### Key Findings from Test10
The comprehensive benchmark reveals significant performance impacts:

**Write Operations:**
- Sequential writes show the highest overhead (468-526x slower)
- Random writes have moderate overhead (72-84x slower)
- MDP=1 (structured) consistently slower than MDP=0 (direct)

**Read Operations:**
- Sequential reads show high overhead (190x slower)
- Random reads show the lowest overhead (26-27x slower)
- Minimal difference between MDP=0 and MDP=1 for reads

**Syscall Performance (from Test9):**
- Get operations: ~5.2μs (fastest)
- Set operations: ~12.9μs (moderate)
- Enable/Disable: ~47-51μs (slowest, includes page table expansion)

## Test Output Locations

- **Tests 1-9**: Logs saved in `output/` directory (e.g., `output/test1.log`)
- **Test10**: Results in `test10/` directory:
  - Individual test results: `*_YYYYMMDD_HHMMSS.txt`
  - Summary report: `summary_YYYYMMDD_HHMMSS.txt`
  - CSV data: `results_YYYYMMDD_HHMMSS.csv`

## Error Handling

Comprehensive error checking for:
- **ENODATA**: PTE metadata not set up or page table not expanded
- **EINVAL**: Invalid arguments or disable without enable
- **EPERM**: Permission issues (requires CAP_SYS_ADMIN)
- **EEXIST**: Double enable attempt
- **EFAULT**: Invalid buffer pointers
- **ENOMEM**: Memory allocation failures

## Syscall Design

### New Buffer-Based Interface
```c
// Enable/disable page table expansion
int sys_enable_pte_meta(unsigned long addr);
int sys_disable_pte_meta(unsigned long addr);

// Set metadata (MDP=0: direct u64, MDP=1: structured buffer)
int sys_set_pte_meta(unsigned long addr, int mdp, unsigned long meta_ptr);

// Get metadata using buffer parameter
int sys_get_pte_meta(unsigned long addr, void *buffer);
```

### Metadata Header Structure (MDP=1)
```c
struct metadata_header {
    uint32_t version;    // Metadata format version
    uint32_t type;       // Application-specific type
    uint32_t length;     // Payload length in bytes
    uint32_t reserved;   // Reserved for future use
};
```

## Makefile Targets

- `make all` – Build all tests
- `make test` – Run all tests with colored output
- `make clean` – Clean all build files
- `make test_N` – Run individual test (N=1..10)
- For Test10: `cd test10 && ./sysbench.sh`

## Technical Notes

- All tests use page-aligned memory with `mlock()` during execution
- Pattern verification ensures memory integrity throughout tests
- Timing uses `CLOCK_MONOTONIC` for precise measurements
- Test10 requires no manual compilation (automated by script)
- Sysbench modifications support both MDP=0 and MDP=1 metadata types
- Performance overhead varies significantly by access pattern and metadata type

## Troubleshooting

### Common Issues
1. **Syscalls not found**: Ensure PTE metadata kernel module is loaded
2. **Permission denied**: Run tests with `sudo` (requires CAP_SYS_ADMIN)
3. **Build failures**: Install prerequisites (`build-essential`, `autotools-dev`)
4. **LuaJIT issues**: Install `libluajit-5.1-dev` or let script use bundled version

### Debug Information
Test10 provides detailed debug output including:
- Build process status
- Syscall availability verification
- Binary location and version information
- Detailed error messages with suggested fixes