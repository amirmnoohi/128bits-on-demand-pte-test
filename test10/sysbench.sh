#!/bin/bash

# PTE Metadata Performance Testing Script
# 8 Tests: 4 scenarios (write-seq, write-rnd, read-seq, read-rnd) x 2 conditions (with/without PTE)

set -e  # Exit on error

# Configuration
SYSBENCH_PATH="./sysbench"
TEST_DURATION="--time=30"  # 30 seconds per test
DATE=$(date +"%Y%m%d_%H%M%S")

# Function to run a test and save results
run_test() {
    local test_name="$1"
    local command="$2"
    local output_file="${test_name}_${DATE}.txt"
    
    echo "Running Test: $test_name"
    echo "Command: $command"
    echo "Output: $output_file"
    
    # Write test header to file
    {
        echo "=== PTE Metadata Test: $test_name ==="
        echo "Date: $(date)"
        echo "Command: $command"
        echo "Duration: $TEST_DURATION"
        echo "System: $(uname -a)"
        echo "CPU Info: $(grep 'model name' /proc/cpuinfo | head -1)"
        echo "Memory: $(grep MemTotal /proc/meminfo)"
        echo ""
    } > "$output_file"
    
    # Run the actual test and append to file
    eval "$command" >> "$output_file" 2>&1
    
    # Add separator
    echo "" >> "$output_file"
    echo "=== End of Test ===" >> "$output_file"
    
    echo "✅ Completed: $test_name"
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
echo "=== PTE Metadata Performance Testing Suite ==="
echo "8 Tests Total: 4 scenarios x 2 conditions (with/without PTE)"
echo "Date: $(date)"
echo "Results will be saved to current directory"
echo "Test duration: $TEST_DURATION"

# Quick test of calculation function
echo "Testing calculation function..."
test_calc=$(calculate_impact "1000" "100")
echo "Test calculation (1000 → 100): $test_calc"
echo ""

echo "🔹 Starting Write Tests..."

# Test 1: Write Sequential - WITHOUT PTE
run_test "01_write_seq_no_pte" \
    "$SYSBENCH_PATH $TEST_DURATION --memory-oper=write --memory-access-mode=seq memory run"

# Test 2: Write Sequential - WITH PTE
run_test "02_write_seq_with_pte" \
    "sudo $SYSBENCH_PATH $TEST_DURATION --memory-oper=write --memory-access-mode=seq --memory-pte-meta=on memory run"

# Test 3: Write Random - WITHOUT PTE
run_test "03_write_rnd_no_pte" \
    "$SYSBENCH_PATH $TEST_DURATION --memory-oper=write --memory-access-mode=rnd memory run"

# Test 4: Write Random - WITH PTE
run_test "04_write_rnd_with_pte" \
    "sudo $SYSBENCH_PATH $TEST_DURATION --memory-oper=write --memory-access-mode=rnd --memory-pte-meta=on memory run"

echo "🔹 Starting Read Tests..."

# Test 5: Read Sequential - WITHOUT PTE
run_test "05_read_seq_no_pte" \
    "$SYSBENCH_PATH $TEST_DURATION --memory-oper=read --memory-access-mode=seq memory run"

# Test 6: Read Sequential - WITH PTE
run_test "06_read_seq_with_pte" \
    "sudo $SYSBENCH_PATH $TEST_DURATION --memory-oper=read --memory-access-mode=seq --memory-pte-meta=on memory run"

# Test 7: Read Random - WITHOUT PTE
run_test "07_read_rnd_no_pte" \
    "$SYSBENCH_PATH $TEST_DURATION --memory-oper=read --memory-access-mode=rnd memory run"

# Test 8: Read Random - WITH PTE
run_test "08_read_rnd_with_pte" \
    "sudo $SYSBENCH_PATH $TEST_DURATION --memory-oper=read --memory-access-mode=rnd --memory-pte-meta=on memory run"

echo "🔹 Generating Summary Report..."

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
    echo "8 Tests Performed:"
    echo "1. Write Sequential - No PTE"
    echo "2. Write Sequential - With PTE" 
    echo "3. Write Random - No PTE"
    echo "4. Write Random - With PTE"
    echo "5. Read Sequential - No PTE"
    echo "6. Read Sequential - With PTE"
    echo "7. Read Random - No PTE"
    echo "8. Read Random - With PTE"
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
    w_seq_pte=$(extract_with_check "02_write_seq_with_pte_${DATE}.txt" "Sequential Write (with PTE)")
    echo "Sequential Write (no PTE):   $w_seq_no_pte ops/sec"
    echo "Sequential Write (with PTE): $w_seq_pte ops/sec"
    
    # Random Write  
    w_rnd_no_pte=$(extract_with_check "03_write_rnd_no_pte_${DATE}.txt" "Random Write (no PTE)")
    w_rnd_pte=$(extract_with_check "04_write_rnd_with_pte_${DATE}.txt" "Random Write (with PTE)")
    echo "Random Write (no PTE):       $w_rnd_no_pte ops/sec"
    echo "Random Write (with PTE):     $w_rnd_pte ops/sec"
    
    echo ""
    echo "READ OPERATIONS:"
    echo "----------------"
    
    # Sequential Read
    r_seq_no_pte=$(extract_with_check "05_read_seq_no_pte_${DATE}.txt" "Sequential Read (no PTE)")
    r_seq_pte=$(extract_with_check "06_read_seq_with_pte_${DATE}.txt" "Sequential Read (with PTE)")
    echo "Sequential Read (no PTE):    $r_seq_no_pte ops/sec"
    echo "Sequential Read (with PTE):  $r_seq_pte ops/sec"
    
    # Random Read
    r_rnd_no_pte=$(extract_with_check "07_read_rnd_no_pte_${DATE}.txt" "Random Read (no PTE)")
    r_rnd_pte=$(extract_with_check "08_read_rnd_with_pte_${DATE}.txt" "Random Read (with PTE)")
    echo "Random Read (no PTE):        $r_rnd_no_pte ops/sec"
    echo "Random Read (with PTE):      $r_rnd_pte ops/sec"
    
} >> "$SUMMARY_FILE"

# Create simple CSV for analysis
CSV_FILE="results_${DATE}.csv"
{
    echo "Test_Name,Operation,Access_Mode,PTE_Enabled,Ops_Per_Second"
    echo "write_seq_no_pte,write,sequential,no,$w_seq_no_pte"
    echo "write_seq_with_pte,write,sequential,yes,$w_seq_pte"
    echo "write_rnd_no_pte,write,random,no,$w_rnd_no_pte"
    echo "write_rnd_with_pte,write,random,yes,$w_rnd_pte"
    echo "read_seq_no_pte,read,sequential,no,$r_seq_no_pte"
    echo "read_seq_with_pte,read,sequential,yes,$r_seq_pte"
    echo "read_rnd_no_pte,read,random,no,$r_rnd_no_pte"
    echo "read_rnd_with_pte,read,random,yes,$r_rnd_pte"
} > "$CSV_FILE"

# Final summary with better impact calculation
echo ""
echo "🎉 All 8 tests completed successfully!"
echo ""
echo "Results Summary:"
echo "=================="
echo "Write Sequential: $w_seq_no_pte → $w_seq_pte ops/sec ($(calculate_impact "$w_seq_no_pte" "$w_seq_pte"))"
echo "Write Random:     $w_rnd_no_pte → $w_rnd_pte ops/sec ($(calculate_impact "$w_rnd_no_pte" "$w_rnd_pte"))"
echo "Read Sequential:  $r_seq_no_pte → $r_seq_pte ops/sec ($(calculate_impact "$r_seq_no_pte" "$r_seq_pte"))"
echo "Read Random:      $r_rnd_no_pte → $r_rnd_pte ops/sec ($(calculate_impact "$r_rnd_no_pte" "$r_rnd_pte"))"
echo ""
echo "Files created in current directory:"
echo "- 8 individual test results: *_${DATE}.txt"
echo "- Summary report: summary_${DATE}.txt"  
echo "- CSV data: results_${DATE}.csv"
echo ""
echo "📁 Current directory contains all results!"