# lexer_debug.gdb
set confirm off
set pagination off

# Break at lexer state
b lexer::LexerState::getCurrentChar

commands
    printf "\n=== LexerState::getCurrentChar ===\n"
    printf "Source size: %d\n", source_.size()
    printf "Current position: %d\n", position_
    printf "IsAtEnd: %d\n", isAtEnd()
    
    # Examine the lexer state
    print *this
    
    # Pause for inspection
    printf "\nCommands: c (continue), s (step), n (next), p expr, bt\n"
end

# Run with input
run ./sample.tspp