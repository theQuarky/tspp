add_library(core
    core/diagnostics/error_reporter.cpp
    core/utils/file_utils.cpp
    core/utils/log_utils.cpp
    core/utils/string_utils.cpp
)

add_library(tokens
    tokens/stream/token_stream.cpp
)

add_library(lexer
    lexer/lexer.cpp
    lexer/scanner/base/scanner_base.cpp
    lexer/scanner/specialized/identifier_scanner.cpp
    lexer/scanner/specialized/number_scanner.cpp
    lexer/scanner/specialized/operator_scanner.cpp
    lexer/scanner/specialized/string_scanner.cpp
    lexer/scanner/token_scanner.cpp
    lexer/state/lexer_state.cpp
)

add_library(parser
    parser/parser.cpp
    parser/visitors/parse_visitor/base/base_parse_visitor.cpp
    parser/visitors/parse_visitor/expression/expression_parse_visitor.cpp
    parser/visitors/parse_visitor/declaration/declaration_parse_visitor.cpp
    parser/visitors/parse_visitor/statement/statement_parse_visitor.cpp
    parser/visitors/type_check_visitor/resolved_type.cpp
    parser/visitors/type_check_visitor/type_check_visitor.cpp
    parser/visitors/type_check_visitor/type_scope.cpp
)

# New codegen library
add_library(codegen
    codegen/codegen_errors.cpp
    codegen/codegen_options.cpp
    codegen/llvm/llvm_context.cpp
    codegen/llvm/llvm_type_builder.cpp
    codegen/llvm/llvm_code_gen.cpp
    codegen/llvm/llvm_value.cpp
    codegen/llvm/llvm_function.cpp
    codegen/llvm/llvm_utils.cpp
    codegen/llvm/llvm_optimizer.cpp
)

add_library(repl
    repl/repl.cpp
)

# Find LLVM package
find_package(LLVM REQUIRED CONFIG)

# Include LLVM directories
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Link dependencies
target_link_libraries(core PUBLIC tokens)
target_link_libraries(tokens PUBLIC core)
target_link_libraries(lexer PUBLIC core tokens)
target_link_libraries(parser PUBLIC core tokens)
target_link_libraries(repl PUBLIC core tokens lexer parser)

# Link codegen with LLVM and other dependencies
target_link_libraries(codegen 
    PUBLIC 
        core 
        parser
        LLVM
)

# Include directories
target_include_directories(core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(tokens PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(lexer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(parser PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(codegen PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(repl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Main executable
add_executable(tspp main.cpp)

target_link_libraries(tspp
    PRIVATE
        core
        tokens
        lexer
        parser
        codegen
        repl
)