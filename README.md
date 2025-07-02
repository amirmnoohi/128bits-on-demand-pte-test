# PTE Meta Test Suite

This test suite verifies the functionality and performance of PTE (Page Table Entry) meta operations in the kernel. It now includes 10 tests, covering both functional and performance aspects, including a new sysbench-based benchmark (test10).

## Test Structure

The suite consists of 10 tests:

### Test1–Test9: Functional and Timing Tests
Each of these tests focuses on a specific aspect of PTE meta operations:
- **Test1:** Basic page allocation and write
- **Test2:** PTE meta content (bit 58=0)
- **Test3:** PTE meta pointer (bit 58=1)
- **Test4:** Get meta when not expanded
- **Test5:** Get meta without setup
- **Test6:** Enable/disable meta
- **Test7:** Disable meta without enable
- **Test8:** Set/get timing comparison
- **Test9:** 10,000-iteration statistics (enable, set, get, disable)

Each test measures timing, verifies results, and checks error handling (ENODATA, EINVAL, EPERM).

### Test10: PTE Metadata Performance Benchmark
Located in `test10/`, this test uses a custom `sysbench.sh` script to benchmark memory operations with and without PTE metadata. It covers:
- Write Sequential (with/without PTE)
- Write Random (with/without PTE)
- Read Sequential (with/without PTE)
- Read Random (with/without PTE)

For each scenario, it measures operations per second and generates:
- Individual result files (`*_DATE.txt`)
- A summary report (`summary_DATE.txt`)
- A CSV file (`results_DATE.csv`)

## Building and Running

### Prerequisites
- GCC compiler
- Make
- Linux kernel with PTE meta support
- For test10: `sysbench` (provided in `test10/`)

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
make test_10  # Run Test10 (see below)
```

### Run Test10 (Performance Benchmark)
```bash
cd test10
bash sysbench.sh
```
This will run all 8 scenarios and generate result files in the current directory.

### Clean Build Files
```bash
make clean
```

## Test Output

- **Tests 1–9:** Output logs are saved in the `output/` directory (e.g., `output/test1.log`).
- **Test10:** Result files (`*.txt`), summary (`summary_DATE.txt`), and CSV (`results_DATE.csv`) are created in `test10/`.

## Error Handling

Tests check for:
- ENODATA: PTE meta not set up
- EINVAL: Invalid arguments
- EPERM: Permission issues

## Common Patterns

All tests (1–9) follow these steps:
1. Page allocation and locking
2. Pattern writing and verification
3. PTE meta operations
4. Result verification
5. Cleanup (unlock and free)

Test10 benchmarks memory operations using sysbench with and without PTE meta enabled.

## Makefile Targets

- `make all`      – Build all tests
- `make test`     – Run all tests
- `make clean`    – Clean all tests
- `make test_N`   – Run testN individually (N=1..10)
- For test10: `cd test10 && bash sysbench.sh`

## Notes

- All tests use page-aligned memory and lock memory during tests
- Pattern verification ensures memory integrity
- Timing uses CLOCK_MONOTONIC
- Error messages are consistently formatted
- Test10 does not require compilation; it is a shell script using the provided sysbench binary

## Sample Outputs

### Running All Tests
```bash
$ make test
=============================================
  Running all tests
=============================================
Running test in test1... 
[PASS] Test successful in test1
...
Running test in test10... 
[PASS] Test successful in test10 (no build needed)
=============================================
  Summary: 10 passed, 0 failed
  Logs: output/*.log
=============================================
```

### Running Test10
```bash
$ cd test10
$ bash sysbench.sh
=== PTE Metadata Performance Testing Suite ===
8 Tests Total: 4 scenarios x 2 conditions (with/without PTE)
...
🎉 All 8 tests completed successfully!
Files created in current directory:
- 8 individual test results: *_DATE.txt
- Summary report: summary_DATE.txt
- CSV data: results_DATE.csv
```

### Cleaning Build Files
```bash
$ make clean
=============================================
  Cleaning Up
=============================================
Cleaning in test1... 
[PASS] Cleanup successful in test1
...
Cleaning in test10... 
[PASS] No cleanup needed in test10
=============================================
  Summary: 10 passed, 0 failed
=============================================
```

Note: In the actual terminal output:
- Headers are shown in blue and bold
- [PASS] messages are shown in green
- [FAIL] messages would be shown in red
- Running/Cleaning/Building messages are shown in yellow
- Summary numbers are color-coded (green for passed, red for failed)

#### Test9 Output
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

The output shows detailed timing statistics for each PTE meta operation over 10,000 iterations. Key observations:
- Get operations are fastest (mean: ~5.2μs)
- Set operations are relatively fast (mean: ~12.9μs)
- Enable/Disable operations are slower (mean: ~51.1μs/~46.8μs)
- All operations show some variance in timing, with occasional spikes
