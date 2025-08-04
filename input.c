#include <stdio.h>

struct Point { int x; int y; };

void func1() {
    struct Point p = {.x = 1, .y = 2};
    struct Point *ptr = &p;
    int a = ptr->x;  // Precedence 1: ->
    int b = p.y;     // Precedence 1: .
}

int func2(int x, int y) {
    int result = x * y + (x - y);  // * (3), + (4), - (4)
    if (result == 0 || result < 100) {  // == (7), < (6)
        result = result << 2;  // << (5)
    }
    return result ? result : -1;  // ?: (13)
}

int main() {
    func1();
    int r = func2(5, 3);
    printf("Result: %d\n", r);
    return 0;
}