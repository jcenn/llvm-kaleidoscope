import os
import subprocess
import glob

def main():
    compiler_path = "cmake-build-debug/llvm_kaleidoscope"
    
    # ANSI escape codes for colors
    COLOR_GREEN = "\033[92m"
    COLOR_RED = "\033[91m"
    COLOR_RESET = "\033[0m"

    if not os.path.exists(compiler_path):
        print(f"Compiler not found at {compiler_path}. Please build it first.")
        return

    test_files = glob.glob("programs/**/*.kld", recursive=True)
    
    if not test_files:
        print("No .kld test files found in programs/ directory.")
        return

    passed_count = 0
    failed_count = 0

    for kld_file in test_files:
        base_path = os.path.splitext(kld_file)[0]
        ll_file = base_path + ".ll"
        
        print(f"Running {kld_file}...")
        
        # Parse first line for expected exit code
        expected_exit_code = None
        with open(kld_file, 'r') as f:
            first_line = f.readline().strip()
            if first_line.startswith("// EXPECTS "):
                try:
                    expected_exit_code = int(first_line[len("// EXPECT "):].strip())
                except ValueError:
                    print(f"  {COLOR_RED}[WARNING]{COLOR_RESET} Invalid EXPECT format in {kld_file}")
        
        # Run the compiler
        compile_result = subprocess.run([compiler_path, base_path], capture_output=True, text=True)
        if compile_result.returncode != 0:
            print(f"  {COLOR_RED}[FAILED]{COLOR_RESET} Compilation failed for {kld_file}")
            print(f"  Stderr: {compile_result.stderr.strip()}")
            failed_count += 1
            continue

        # Run lli
        lli_result = subprocess.run(["lli", ll_file], capture_output=True, text=True)
        
        # Note: In POSIX, a return value of -1 from main typically results in exit code 255.
        actual_code = lli_result.returncode
        if actual_code == 255:
            actual_code = -1
            
        if expected_exit_code is not None:
            if actual_code == expected_exit_code:
                print(f"  {COLOR_GREEN}[PASSED]{COLOR_RESET} Returned {actual_code} (Expected {expected_exit_code})")
                passed_count += 1
            else:
                print(f"  {COLOR_RED}[FAILED]{COLOR_RESET} Returned {actual_code} (Expected {expected_exit_code})")
                failed_count += 1
        else:
            if actual_code != -1:
                print(f"  {COLOR_GREEN}[PASSED]{COLOR_RESET} Returned {actual_code}")
                passed_count += 1
            else:
                print(f"  {COLOR_RED}[FAILED]{COLOR_RESET} Returned {actual_code} (Expected != -1)")
                failed_count += 1

    print("\n--- Summary ---")
    print(f"Total:  {passed_count + failed_count}")
    print(f"Passed: {passed_count}")
    print(f"Failed: {failed_count}")

if __name__ == "__main__":
    main()
