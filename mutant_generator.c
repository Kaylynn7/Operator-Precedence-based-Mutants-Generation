#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <unistd.h>

#define MAX_FUNCTIONS 50
#define MAX_LINE_LEN 1024
#define MAX_RULES 50
#define MAX_FILENAME_LEN 512

typedef struct MutationRule {
    int precedence;
    char *pattern;
    char *replacement;
    char *suffix;
    regex_t regex;
} MutationRule;

typedef struct Function {
    char name[256];
    int start_line;
    int end_line;
} Function;

char **file_lines;
int total_lines = 0;
bool *is_comment_line = NULL;
Function functions[MAX_FUNCTIONS];
int function_count = 0;

void compile_rules(void);
void parse_source_file(FILE *file);
void generate_mutant(const Function *func, const MutationRule *rule, const char *output_dir);
void generate_mutants(const char *output_dir);
void free_resources(void);

MutationRule rules[] = {
    // Precedence 1
    {1, "\\->", ".", "ptr_to_member", {0}},
    {1, "\\.", "->", "member_to_ptr", {0}},
    
    // Precedence 2
    {2, "\\+\\+", "--", "postinc_to_postdec", {0}},
    {2, "--", "++", "postdec_to_postinc", {0}},
    {2, "(\\s|^)\\+([^+=])", "\\1-\\2", "unaryplus_to_minus", {0}},
    {2, "(\\s|^)-([^-=])", "\\1+\\2", "unaryminus_to_plus", {0}},
    
    // Precedence 3
    {3, "\\*", "/", "mul_to_div", {0}},
    {3, "\\*", "%", "mul_to_mod", {0}},
    {3, "/", "\\*", "div_to_mul", {0}},
    {3, "/", "%", "div_to_mod", {0}},
    {3, "%", "\\*", "mod_to_mul", {0}},
    {3, "%", "/", "mod_to_div", {0}},
    
    // Precedence 4
    {4, "\\+", "-", "add_to_sub", {0}},
    {4, "-", "+", "sub_to_add", {0}},
    
    // Precedence 5
    {5, "<<", ">>", "shiftleft_to_right", {0}},
    {5, ">>", "<<", "shiftright_to_left", {0}},
    
    // Precedence 6
    {6, "<", ">", "lt_to_gt", {0}},
    {6, ">", "<", "gt_to_lt", {0}},
    {6, "<=", ">=", "lte_to_gte", {0}},
    {6, ">=", "<=", "gte_to_lte", {0}},
    
    // Precedence 7
    {7, "==", "!=", "eq_to_neq", {0}},
    {7, "!=", "==", "neq_to_eq", {0}},
    
    // Precedence 8-10
    {8, " & ", " ^ ", "bitand_to_xor", {0}},
    {8, " & ", " | ", "bitand_to_or", {0}},
    {9, " \\^ ", " & ", "bitxor_to_and", {0}},
    {10, " \\| ", " & ", "bitor_to_and", {0}},
    
    // Precedence 11-12
    {11, "&&", "||", "logand_to_or", {0}},
    {12, "\\|\\|", "&&", "logor_to_and", {0}},
    
    // Precedence 14
    {14, "\\+=", "-=", "addassign_to_sub", {0}},
    {14, "-=", "+=", "subassign_to_add", {0}},
    {14, "\\*=", "/=", "mulassign_to_div", {0}},
    {14, "/=", "\\*=", "divassign_to_mul", {0}},
    
    {0, NULL, NULL, NULL, {0}}
};

void compile_rules(void) {
    for (int i = 0; rules[i].pattern != NULL; i++) {
        int ret = regcomp(&rules[i].regex, rules[i].pattern, REG_EXTENDED);
        if (ret != 0) {
            char error[100];
            regerror(ret, &rules[i].regex, error, sizeof(error));
            fprintf(stderr, "Regex error compiling '%s': %s\n", rules[i].pattern, error);
            exit(EXIT_FAILURE);
        }
    }
}

void free_resources(void) {
    for (int i = 0; i < total_lines; i++) free(file_lines[i]);
    free(file_lines);
    free(is_comment_line);
    for (int i = 0; rules[i].pattern != NULL; i++) regfree(&rules[i].regex);
}

void parse_source_file(FILE *file) {
    char line[MAX_LINE_LEN];
    int brace_count = 0;
    bool in_comment_block = false;
    int current_func = -1;

    // Store entire file
    rewind(file);
    file_lines = malloc(MAX_LINE_LEN * sizeof(char *));
    while (fgets(line, MAX_LINE_LEN, file)) {
        file_lines[total_lines++] = strdup(line);
    }

    // Detect comments and functions
    is_comment_line = calloc(total_lines, sizeof(bool));
    for (int i = 0; i < total_lines; i++) {
        char *ptr = file_lines[i];
        bool is_comment = false;
        char *trimmed = ptr;
        
        // Trim leading whitespace
        while (*trimmed == ' ' || *trimmed == '\t') trimmed++;

        // Comment detection
        if (in_comment_block) {
            is_comment = true;
            char *end_block = strstr(ptr, "*/");
            if (end_block) {
                in_comment_block = false;
                if (strstr(end_block + 2, "/*")) in_comment_block = true;
            }
        } else {
            if (strncmp(trimmed, "//", 2) == 0) {
                is_comment = true;
            }
            else if (strncmp(trimmed, "/*", 2) == 0) {
                is_comment = true;
                if (!strstr(trimmed, "*/")) in_comment_block = true;
            }
        }
        is_comment_line[i] = is_comment;

        // Function detection (improved regex)
        if (!in_comment_block && !is_comment && current_func == -1) {
            regex_t func_regex;
            regcomp(&func_regex, 
                "^\\s*[a-zA-Z_][a-zA-Z0-9_\\*\\s]+\\s+([a-zA-Z_][a-zA-Z0-9_]*)\\s*\\([^)]*\\)\\s*\\{?",
                REG_EXTENDED);

            regmatch_t matches[2];
            if (regexec(&func_regex, ptr, 2, matches, 0) == 0) {
                if (function_count < MAX_FUNCTIONS) {
                    current_func = function_count;
                    functions[current_func].start_line = i + 1;
                    strncpy(functions[current_func].name, 
                           ptr + matches[1].rm_so, 
                           matches[1].rm_eo - matches[1].rm_so);
                    functions[current_func].name[matches[1].rm_eo - matches[1].rm_so] = '\0';
                    brace_count = 0;
                    function_count++;
                }
            }
            regfree(&func_regex);
        }

        // Track function boundaries
        if (current_func != -1) {
            for (char *p = ptr; *p; p++) {
                if (*p == '{' && !in_comment_block) brace_count++;
                if (*p == '}' && !in_comment_block) brace_count--;
            }
            if (brace_count == 0 && current_func != -1) {
                functions[current_func].end_line = i + 1;
                current_func = -1;
            }
        }
    }
}

void generate_mutant(const Function *func, const MutationRule *rule, const char *output_dir) {
    for (int l = func->start_line - 1; l < func->end_line; l++) {
        if (is_comment_line[l]) continue;

        char *original = strdup(file_lines[l]);
        if (!original) continue;

        regmatch_t pmatch;
        bool mutated = false;
        int offset = 0;

        while (regexec(&rule->regex, original + offset, 1, &pmatch, 0) == 0) {
            char new_line[MAX_LINE_LEN] = {0};
            char fname[MAX_FILENAME_LEN];
            
            // Create mutated line
            strncpy(new_line, original, pmatch.rm_so + offset);
            strncat(new_line, rule->replacement, MAX_LINE_LEN - strlen(new_line) - 1);
            strncat(new_line, original + pmatch.rm_eo + offset, MAX_LINE_LEN - strlen(new_line) - 1);

            // Generate output filename
            snprintf(fname, sizeof(fname), "%s/%s_%s_%d.c",
                    output_dir, func->name, rule->suffix, l + 1);

            // Create mutant file
            FILE *out = fopen(fname, "w");
            if (out) {
                for (int i = 0; i < total_lines; i++) {
                    fputs((i == l) ? new_line : file_lines[i], out);
                }
                fclose(out);

                // Compilation check
                char cmd[MAX_FILENAME_LEN + 50];
                snprintf(cmd, sizeof(cmd), "gcc -c \"%s\" -o /dev/null 2>/dev/null", fname);
                if (system(cmd) != 0) {
                    remove(fname);
                } else {
                    mutated = true;
                }
            }

            offset += pmatch.rm_eo;
            if (mutated) break;
        }
        free(original);
    }
}

void generate_mutants(const char *output_dir) {
    for (int f = 0; f < function_count; f++) {
        for (int r = 0; rules[r].pattern != NULL; r++) {
            generate_mutant(&functions[f], &rules[r], output_dir);
        }
    }
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input.c> <output_dir>\n", argv[0]);
        return EXIT_FAILURE;
    }

    FILE *file = fopen(argv[1], "r");
    if (!file) {
        perror("File open failed");
        return EXIT_FAILURE;
    }

    compile_rules();
    parse_source_file(file);
    fclose(file);

    mkdir(argv[2], 0755);
    generate_mutants(argv[2]);
    free_resources();

    return EXIT_SUCCESS;
}