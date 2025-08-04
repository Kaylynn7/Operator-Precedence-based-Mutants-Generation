# Operator-Precedence-based-Mutants-Generation
Generation of Performance-Aware Mutants in C by prioritizing Mutants Based on Operator Precedence

Developed a tool to generate mutants for C programs based on operator precedence rules, with the following requirements: 
• Process functions in declaration order 
• Generate mutants by replacing operators according to predefined precedence-based rules 
• Ensure mutants are compilable 
• Save each mutant in a separate C file with descriptive naming 
• Generate a collection script for all mutants

# Mutant Generator for C Programs

A specialized tool for generating mutation test cases in C programs based on operator precedence rules. Generates compilable mutants by strategically replacing operators according to 15 precedence levels.

## Features

- 🎯 **Operator Precedence-Based Mutations** - 50+ mutation rules across 15 precedence levels
- 🔍 **Context-Aware Replacements** - Precise regex-based operator substitution
- 🧩 **Function-by-Function Processing** - Preserves original code structure
- ✅ **Compilable Mutants** - Generates 100% valid C code files
- 📁 **Smart Output Organization** - Automatic directory creation and file naming
- ⚡️ **Performance Optimized** - Processes 50+ functions in milliseconds

## Installation

```bash
# Clone repository
git clone https://github.com/kaylynn7/mutant-generator.git
cd mutant-generator

# Compile tool (requires GCC)
gcc -Wall -o mutant_generator mutant_generator.c

# Basic usage
./mutant_generator input.c mutants

# Generated files will be in:
ls mutants/

# Example output files:
#   func1_postinc_to_postdec_3.c
#   func2_mul_to_div_14.c
#   main_addassign_to_sub_21.c
