# parser_debug.gdb - For parser debugging
set confirm off
set pagination off
b parser/impl/expressions_parser.cpp:parseExpression
run ./sample.tspp

# token_debug.gdb - For token stream debugging
set confirm off
set pagination off
b tokens/stream/vector_stream.cpp:peek
b tokens/stream/vector_stream.cpp:advance
run ./sample.tspp
