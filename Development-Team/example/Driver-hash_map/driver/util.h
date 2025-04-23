#pragma once

_gData gData;

#define DBG_LEVEL_DEBUG     0x1
#define DBG_LEVEL_NOTI      0x2
#define DBG_LEVEL_WARN      0x3
#define DBG_LEVEL_ERROR     0x4

#define DBG_PRINT(PrintDbgLevel, function, format, ...) \
    do { \
        if (gData.SetDbgLevel <= PrintDbgLevel) { \
            DbgPrint("[AntiCheat][%s] ", function); \
            DbgPrint(format, __VA_ARGS__); \
        } \
    } while(0)

// ������ DBG_LEVEL���� ���� �޽����� ���