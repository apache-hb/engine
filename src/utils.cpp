#include "utils.h"

#include <stdio.h>

void panic(LPCTSTR msg) {
    // MessageBox(nullptr, msg, TEXT("engine panic"), 0);
    fprintf(stderr, "panic: %ws\n", msg);
    ExitProcess(99);
}
