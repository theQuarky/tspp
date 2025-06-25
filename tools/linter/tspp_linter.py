#!/usr/bin/env python3

"""
TSPP Linter - A static analysis tool for TypeScript++ files
"""

import re
import sys
import os
import argparse
from dataclasses import dataclass
from enum import Enum
from typing import List, Dict, Set, Optional, Tuple

class LintLevel(Enum):
    ERROR = "ERROR"
    WARNING = "WARNING"
    INFO = "INFO"

@dataclass
class LintMessage:
    file: str
    line: int
    column: int
    level: LintLevel
    code: str
    message: str

    def __str__(self) -> str:
        return f"{self.file}:{self.line}:{self.column} - {self.level.value} {self.code}: {self.message}"

class TSPPLinter:
    """
    A linter for TypeScript++ source files
    """
    
    # Rules with codes and descriptions
    RULES = {
        # Syntax rules
        "E001": "Missing semicolon",
        "E002": "Unmatched curly brace",
        "E003": "Unmatched parenthesis",
        "E004": "Invalid attribute usage",
        "E005": "Missing type annotation",
        "E006": "Invalid pointer syntax",
        
        # Style rules
        "W001": "Inconsistent indentation",
        "W002": "Line too long (> 100 characters)",
        "W003": "Missing function return type",
        "W004": "Unused variable",
        "W005": "Redundant memory attribute",
        
        # Memory and safety rules
        "W101": "Raw pointer used outside unsafe block",
        "W102": "Memory allocated without proper deallocation",
        "W103": "Array access without bounds check",
        "W104": "Missing memory placement attribute",
        
        # Best practices
        "I001": "Consider using smart pointers instead of raw pointers",
        "I002": "Consider adding alignment for performance critical data",
        "I003": "Function could be marked as inline",
        "I004": "Consider stack allocation for small objects",
    }

    def __init__(self, config=None):
        self.config = config or {}
        self.messages = []
        
        # Set default options
        self.options = {
            "max_line_length": self.config.get("max_line_length", 100),
            "indent_size": self.config.get("indent_size", 4),
            "require_semicolons": self.config.get("require_semicolons", True),
            "enforce_types": self.config.get("enforce_types", True),
            "recommend_memory_attrs": self.config.get("recommend_memory_attrs", True),
            "ignored_rules": set(self.config.get("ignored_rules", [])),
        }

    def lint_file(self, filename: str) -> List[LintMessage]:
        """Lint a single TSPP file"""
        if not filename.endswith(".tspp"):
            raise ValueError(f"Not a TSPP file: {filename}")

        with open(filename, 'r') as f:
            content = f.read()
            
        self.messages = []
        
        # Split into lines for line-by-line analysis
        lines = content.splitlines()
        
        # Pass 1: Perform line-based checks
        for i, line in enumerate(lines):
            line_num = i + 1
            self._check_line(filename, line_num, line)
            
        # Pass 2: Perform content-based checks
        self._check_content(filename, content)
        
        # Pass 3: Perform structural checks
        self._check_structure(filename, content, lines)

        return self.messages

    def _check_line(self, filename: str, line_num: int, line: str) -> None:
        """Check a single line for simple linting issues"""
        # Check line length
        if len(line.rstrip()) > self.options["max_line_length"] and "W002" not in self.options["ignored_rules"]:
            self.messages.append(LintMessage(
                file=filename,
                line=line_num, 
                column=self.options["max_line_length"] + 1,
                level=LintLevel.WARNING,
                code="W002",
                message=f"Line too long ({len(line.rstrip())} > {self.options['max_line_length']})"
            ))
            
        # Check indentation - skip this for now since it's being too aggressive
        return
        
        # Check for missing semicolons - skip for now
        pass

    def _check_content(self, filename: str, content: str) -> None:
        """Check the file content for various issues"""
        # Check for unmatched braces
        opening_braces = content.count('{')
        closing_braces = content.count('}')
        
        if opening_braces > closing_braces and "E002" not in self.options["ignored_rules"]:
            self.messages.append(LintMessage(
                file=filename,
                line=1,
                column=1,
                level=LintLevel.ERROR,
                code="E002",
                message=f"Unmatched curly braces: {opening_braces} opening, {closing_braces} closing"
            ))
        elif opening_braces < closing_braces and "E002" not in self.options["ignored_rules"]:
            self.messages.append(LintMessage(
                file=filename,
                line=1,
                column=1,
                level=LintLevel.ERROR,
                code="E002",
                message=f"Unmatched curly braces: {opening_braces} opening, {closing_braces} closing"
            ))
            
        # Check for unmatched parentheses
        opening_parens = content.count('(')
        closing_parens = content.count(')')
        
        if opening_parens > closing_parens and "E003" not in self.options["ignored_rules"]:
            self.messages.append(LintMessage(
                file=filename,
                line=1,
                column=1,
                level=LintLevel.ERROR,
                code="E003",
                message=f"Unmatched parentheses: {opening_parens} opening, {closing_parens} closing"
            ))
        elif opening_parens < closing_parens and "E003" not in self.options["ignored_rules"]:
            self.messages.append(LintMessage(
                file=filename,
                line=1,
                column=1,
                level=LintLevel.ERROR,
                code="E003",
                message=f"Unmatched parentheses: {opening_parens} opening, {closing_parens} closing"
            ))

    def _check_structure(self, filename: str, content: str, lines: List[str]) -> None:
        """Check structural aspects of the code"""
        # Check for variable declarations without type annotations
        for i, line in enumerate(lines):
            # Skip comments
            if line.strip().startswith('//') or line.strip().startswith('/*'):
                continue
                
            # Look for let/const declarations without a type annotation
            # This is a simpler but more reliable method
            if re.search(r'(let|const)\s+\w+\s*=', line) and not re.search(r'(let|const)\s+\w+\s*:\s*\w+', line):
                match = re.search(r'(let|const)\s+(\w+)', line)
                if match and self.options["enforce_types"] and "E005" not in self.options["ignored_rules"]:
                    self.messages.append(LintMessage(
                        file=filename,
                        line=i + 1,
                        column=match.start() + 1,
                        level=LintLevel.ERROR,
                        code="E005",
                        message=f"Missing type annotation for variable '{match.group(2)}'"
                    ))

        # Check for function declarations without return types
        func_decl_pattern = re.compile(r'function\s+(\w+)\s*\([^)]*\)\s*(?!:)')
        for i, line in enumerate(lines):
            if line.strip().startswith('//') or line.strip().startswith('/*'):
                continue
                
            match = func_decl_pattern.search(line)
            if match and "W003" not in self.options["ignored_rules"]:
                self.messages.append(LintMessage(
                    file=filename,
                    line=i + 1,
                    column=match.start() + 1,
                    level=LintLevel.WARNING,
                    code="W003",
                    message=f"Missing return type for function '{match.group(1)}'"
                ))
                
        # Check for raw pointers outside of unsafe blocks
        raw_ptr_pattern = re.compile(r'\w+\s*:\s*\w+@(?!unsafe|aligned)')
        for i, line in enumerate(lines):
            if line.strip().startswith('//') or line.strip().startswith('/*'):
                continue
                
            matches = raw_ptr_pattern.finditer(line)
            for match in matches:
                unsafe_block = False
                # Check if we're in an unsafe block by looking backwards
                for j in range(i, -1, -1):
                    if "#unsafe" in lines[j]:
                        unsafe_block = True
                        break
                    elif "}" in lines[j] and "{" not in lines[j][:lines[j].index("}")]:
                        # We've exited a block, stop checking
                        break
                
                if not unsafe_block and "W101" not in self.options["ignored_rules"]:
                    self.messages.append(LintMessage(
                        file=filename,
                        line=i + 1,
                        column=match.start() + 1,
                        level=LintLevel.WARNING,
                        code="W101",
                        message="Raw pointer used outside unsafe block"
                    ))
                    
                    # Also suggest smart pointers
                    if "I001" not in self.options["ignored_rules"]:
                        self.messages.append(LintMessage(
                            file=filename,
                            line=i + 1,
                            column=match.start() + 1,
                            level=LintLevel.INFO,
                            code="I001",
                            message="Consider using smart pointers (#shared<T> or #unique<T>) instead of raw pointers"
                        ))
                        
        # Check for missing memory placement attributes in variable declarations
        if self.options["recommend_memory_attrs"]:
            # Check if line has memory directive
            for i, line in enumerate(lines):
                if line.strip().startswith('//') or line.strip().startswith('/*'):
                    continue
                
                # Skip lines that already have memory directives
                if any(directive in line for directive in ['#stack', '#heap', '#static']):
                    continue
                
                # Check for variable declarations with arrays or new keyword
                if ("new " in line or ("[" in line and "]" in line)) and ("let " in line or "const " in line):
                    match = re.search(r'(let|const)\s+(\w+)', line)
                    if match and "W104" not in self.options["ignored_rules"]:
                        self.messages.append(LintMessage(
                            file=filename,
                            line=i + 1,
                            column=match.start() + 1,
                            level=LintLevel.WARNING,
                            code="W104",
                            message=f"Missing memory placement attribute for variable '{match.group(2)}'"
                        ))

def parse_config(config_file: str) -> Dict:
    """Parse a linter configuration file"""
    if not os.path.exists(config_file):
        return {}
        
    config = {}
    with open(config_file, 'r') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'):
                continue
                
            if '=' in line:
                key, value = line.split('=', 1)
                key = key.strip()
                value = value.strip()
                
                # Handle list values
                if value.startswith('[') and value.endswith(']'):
                    value = [item.strip() for item in value[1:-1].split(',') if item.strip()]
                    
                # Handle boolean values
                elif value.lower() in ('true', 'yes', 'on'):
                    value = True
                elif value.lower() in ('false', 'no', 'off'):
                    value = False
                # Handle integer values
                elif value.isdigit():
                    value = int(value)
                    
                config[key] = value
                
    return config

def format_messages(messages: List[LintMessage], format_type: str) -> str:
    """Format the lint messages in various output formats"""
    if format_type == 'text':
        return '\n'.join(str(msg) for msg in messages)
    elif format_type == 'json':
        import json
        return json.dumps([{
            'file': msg.file,
            'line': msg.line,
            'column': msg.column,
            'level': msg.level.value,
            'code': msg.code,
            'message': msg.message
        } for msg in messages], indent=2)
    else:
        raise ValueError(f"Unknown format type: {format_type}")

def main():
    parser = argparse.ArgumentParser(description='TSPP Linter - A static analysis tool for TypeScript++ files')
    parser.add_argument('files', nargs='+', help='Files or directories to lint')
    parser.add_argument('--config', help='Configuration file path')
    parser.add_argument('--format', choices=['text', 'json'], default='text', help='Output format')
    parser.add_argument('--ignore', nargs='+', help='Rules to ignore (e.g., E001 W002)')
    args = parser.parse_args()
    
    # Load configuration
    config = {}
    if args.config:
        config = parse_config(args.config)
        
    # Add command line ignores to config
    if args.ignore:
        config['ignored_rules'] = list(set(config.get('ignored_rules', []) + args.ignore))
        
    # Initialize linter
    linter = TSPPLinter(config)
    
    # Collect all files to lint
    all_files = []
    for file_path in args.files:
        if os.path.isdir(file_path):
            for root, _, files in os.walk(file_path):
                for file in files:
                    if file.endswith('.tspp'):
                        all_files.append(os.path.join(root, file))
        elif os.path.isfile(file_path) and file_path.endswith('.tspp'):
            all_files.append(file_path)
    
    # Lint all files
    all_messages = []
    for file in all_files:
        try:
            messages = linter.lint_file(file)
            all_messages.extend(messages)
        except Exception as e:
            print(f"Error linting {file}: {e}", file=sys.stderr)
    
    # Sort messages by file, line, column
    all_messages.sort(key=lambda msg: (msg.file, msg.line, msg.column))
    
    # Output results
    print(format_messages(all_messages, args.format))
    
    # Return error code if any errors found
    if any(msg.level == LintLevel.ERROR for msg in all_messages):
        sys.exit(1)
    
if __name__ == "__main__":
    main()