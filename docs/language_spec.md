# TSPP (TypeScript++) Language Documentation

## Overview
TSPP is a systems programming language that combines TypeScript's ergonomic syntax with C++'s performance characteristics. It provides zero-cost abstractions, fine-grained memory control, and compile-time optimizations while maintaining developer-friendly syntax.

## Core Features

### 1. Memory Model

#### Memory Placement Attributes
```typescript
#stack let x: int = 42;    // Stack allocation
#heap let y: int = 42;     // Heap allocation
#static let z: int = 42;   // Static/global allocation
```

#### Pointer Types
```typescript
let ptr: int@;             // Raw pointer (@ instead of *)
let safePtr: int@safe;     // Safe pointer
let alignedPtr: int@aligned(64); // Aligned pointer
```

#### Smart Pointers
```typescript
let shared: #shared<T>;    // Reference counted
let unique: #unique<T>;    // Unique ownership
```

### 2. Performance Features

#### SIMD and Vectorization
```typescript
#simd
function vectorAdd(a: float[4], b: float[4]): float[4] {
    return a + b;  // Automatically vectorized
}
```

#### Cache Control
```typescript
#aligned(64) 
class CacheOptimized {
    #hot data: float[16];  // Frequently accessed data
}

#prefetch
function prefetchData(data: float@) {
    // Data will be prefetched
}
```

#### Compile-Time Execution
```typescript
#const
function fibonacci(n: int): int {
    if (n <= 1) return n;
    return fibonacci(n - 1) + fibonacci(n - 2);
}

#constexpr let fib10 = fibonacci(10);  // Computed at compile time
```

### 3. Type System

#### Zero-Cost Abstractions
```typescript
#zerocast
interface Comparable<T> {
    compareTo(other: T): number;
}

#inline
function sort<T extends Comparable<T>>(array: T[]): void {
    // Compile-time interface checking, no runtime overhead
}
```

#### Type Constraints
```typescript
function add<T extends number>(a: T, b: T): T {
    return a + b;
}
```

### 4. Safety and Control

#### Unsafe Operations
```typescript
#unsafe
function copyMemory<T>(src: T@, dest: T@, size: int): void {
    // Direct memory manipulation
}
```

#### Thread Control
```typescript
#pinned(1)
function criticalTask() {
    // Runs on CPU core 1
}

#atomic
function syncOperation() {
    // Thread-safe operations
}
```

## Compilation Pipeline

1. **Lexical Analysis**
   - Input: TSPP source code
   - Output: Token stream
   - Components: Currently implemented in lexer/

2. **Syntax Analysis**
   - Input: Token stream
   - Output: AST
   - Components: Currently implemented in parser/

3. **Semantic Analysis**
   - Type checking
   - Memory safety verification
   - Attribute validation

4. **IR Generation**
   - LLVM IR generation
   - Optimization hints from attributes

5. **Optimization**
   - LLVM optimization passes
   - Custom TSPP-specific optimizations

6. **Code Generation**
   - Platform-specific code generation
   - Debug information generation

## Implementation Status

### Currently Implemented
- Basic lexer and token system
- Parser foundation
- AST structure
- Basic type system
- Variable declarations
- Basic expressions and operations

### Next Steps
1. Implement attribute system
2. Add LLVM IR generation
3. Integrate memory management system
4. Implement zero-cost abstractions
5. Add SIMD support
6. Create optimization passes

## Performance Considerations

### Memory Layout
- Stack allocation by default
- Explicit heap allocation when needed
- Cache-aligned structures
- SIMD-friendly data organization

### Optimizations
- Compile-time evaluation
- Function inlining
- Loop unrolling
- Dead code elimination
- Constant propagation

### Zero-Cost Features
- Templates without runtime overhead
- Interface checks at compile-time
- Inlined functions
- Optimized smart pointers

## Best Practices

1. **Memory Management**
   - Use stack allocation for small, scoped objects
   - Use smart pointers for heap management
   - Align data structures for cache efficiency

2. **Performance**
   - Mark hot paths with #hot
   - Use SIMD for data-parallel operations
   - Leverage compile-time execution
   - Use appropriate memory layout attributes

3. **Safety**
   - Minimize unsafe blocks
   - Use safe pointers by default
   - Validate memory access in unsafe code

4. **Type System**
   - Utilize zero-cost abstractions
   - Use appropriate type constraints
   - Leverage compile-time type checking