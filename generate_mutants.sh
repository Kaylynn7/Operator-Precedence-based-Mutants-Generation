#!/bin/bash

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <input_file.c> <output_directory>"
    exit 1
fi

INPUT_FILE="$1"
OUTPUT_DIR="$2"

rm -rf "$OUTPUT_DIR"
mkdir -p "$OUTPUT_DIR"

gcc -g -o mutant_generator mutant_generator.c
if [ $? -ne 0 ]; then
    echo "Compilation failed!"
    exit 1
fi

./mutant_generator "$INPUT_FILE" "$OUTPUT_DIR"

COMPILE_SCRIPT="${OUTPUT_DIR}/compile_all.sh"
echo "#!/bin/bash" > "$COMPILE_SCRIPT"
echo "echo 'Compiling mutants...'" >> "$COMPILE_SCRIPT"
echo "success=0" >> "$COMPILE_SCRIPT"
echo "fail=0" >> "$COMPILE_SCRIPT"

find "$OUTPUT_DIR" -name "*.c" | while read -r mutant; do
    base=$(basename "$mutant" .c)
    obj="${mutant%.c}.o"
    echo "echo -n 'Compiling $base... '" >> "$COMPILE_SCRIPT"
    echo "gcc -c \"$mutant\" -o \"$obj\" 2>/dev/null" >> "$COMPILE_SCRIPT"
    echo "if [ \$? -eq 0 ]; then" >> "$COMPILE_SCRIPT"
    echo "    echo 'OK'" >> "$COMPILE_SCRIPT"
    echo "    ((success++))" >> "$COMPILE_SCRIPT"
    echo "else" >> "$COMPILE_SCRIPT"
    echo "    echo 'FAIL'" >> "$COMPILE_SCRIPT"
    echo "    rm -f \"$mutant\"" >> "$COMPILE_SCRIPT"
    echo "    ((fail++))" >> "$COMPILE_SCRIPT"
    echo "fi" >> "$COMPILE_SCRIPT"
done

echo "echo ''" >> "$COMPILE_SCRIPT"
echo "echo \"Compilation results:\"" >> "$COMPILE_SCRIPT"
echo "echo \"Successful: \$success\"" >> "$COMPILE_SCRIPT"
echo "echo \"Failed: \$fail\"" >> "$COMPILE_SCRIPT"
chmod +x "$COMPILE_SCRIPT"

echo "Run compilation with: $OUTPUT_DIR/compile_all.sh"