# Test the generated LLVM IR

# 1. View the generated LLVM IR
echo "=== Generated LLVM IR ==="
cat test.tspp.ll

echo -e "\n=== Verifying LLVM IR ==="
# 2. Verify the LLVM IR is valid
llvm-as test.tspp.ll -o test.bc
if [ $? -eq 0 ]; then
    echo "✅ LLVM IR is valid!"
else
    echo "❌ LLVM IR has syntax errors"
    exit 1
fi

echo -e "\n=== Compiling to Executable ==="
# 3. Compile the LLVM IR to an executable
clang test.tspp.ll -o test_program
if [ $? -eq 0 ]; then
    echo "✅ Successfully compiled to executable!"
else
    echo "❌ Compilation failed"
    exit 1
fi

echo -e "\n=== Running the Program ==="
# 4. Run the compiled program
./test_program
echo "Exit code: $?"

echo -e "\n=== Alternative: Using lli (LLVM interpreter) ==="
# 5. Alternative: Run directly with LLVM interpreter
lli test.tspp.ll
echo "lli exit code: $?"

echo -e "\n=== Assembly Generation ==="
# 6. Generate assembly code to see what was produced
llc test.tspp.ll -o test.s
echo "✅ Assembly generated in test.s"
echo "First 20 lines of assembly:"
head -20 test.s