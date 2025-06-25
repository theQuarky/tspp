# TSPP Linter

A static analysis tool for TypeScript++ files that detects syntax errors, style issues, and suggests best practices.

## Features

- **Syntax Validation**: Checks for basic syntax conformance to the TSPP grammar
- **Style Enforcement**: Enforces consistent style rules like indentation and line length
- **Memory Safety**: Identifies potential memory management issues
- **Best Practices**: Suggests improvements for performance and safety

## Installation

The linter requires Python 3.7 or newer.

```bash
# Make the linter executable
chmod +x tspp_linter.py

# Optional: link to a location in your PATH
ln -s $(pwd)/tspp_linter.py /usr/local/bin/tspp-lint
```

## Usage

```bash
# Lint a single file
./tspp_linter.py path/to/file.tspp

# Lint multiple files
./tspp_linter.py file1.tspp file2.tspp

# Lint all .tspp files in a directory
./tspp_linter.py path/to/directory/

# Use a configuration file
./tspp_linter.py --config path/to/tspp_lint.conf file.tspp

# Ignore specific rules
./tspp_linter.py --ignore E001 W002 file.tspp

# Output in JSON format
./tspp_linter.py --format json file.tspp
```

## Configuration

Create a configuration file to customize the linter's behavior:

```
# Example tspp_lint.conf
max_line_length = 100
indent_size = 4
require_semicolons = true
enforce_types = true
recommend_memory_attrs = true
ignored_rules = [W002, I004]
```

## Linting Rules

### Errors (E)

- `E001`: Missing semicolon
- `E002`: Unmatched curly brace
- `E003`: Unmatched parenthesis
- `E004`: Invalid attribute usage
- `E005`: Missing type annotation
- `E006`: Invalid pointer syntax

### Warnings (W)

- `W001`: Inconsistent indentation
- `W002`: Line too long (> 100 characters)
- `W003`: Missing function return type
- `W004`: Unused variable
- `W005`: Redundant memory attribute
- `W101`: Raw pointer used outside unsafe block
- `W102`: Memory allocated without proper deallocation
- `W103`: Array access without bounds check
- `W104`: Missing memory placement attribute

### Information (I)

- `I001`: Consider using smart pointers instead of raw pointers
- `I002`: Consider adding alignment for performance critical data
- `I003`: Function could be marked as inline
- `I004`: Consider stack allocation for small objects

## Integration with Editors

### VS Code

Add to your VS Code settings.json file:

```json
{
    "typescript.tsserver.pluginPaths": ["path/to/tspp_linter"],
    "typescript.validate.enable": true
}
```

### Vim/Neovim

Add to your vim configuration:

```vim
autocmd BufWritePost *.tspp !tspp_linter.py %
```

## License

MIT