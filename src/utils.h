#pragma once

#include <windows.h>

#define STR_INNER(x) #x
#define STR(x) STR_INNER(x)

#define ASSERT(expr) if (!(expr)) { PANIC(STR(__FILE__) "." STR(__LINE__) ": " #expr); }
#define PANIC(expr) panic(TEXT(expr))

void panic(LPCTSTR msg);
