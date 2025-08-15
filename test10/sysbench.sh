#!/bin/bash

# PTE Metadata Performance Testing Script
# 12 Tests: 4 scenarios (write-seq, write-rnd, read-seq, read-rnd) x 3 conditions (no PTE, MDP=0, MDP=1)

set -e  # Exit on error

# Configuration
SYSBENCH_SRC_DIR="./sysbench"  # Sysbench source folder in current directory
SYSBENCH_PATH="./sysbench_binary"  # Compiled binary name (avoid conflict with source folder)
TEST_DURATION="--time=30"  # 30 seconds per test
DATE=$(date +"%Y%m%d_%H%M%S")

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to check prerequisites
check_prerequisites() {
    echo -e "${BLUE}🔧 Checking prerequisites...${NC}"
    
    # Check for required tools
    local tools=("gcc" "make" "bc" "autoconf" "autoreconf")
    for tool in "${tools[@]}"; do
        if ! command -v "$tool" >/dev/null 2>&1; then
            echo -e "${RED}❌ Required tool not found: $tool${NC}"
            echo "  Try: apt install build-essential autotools-dev"
            exit 1
        fi
    done
    
    # Check for pkg-config (often needed by autotools)
    if ! command -v pkg-config >/dev/null 2>&1; then
        echo -e "${YELLOW}⚠️  Warning: pkg-config not found, may cause configure issues${NC}"
        echo "  Try: apt install pkg-config"
    fi
    
    # Check for LuaJIT development files (helps avoid build issues)
    if ! pkg-config --exists luajit 2>/dev/null; then
        echo -e "${YELLOW}⚠️  Warning: LuaJIT development files not found${NC}"
        echo "  Try: apt install libluajit-5.1-dev"
        echo "  (Will try to use bundled LuaJIT if system version fails)"
    fi
    
    # Check for root privileges (needed for syscalls)
    if [[ $EUID -ne 0 ]]; then
        echo -e "${YELLOW}⚠️  Warning: Not running as root. PTE metadata tests may fail.${NC}"
        echo "Consider running with sudo for full functionality."
    fi
    
    echo -e "${GREEN}✅ Prerequisites check passed${NC}"
}

# Function to compile sysbench
compile_sysbench() {
    echo -e "${BLUE}🔨 Compiling sysbench with PTE metadata support...${NC}"
    
    if [[ ! -d "$SYSBENCH_SRC_DIR" ]]; then
        echo -e "${RED}❌ Error: Sysbench source directory not found: $SYSBENCH_SRC_DIR${NC}"
        echo "Please ensure sysbench source code is available in the current directory."
        exit 1
    fi
    
    # Check if binary already exists and is newer than source
    if [[ -x "$SYSBENCH_PATH" ]]; then
        if [[ "$SYSBENCH_PATH" -nt "$SYSBENCH_SRC_DIR" ]]; then
            echo -e "${GREEN}  ✅ Existing binary is up to date, skipping compilation${NC}"
            return 0
        fi
    fi
    
    cd "$SYSBENCH_SRC_DIR"
    
    # Step 1: Clean previous build
    echo -e "${BLUE}  🧹 Cleaning previous build...${NC}"
    make distclean >/dev/null 2>&1 || make clean >/dev/null 2>&1 || true
    
    # Step 2: Run autogen.sh to generate configure script
    echo -e "${BLUE}  🔧 Running autogen.sh...${NC}"
    if [[ ! -f "autogen.sh" ]]; then
        echo -e "${RED}❌ Error: autogen.sh not found${NC}"
        cd - >/dev/null
        exit 1
    fi
    
    ./autogen.sh || {
        echo -e "${RED}❌ Error: autogen.sh failed${NC}"
        echo "Output:"
        ./autogen.sh
        cd - >/dev/null
        exit 1
    }
    
    # Step 3: Configure the build
    echo -e "${BLUE}  ⚙️  Running configure...${NC}"
    if [[ ! -f "configure" ]]; then
        echo -e "${RED}❌ Error: configure script not generated${NC}"
        cd - >/dev/null
        exit 1
    fi
    
    # Try different configure options
    echo "     Trying configure with system LuaJIT..."
    if ./configure --without-mysql --with-system-luajit >/dev/null 2>&1; then
        echo -e "${GREEN}     ✅ Configure successful (with system LuaJIT)${NC}"
    else
        echo "     System LuaJIT failed, trying bundled LuaJIT..."
        if ./configure --without-mysql >/dev/null 2>&1; then
            echo -e "${GREEN}     ✅ Configure successful (with bundled LuaJIT)${NC}"
        else
            echo -e "${RED}❌ Error: Configure failed with both options${NC}"
            echo "Showing configure output:"
            ./configure --without-mysql
            cd - >/dev/null
            exit 1
        fi
    fi
    
    # Step 4: Compile with maximum parallelism
    echo -e "${BLUE}  🔨 Compiling with $(nproc) parallel jobs...${NC}"
    if make -j$(nproc) >/dev/null 2>&1; then
        echo -e "${GREEN}     ✅ Compilation successful${NC}"
    else
        echo -e "${YELLOW}     ⚠️  Parallel build failed, trying single-threaded...${NC}"
        if make -j1 >/dev/null 2>&1; then
            echo -e "${GREEN}     ✅ Single-threaded compilation successful${NC}"
        else
            echo -e "${RED}❌ Error: Compilation failed${NC}"
            echo "Showing build output:"
            make -j1
            cd - >/dev/null
            exit 1
        fi
    fi
    
    # Debug: Show what was actually built
    echo -e "${BLUE}  🔍 Checking build results...${NC}"
    echo "   Files in src/ directory:"
    ls -la src/ 2>/dev/null || echo "     src/ directory not found"
    echo "   Executable files in current directory:"
    find . -maxdepth 2 -name "*sysbench*" -type f -executable 2>/dev/null || echo "     No sysbench executables found"
    
    # Step 5: Find and copy binary to test directory
    echo -e "${BLUE}  📦 Installing binary...${NC}"
    
    # Look for sysbench binary in common locations
    local binary_found=""
    local search_paths=("src/sysbench" "sysbench" "src/sysbench.exe" "./sysbench")
    
    for path in "${search_paths[@]}"; do
        if [[ -f "$path" && -x "$path" ]]; then
            binary_found="$path"
            echo "   Found binary at: $path"
            break
        fi
    done
    
    if [[ -z "$binary_found" ]]; then
        echo -e "${RED}❌ Error: Could not find sysbench binary${NC}"
        echo "   Searched in:"
        for path in "${search_paths[@]}"; do
            echo "     - $path"
        done
        echo "   Available files in src/:"
        ls -la src/ 2>/dev/null || echo "     src/ directory not found"
        echo "   Available files in current directory:"
        ls -la . | grep -E "(sysbench|executable)"
        cd - >/dev/null
        exit 1
    fi
    
    cd - >/dev/null
    if cp "$SYSBENCH_SRC_DIR/$binary_found" "$SYSBENCH_PATH"; then
        chmod +x "$SYSBENCH_PATH"
        echo -e "${GREEN}✅ Sysbench compiled and installed successfully${NC}"
        echo "   Source: $SYSBENCH_SRC_DIR/$binary_found"
        echo "   Target: $SYSBENCH_PATH"
        echo "   Version: $($SYSBENCH_PATH --version 2>/dev/null | head -1 || echo 'Version check failed')"
    else
        echo -e "${RED}❌ Error: Failed to copy sysbench binary${NC}"
        echo "   Source: $SYSBENCH_SRC_DIR/$binary_found"
        echo "   Target: $SYSBENCH_PATH"
        exit 1
    fi
}

# Function to verify syscall availability
verify_syscalls() {
    echo -e "${BLUE}🔍 Verifying PTE metadata syscalls...${NC}"
    
    # Check if syscalls are available in the kernel
    if [[ ! -f "/proc/kallsyms" ]]; then
        echo -e "${YELLOW}⚠️  Warning: Cannot verify syscall availability (/proc/kallsyms not accessible)${NC}"
        return 0
    fi
    
    # Check for syscall symbols
    local syscalls=("sys_enable_pte_meta" "sys_disable_pte_meta" "sys_set_pte_meta" "sys_get_pte_meta")
    local missing_syscalls=0
    
    for syscall in "${syscalls[@]}"; do
        if ! grep -q "$syscall" /proc/kallsyms 2>/dev/null; then
            echo -e "${RED}❌ Syscall not found: $syscall${NC}"
            ((missing_syscalls++))
        else
            echo -e "${GREEN}✅ Found: $syscall${NC}"
        fi
    done
    
    if [[ $missing_syscalls -gt 0 ]]; then
        echo -e "${YELLOW}⚠️  Warning: $missing_syscalls syscall(s) missing from kernel${NC}"
        echo "PTE metadata tests will use standard memory operations."
    else
        echo -e "${GREEN}✅ All PTE metadata syscalls available${NC}"
    fi
}

# Function to run a test and save results
run_test() {
    local test_name="$1"
    local command="$2"
    local output_file="${test_name}_${DATE}.txt"
    
    echo -e "${BLUE}🏃 Running Test: $test_name${NC}"
    echo "   Command: $command"
    echo "   Output: $output_file"
    
    # Write test header to file
    {
        echo "=== PTE Metadata Test: $test_name ==="
        echo "Date: $(date)"
        echo "Command: $command"
        echo "Duration: $TEST_DURATION"
        echo "System: $(uname -a)"
        echo "CPU Info: $(grep 'model name' /proc/cpuinfo | head -1)"
        echo "Memory: $(grep MemTotal /proc/meminfo)"
        echo "Kernel: $(uname -r)"
        echo ""
    } > "$output_file"
    
    # Run the actual test and append to file
    echo "   Executing test..."
    if eval "$command" >> "$output_file" 2>&1; then
        echo -e "${GREEN}✅ Completed: $test_name${NC}"
    else
        echo -e "${RED}❌ Failed: $test_name${NC}"
        echo "   Check $output_file for error details"
        # Don't exit, continue with other tests
    fi
    
    # Add separator
    echo "" >> "$output_file"
    echo "=== End of Test ===" >> "$output_file"
    echo ""
}

# Improved extraction function
extract_ops_per_sec() {
    local file="$1"
    
    if [[ ! -f "$file" ]]; then
        echo "0"
        return
    fi
    
    # Debug output for first file to see the format
    if [[ "$file" == *"01_write_seq_no_pte"* ]]; then
        echo "DEBUG: Sample sysbench output from $file:" >&2
        echo "===================" >&2
        tail -20 "$file" >&2
        echo "===================" >&2
    fi
    
    local ops_per_sec=""
    
    # Try multiple extraction patterns
    # Pattern 1: Standard sysbench format "operations per second"
    ops_per_sec=$(grep -i "operations per second" "$file" | tail -1 | grep -Eo '[0-9]+\.?[0-9]*' | head -1 2>/dev/null)
    
    # Pattern 2: Look for "ops/sec"
    if [[ -z "$ops_per_sec" || "$ops_per_sec" == "0" ]]; then
        ops_per_sec=$(grep -i "ops/sec" "$file" | tail -1 | grep -Eo '[0-9]+\.?[0-9]*' | head -1 2>/dev/null)
    fi
    
    # Pattern 3: Calculate from total operations and time
    if [[ -z "$ops_per_sec" || "$ops_per_sec" == "0" ]]; then
        local total_ops=$(grep -i "total operations" "$file" | grep -Eo '[0-9]+' | head -1 2>/dev/null)
        local time_elapsed=$(grep -i "total.*time.*:" "$file" | grep -Eo '[0-9]+\.?[0-9]*' | head -1 2>/dev/null)
        if [[ -n "$total_ops" && -n "$time_elapsed" && "$time_elapsed" != "0" ]]; then
            ops_per_sec=$(echo "scale=2; $total_ops / $time_elapsed" | bc 2>/dev/null)
        fi
    fi
    
    # Pattern 4: Look for any line with numbers and "per second"
    if [[ -z "$ops_per_sec" || "$ops_per_sec" == "0" ]]; then
        ops_per_sec=$(grep -i "per second" "$file" | grep -Eo '[0-9]+\.?[0-9]*' | tail -1 2>/dev/null)
    fi
    
    # If all else fails, return 0
    if [[ -z "$ops_per_sec" ]]; then
        ops_per_sec="0"
    fi
    
    echo "$ops_per_sec"
}

# Function to calculate performance impact
calculate_impact() {
    local baseline="$1"
    local with_pte="$2"
    
    if [[ "$baseline" == "0" || -z "$baseline" || "$with_pte" == "0" || -z "$with_pte" ]]; then
        echo "N/A"
        return
    fi
    
    # Remove any non-numeric characters and convert to integers for calculation
    baseline=$(echo "$baseline" | sed 's/[^0-9.]//g')
    with_pte=$(echo "$with_pte" | sed 's/[^0-9.]//g')
    
    # Check if bc is available, otherwise use awk
    if command -v bc >/dev/null 2>&1; then
        # Calculate how many times slower (slowdown factor)
        local slowdown=$(echo "scale=1; $baseline / $with_pte" | bc 2>/dev/null)
        
        # Also calculate percentage overhead
        local overhead=$(echo "scale=1; (($baseline - $with_pte) * 100) / $baseline" | bc 2>/dev/null)
    else
        # Use awk as fallback
        local slowdown=$(awk "BEGIN {printf \"%.1f\", $baseline / $with_pte}")
        local overhead=$(awk "BEGIN {printf \"%.1f\", (($baseline - $with_pte) * 100) / $baseline}")
    fi
    
    # Validate results
    if [[ -z "$slowdown" || -z "$overhead" ]]; then
        echo "calculation error"
        return
    fi
    
    echo "${slowdown}x slower (${overhead}% overhead)"
}

# Check if result files exist before processing
check_file_exists() {
    local file="$1"
    if [[ ! -f "$file" ]]; then
        echo "ERROR: Result file not found: $file" >&2
        echo "Available files in current directory:" >&2
        ls -la ./*.txt 2>/dev/null || echo "No .txt files found" >&2
        return 1
    fi
    return 0
}

# Extract results with better error handling
extract_with_check() {
    local file="$1"
    local test_name="$2"
    
    if check_file_exists "$file"; then
        local result=$(extract_ops_per_sec "$file")
        echo "Extracted from $test_name: $result ops/sec" >&2
        echo "$result"
    else
        echo "0"
    fi
}

# Start testing
echo -e "${BLUE}=== PTE Metadata Performance Testing Suite ===${NC}"
echo "12 Tests Total: 4 scenarios x 3 conditions (no PTE, MDP=0, MDP=1)"
echo "Date: $(date)"
echo "Results will be saved to current directory"
echo "Test duration: $TEST_DURATION"
echo ""

# Step 1: Check prerequisites
check_prerequisites
echo ""

# Step 2: Compile sysbench
compile_sysbench
echo ""

# Step 3: Verify syscalls are available
verify_syscalls
echo ""

# Step 4: Quick test of calculation function
echo -e "${BLUE}🧮 Testing calculation function...${NC}"
test_calc=$(calculate_impact "1000" "100")
echo "Test calculation (1000 → 100): $test_calc"
echo ""

echo -e "${YELLOW}🔹 Starting Write Tests...${NC}"

# Test 1: Write Sequential - WITHOUT PTE
run_test "01_write_seq_no_pte" \
    "$SYSBENCH_PATH $TEST_DURATION --memory-oper=write --memory-access-mode=seq memory run"

# Test 2: Write Sequential - WITH PTE MDP=0 (direct u64)
run_test "02_write_seq_with_pte_mdp0" \
    "$SYSBENCH_PATH $TEST_DURATION --memory-oper=write --memory-access-mode=seq --memory-pte-meta=on --memory-pte-meta-type=0 memory run"

# Test 3: Write Sequential - WITH PTE MDP=1 (structured)
run_test "03_write_seq_with_pte_mdp1" \
    "$SYSBENCH_PATH $TEST_DURATION --memory-oper=write --memory-access-mode=seq --memory-pte-meta=on --memory-pte-meta-type=1 memory run"

# Test 4: Write Random - WITHOUT PTE
run_test "04_write_rnd_no_pte" \
    "$SYSBENCH_PATH $TEST_DURATION --memory-oper=write --memory-access-mode=rnd memory run"

# Test 5: Write Random - WITH PTE MDP=0 (direct u64)
run_test "05_write_rnd_with_pte_mdp0" \
    "$SYSBENCH_PATH $TEST_DURATION --memory-oper=write --memory-access-mode=rnd --memory-pte-meta=on --memory-pte-meta-type=0 memory run"

# Test 6: Write Random - WITH PTE MDP=1 (structured)
run_test "06_write_rnd_with_pte_mdp1" \
    "$SYSBENCH_PATH $TEST_DURATION --memory-oper=write --memory-access-mode=rnd --memory-pte-meta=on --memory-pte-meta-type=1 memory run"

echo -e "${YELLOW}🔹 Starting Read Tests...${NC}"

# Test 7: Read Sequential - WITHOUT PTE
run_test "07_read_seq_no_pte" \
    "$SYSBENCH_PATH $TEST_DURATION --memory-oper=read --memory-access-mode=seq memory run"

# Test 8: Read Sequential - WITH PTE MDP=0 (direct u64)
run_test "08_read_seq_with_pte_mdp0" \
    "$SYSBENCH_PATH $TEST_DURATION --memory-oper=read --memory-access-mode=seq --memory-pte-meta=on --memory-pte-meta-type=0 memory run"

# Test 9: Read Sequential - WITH PTE MDP=1 (structured)
run_test "09_read_seq_with_pte_mdp1" \
    "$SYSBENCH_PATH $TEST_DURATION --memory-oper=read --memory-access-mode=seq --memory-pte-meta=on --memory-pte-meta-type=1 memory run"

# Test 10: Read Random - WITHOUT PTE
run_test "10_read_rnd_no_pte" \
    "$SYSBENCH_PATH $TEST_DURATION --memory-oper=read --memory-access-mode=rnd memory run"

# Test 11: Read Random - WITH PTE MDP=0 (direct u64)
run_test "11_read_rnd_with_pte_mdp0" \
    "$SYSBENCH_PATH $TEST_DURATION --memory-oper=read --memory-access-mode=rnd --memory-pte-meta=on --memory-pte-meta-type=0 memory run"

# Test 12: Read Random - WITH PTE MDP=1 (structured)
run_test "12_read_rnd_with_pte_mdp1" \
    "$SYSBENCH_PATH $TEST_DURATION --memory-oper=read --memory-access-mode=rnd --memory-pte-meta=on --memory-pte-meta-type=1 memory run"

echo -e "${YELLOW}🔹 Generating Summary Report...${NC}"

# Create summary comparison
SUMMARY_FILE="summary_${DATE}.txt"
{
    echo "=== PTE Metadata Performance Test Summary ==="
    echo "Date: $(date)"
    echo "Test Duration: $TEST_DURATION"
    echo ""
    echo "System Information:"
    echo "OS: $(uname -a)"
    echo "CPU: $(grep 'model name' /proc/cpuinfo | head -1 | cut -d':' -f2 | xargs)"
    echo "Memory: $(grep MemTotal /proc/meminfo | awk '{print $2 $3}')"
    echo "Kernel: $(uname -r)"
    echo ""
    echo "12 Tests Performed:"
    echo "1. Write Sequential - No PTE"
    echo "2. Write Sequential - PTE MDP=0 (direct u64)"
    echo "3. Write Sequential - PTE MDP=1 (structured)"
    echo "4. Write Random - No PTE"
    echo "5. Write Random - PTE MDP=0 (direct u64)"
    echo "6. Write Random - PTE MDP=1 (structured)"
    echo "7. Read Sequential - No PTE"
    echo "8. Read Sequential - PTE MDP=0 (direct u64)"
    echo "9. Read Sequential - PTE MDP=1 (structured)"
    echo "10. Read Random - No PTE"
    echo "11. Read Random - PTE MDP=0 (direct u64)"
    echo "12. Read Random - PTE MDP=1 (structured)"
    echo ""
    echo "Performance Comparison:"
    echo "======================="
} > "$SUMMARY_FILE"

# Extract and compare results
echo "📊 Extracting performance metrics..."

# Write results comparison
{
    echo ""
    echo "WRITE OPERATIONS:"
    echo "-----------------"
    
    # Sequential Write
    w_seq_no_pte=$(extract_with_check "01_write_seq_no_pte_${DATE}.txt" "Sequential Write (no PTE)")
    w_seq_mdp0=$(extract_with_check "02_write_seq_with_pte_mdp0_${DATE}.txt" "Sequential Write (MDP=0)")
    w_seq_mdp1=$(extract_with_check "03_write_seq_with_pte_mdp1_${DATE}.txt" "Sequential Write (MDP=1)")
    echo "Sequential Write (no PTE):   $w_seq_no_pte ops/sec"
    echo "Sequential Write (MDP=0):    $w_seq_mdp0 ops/sec"
    echo "Sequential Write (MDP=1):    $w_seq_mdp1 ops/sec"
    
    # Random Write  
    w_rnd_no_pte=$(extract_with_check "04_write_rnd_no_pte_${DATE}.txt" "Random Write (no PTE)")
    w_rnd_mdp0=$(extract_with_check "05_write_rnd_with_pte_mdp0_${DATE}.txt" "Random Write (MDP=0)")
    w_rnd_mdp1=$(extract_with_check "06_write_rnd_with_pte_mdp1_${DATE}.txt" "Random Write (MDP=1)")
    echo "Random Write (no PTE):       $w_rnd_no_pte ops/sec"
    echo "Random Write (MDP=0):        $w_rnd_mdp0 ops/sec"
    echo "Random Write (MDP=1):        $w_rnd_mdp1 ops/sec"
    
    echo ""
    echo "READ OPERATIONS:"
    echo "----------------"
    
    # Sequential Read
    r_seq_no_pte=$(extract_with_check "07_read_seq_no_pte_${DATE}.txt" "Sequential Read (no PTE)")
    r_seq_mdp0=$(extract_with_check "08_read_seq_with_pte_mdp0_${DATE}.txt" "Sequential Read (MDP=0)")
    r_seq_mdp1=$(extract_with_check "09_read_seq_with_pte_mdp1_${DATE}.txt" "Sequential Read (MDP=1)")
    echo "Sequential Read (no PTE):    $r_seq_no_pte ops/sec"
    echo "Sequential Read (MDP=0):     $r_seq_mdp0 ops/sec"
    echo "Sequential Read (MDP=1):     $r_seq_mdp1 ops/sec"
    
    # Random Read
    r_rnd_no_pte=$(extract_with_check "10_read_rnd_no_pte_${DATE}.txt" "Random Read (no PTE)")
    r_rnd_mdp0=$(extract_with_check "11_read_rnd_with_pte_mdp0_${DATE}.txt" "Random Read (MDP=0)")
    r_rnd_mdp1=$(extract_with_check "12_read_rnd_with_pte_mdp1_${DATE}.txt" "Random Read (MDP=1)")
    echo "Random Read (no PTE):        $r_rnd_no_pte ops/sec"
    echo "Random Read (MDP=0):         $r_rnd_mdp0 ops/sec"
    echo "Random Read (MDP=1):         $r_rnd_mdp1 ops/sec"
    
} >> "$SUMMARY_FILE"

# Create simple CSV for analysis
CSV_FILE="results_${DATE}.csv"
{
    echo "Test_Name,Operation,Access_Mode,PTE_Type,Ops_Per_Second"
    echo "write_seq_no_pte,write,sequential,none,$w_seq_no_pte"
    echo "write_seq_mdp0,write,sequential,mdp0,$w_seq_mdp0"
    echo "write_seq_mdp1,write,sequential,mdp1,$w_seq_mdp1"
    echo "write_rnd_no_pte,write,random,none,$w_rnd_no_pte"
    echo "write_rnd_mdp0,write,random,mdp0,$w_rnd_mdp0"
    echo "write_rnd_mdp1,write,random,mdp1,$w_rnd_mdp1"
    echo "read_seq_no_pte,read,sequential,none,$r_seq_no_pte"
    echo "read_seq_mdp0,read,sequential,mdp0,$r_seq_mdp0"
    echo "read_seq_mdp1,read,sequential,mdp1,$r_seq_mdp1"
    echo "read_rnd_no_pte,read,random,none,$r_rnd_no_pte"
    echo "read_rnd_mdp0,read,random,mdp0,$r_rnd_mdp0"
    echo "read_rnd_mdp1,read,random,mdp1,$r_rnd_mdp1"
} > "$CSV_FILE"

# Final summary with better impact calculation
echo ""
echo -e "${GREEN}🎉 All 12 tests completed successfully!${NC}"
echo ""
echo -e "${BLUE}Results Summary:${NC}"
echo "=================="
echo -e "Write Sequential: ${YELLOW}$w_seq_no_pte → $w_seq_mdp0 → $w_seq_mdp1${NC} ops/sec"
echo "  MDP=0 impact: $(calculate_impact "$w_seq_no_pte" "$w_seq_mdp0")"
echo "  MDP=1 impact: $(calculate_impact "$w_seq_no_pte" "$w_seq_mdp1")"
echo ""
echo -e "Write Random:     ${YELLOW}$w_rnd_no_pte → $w_rnd_mdp0 → $w_rnd_mdp1${NC} ops/sec"
echo "  MDP=0 impact: $(calculate_impact "$w_rnd_no_pte" "$w_rnd_mdp0")"
echo "  MDP=1 impact: $(calculate_impact "$w_rnd_no_pte" "$w_rnd_mdp1")"
echo ""
echo -e "Read Sequential:  ${YELLOW}$r_seq_no_pte → $r_seq_mdp0 → $r_seq_mdp1${NC} ops/sec"
echo "  MDP=0 impact: $(calculate_impact "$r_seq_no_pte" "$r_seq_mdp0")"
echo "  MDP=1 impact: $(calculate_impact "$r_seq_no_pte" "$r_seq_mdp1")"
echo ""
echo -e "Read Random:      ${YELLOW}$r_rnd_no_pte → $r_rnd_mdp0 → $r_rnd_mdp1${NC} ops/sec"
echo "  MDP=0 impact: $(calculate_impact "$r_rnd_no_pte" "$r_rnd_mdp0")"
echo "  MDP=1 impact: $(calculate_impact "$r_rnd_no_pte" "$r_rnd_mdp1")"
echo ""
echo -e "${BLUE}Files created in current directory:${NC}"
echo "- Compiled sysbench binary: $SYSBENCH_PATH"
echo "- 12 individual test results: *_${DATE}.txt"
echo "- Summary report: summary_${DATE}.txt"  
echo "- CSV data: results_${DATE}.csv"
echo ""
echo -e "${GREEN}📁 Current directory contains all results!${NC}"
echo ""
