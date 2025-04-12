#include "framework.h"

#include "Debugutil.h"

// ����� ��¿� ��ƿ �Լ�
void DebugPrintA(const char* format, ...) {
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsprintf_s(buffer, format, args);
    va_end(args);
    OutputDebugStringA(buffer);
}

void DebugPrintW(const wchar_t* format, ...) {
    wchar_t buffer[512];
    va_list args;
    va_start(args, format);
    vswprintf_s(buffer, 512, format, args);
    va_end(args);
    OutputDebugStringW(buffer);
}
