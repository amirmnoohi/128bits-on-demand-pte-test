# PTE Meta Test Suite

This test suite verifies the functionality of the PTE (Page Table Entry) meta operations in the kernel. It includes tests for enabling, disabling, setting, and getting PTE metadata.

## Test Structure

The test suite consists of 8 tests, each focusing on different aspects of PTE meta operations:

### Test1: Basic Page Allocation and Write
- Tests basic page allocation and pattern writing
- Verifies memory pattern integrity
- Measures timing for allocation and writing operations

### Test2: PTE Meta Content Test
- Tests setting PTE meta with bit 58=0 (content)
- Verifies meta value and type
- Measures timing for set/get operations
- Uses meta value: 0xCAFEBABE

### Test3: PTE Meta Pointer Test
- Tests setting PTE meta with bit 58=1 (pointer)
- Verifies meta value and type
- Measures timing for set/get operations
- Uses meta value: 0xCAFEBABE

### Test4: PTE Meta Not Expanded Test
- Tests getting PTE meta when not expanded
- Verifies ENODATA error handling
- Measures timing for get operation

### Test5: PTE Meta Without Setup Test
- Tests getting PTE meta without prior setup
- Verifies ENODATA error handling
- Measures timing for get operation

### Test6: PTE Meta Enable/Disable Test
- Tests enabling and disabling PTE meta
- Verifies successful enable/disable operations
- Measures timing for enable/disable operations

### Test7: PTE Meta Disable Without Enable Test
- Tests disabling PTE meta without prior enable
- Verifies ENODATA error handling
- Measures timing for disable operation

### Test8: PTE Meta Set/Get Timing Comparison
- Tests comparing timing between first and second set operations
- Verifies meta value changes
- Measures timing for multiple set/get operations
- Uses meta values: 0xCAFEBABE and 0xDEADBEEF

## Building and Running

### Prerequisites
- GCC compiler
- Make
- Linux kernel with PTE meta support

### Build
```bash
make
```

### Run All Tests
```bash
make test
```

### Run Individual Tests
```bash
make test_1  # Run Test1
make test_2  # Run Test2
# ... and so on
```

### Clean Build Files
```bash
make clean
```

## Test Output

Each test produces output in two sections:
1. **Timing Measurements**
   - Shows execution time for each operation
   - Includes page allocation, pattern writing, and PTE meta operations

2. **Results and Verification**
   - Shows verification results
   - Displays success/failure indicators
   - Reports any errors encountered

## Log Files

Test outputs are saved in the `output` directory:
- `output/test1.log` - Test1 output
- `output/test2.log` - Test2 output
- And so on...

Each log file contains:
- Complete test output
- Timing measurements
- Verification results
- Any error messages

## Error Handling

Tests verify various error conditions:
- ENODATA: When PTE meta is not set up
- EINVAL: Invalid arguments
- EPERM: Permission issues

## Common Patterns

All tests follow these common patterns:
1. Page allocation and locking
2. Pattern writing and verification
3. PTE meta operations
4. Result verification
5. Cleanup (unlock and free)

## Makefile Structure

- Main Makefile: Orchestrates all tests
- Individual test Makefiles:
  - Compiler: GCC
  - Flags: -Wall -O0
  - Target: testN_program
  - Phony targets: all, clean, test_N

## Notes

- All tests use page-aligned memory
- Memory is locked during tests
- Pattern verification ensures memory integrity
- Timing measurements use CLOCK_MONOTONIC
- Error messages follow consistent formatting

## Sample Outputs

### Running All Tests
```bash
$ make test
=============================================
  Running all tests
=============================================
Running test in test1... 
[PASS] Test successful in test1
Running test in test2... 
[PASS] Test successful in test2
Running test in test3... 
[PASS] Test successful in test3
Running test in test4... 
[PASS] Test successful in test4
Running test in test5... 
[PASS] Test successful in test5
Running test in test6... 
[PASS] Test successful in test6
Running test in test7... 
[PASS] Test successful in test7
Running test in test8... 
[PASS] Test successful in test8
=============================================
  Summary: 8 passed, 0 failed
  Logs: output/*.log
=============================================
```

### Building All Tests
```bash
$ make
=============================================
  Building in all directories
=============================================
Building in test1... 
[PASS] Build successful in test1
Building in test2... 
[PASS] Build successful in test2
Building in test3... 
[PASS] Build successful in test3
Building in test4... 
[PASS] Build successful in test4
Building in test5... 
[PASS] Build successful in test5
Building in test6... 
[PASS] Build successful in test6
Building in test7... 
[PASS] Build successful in test7
Building in test8... 
[PASS] Build successful in test8
=============================================
  Summary: 8 passed, 0 failed
=============================================
```

### Cleaning Build Files
```bash
$ make clean
=============================================
  Cleaning Up
=============================================
Cleaning in test1... 
[PASS] Cleanup successful in test1
Cleaning in test2... 
[PASS] Cleanup successful in test2
Cleaning in test3... 
[PASS] Cleanup successful in test3
Cleaning in test4... 
[PASS] Cleanup successful in test4
Cleaning in test5... 
[PASS] Cleanup successful in test5
Cleaning in test6... 
[PASS] Cleanup successful in test6
Cleaning in test7... 
[PASS] Cleanup successful in test7
Cleaning in test8... 
[PASS] Cleanup successful in test8
=============================================
  Summary: 8 passed, 0 failed
=============================================
```

Note: In the actual terminal output:
- Headers are shown in blue and bold
- [PASS] messages are shown in green
- [FAIL] messages would be shown in red
- Running/Cleaning/Building messages are shown in yellow
- Summary numbers are color-coded (green for passed, red for failed)
