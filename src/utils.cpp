#include "utils.h"

void panic(LPCTSTR msg) {
    MessageBox(nullptr, msg, TEXT("engine panic"), 0);
    ExitProcess(99);
}
