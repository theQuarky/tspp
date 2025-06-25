#pragma once
#include <cassert>

/**
 * Debug assertion macros
 * In release builds (NDEBUG defined), these become no-ops
 * In debug builds, they perform runtime checks
 */
#ifdef NDEBUG
#define ASSERT(condition) ((void)0)
#define ASSERT_MSG(condition, message) ((void)0)
#else
/**
 * @brief Verifies that a condition is true
 * @param condition The condition to check
 */
#define ASSERT(condition) assert(condition)

/**
 * @brief Verifies that a condition is true with a custom message
 * @param condition The condition to check
 * @param message Message to display if assertion fails
 */
#define ASSERT_MSG(condition, message) assert((condition) && (message))
#endif

/**
 * @brief Marks code that should never be executed
 * In debug builds, triggers an assertion failure if reached
 */
#define UNREACHABLE() ASSERT_MSG(false, "Unreachable code reached")

/**
 * @brief Makes a class non-copyable
 * Deletes copy constructor and copy assignment operator
 * @param TypeName Name of the class
 */
#define NON_COPYABLE(TypeName)                                                 \
  TypeName(const TypeName &) = delete;                                         \
  TypeName &operator=(const TypeName &) = delete

/**
 * @brief Makes a class non-copyable with virtual destructor
 * Combines NON_COPYABLE with virtual destructor declaration
 * @param TypeName Name of the class
 */
#define NON_COPYABLE_VIRTUAL(TypeName)                                         \
  NON_COPYABLE(TypeName);                                                      \
  virtual ~TypeName() = default

/**
 * @brief Implements the Singleton pattern for a class
 * Deletes copy operations and provides static instance() method
 * @param TypeName Name of the class
 */
#define SINGLETON(TypeName)                                                    \
  NON_COPYABLE(TypeName);                                                      \
                                                                               \
public:                                                                        \
  static TypeName &instance() {                                                \
    static TypeName instance;                                                  \
    return instance;                                                           \
  }                                                                            \
                                                                               \
private:                                                                       \
  TypeName() = default

/**
 * String literal conversion macros
 */
#define STRINGIFY(x) #x          ///< Converts argument to string literal
#define TOSTRING(x) STRINGIFY(x) ///< Converts macro expansion to string literal

/**
 * Compiler-specific optimizations
 * Provides branch prediction hints and inline forcing
 */
#if defined(__GNUC__) || defined(__clang__)
/**
 * @brief Hints that condition is likely to be true
 * Helps compiler optimize branch prediction
 */
#define LIKELY(x) __builtin_expect(!!(x), 1)

/**
 * @brief Hints that condition is likely to be false
 * Helps compiler optimize branch prediction
 */
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

/**
 * @brief Forces function to be inlined
 * Overrides compiler's inlining decisions
 */
#define FORCE_INLINE __attribute__((always_inline))
#else
// Fallbacks for non-GCC/Clang compilers
#define LIKELY(x) (x)
#define UNLIKELY(x) (x)
#define FORCE_INLINE inline
#endif