#!/bin/bash

# Exit on error
set -e

# 1. Get the input filename
INPUT_FILE="$1"

if [ -z "$INPUT_FILE" ]; then
  echo "❌ Please provide the base filename (e.g., variables.tspp)"
  exit 1
fi

# compile the input file to LLVM IR
echo "=== Compiling $INPUT_FILE to LLVM IR ==="
./build/src/tspp "$INPUT_FILE"

# 2. Derive LLVM IR filename
LLVM_IR="${INPUT_FILE}.ll"

# 3. Display LLVM IR
echo "=== Generated LLVM IR ==="
cat "$LLVM_IR"

echo -e "\n=== Verifying LLVM IR ==="
# 4. Verify the LLVM IR is valid
llvm-as "$LLVM_IR" -o temp.bc
echo "✅ LLVM IR is valid!"

echo -e "\n=== Compiling to Executable ==="
# 5. Compile to native executable
clang "$LLVM_IR" -o temp_exec
echo "✅ Successfully compiled to executable!"

echo -e "\n=== Running the Program ==="
# 6. Run the compiled program
./temp_exec
echo "Exit code: $?"

echo -e "\n=== Alternative: Using lli (LLVM interpreter) ==="
# 7. Run with LLVM interpreter
lli "$LLVM_IR"
echo "lli exit code: $?"

echo -e "\n=== Assembly Generation ==="
# 8. Generate assembly
llc "$LLVM_IR" -o temp.s
echo "✅ Assembly generated in temp.s"
echo "First 20 lines of assembly:"
head -20 temp.s
