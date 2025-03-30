#include "framework.h"

#include <iostream>
#include <exception>
#include <string>
using namespace std;

// 디버그 출력용 유틸 함수
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

// 구조체 정의
typedef struct _FLT_TO_USER_WRAPPER {
    FILTER_MESSAGE_HEADER hdr;
    FLT_TO_USER data;
} FLT_TO_USER_WRAPPER, * PFLT_TO_USER_WRAPPER;

typedef struct _FLT_TO_USER_REPLY_WRAPPER {
    FILTER_REPLY_HEADER hdr;
    FLT_TO_USER_REPLY data;
} FLT_TO_USER_REPLY_WRAPPER, * PFLT_TO_USER_REPLY_WRAPPER;

int DriverPort() {
    HANDLE port_handle = nullptr;
    HRESULT h_result;

    USER_TO_FLT sent;        ZeroMemory(&sent, sizeof(sent));
    USER_TO_FLT_REPLY reply; ZeroMemory(&reply, sizeof(reply));
    FLT_TO_USER_WRAPPER recv;             ZeroMemory(&recv, sizeof(recv));
    FLT_TO_USER_REPLY_WRAPPER recv_reply; ZeroMemory(&recv_reply, sizeof(recv_reply));
    DWORD returned_bytes = 0;

    try {
        h_result = FilterConnectCommunicationPort(
            TEXT(MINIFLT_EXAMPLE_PORT_NAME),
            0,
            nullptr,
            0,
            nullptr,
            &port_handle
        );

        if (IS_ERROR(h_result)) {
            DebugPrintA("[Error] FilterConnectCommunicationPort failed (HRESULT = 0x%x)\n", h_result);
            throw runtime_error("FilterConnectCommunicationPort failed");
        }

        wcscpy_s(sent.msg, ARRAYSIZE(sent.msg), L"Hello From Agent");
        h_result = FilterSendMessage(
            port_handle,
            &sent,
            sizeof(sent),
            &reply,
            sizeof(reply),
            &returned_bytes
        );

        if (IS_ERROR(h_result)) {
            DebugPrintA("[Error] FilterSendMessage failed (HRESULT = 0x%x)\n", h_result);
            throw runtime_error("FilterSendMessage failed");
        }

        if (returned_bytes > 0) {
            DebugPrintW(L"[Reply] %s\n", reply.msg);
        }
        else {
            DebugPrintA("[Reply] No reply\n");
        }

        while (true) {
            h_result = FilterGetMessage(
                port_handle,
                &recv.hdr,
                sizeof(recv),
                nullptr
            );

            if (IS_ERROR(h_result)) {
                DebugPrintA("[Error] FilterGetMessage failed (HRESULT = 0x%x)\n", h_result);
                throw runtime_error("FilterGetMessage failed");
            }

            ZeroMemory(&recv_reply, sizeof(recv_reply));
            if (wcsstr(recv.data.path, L"test.txt")) {
                recv_reply.data.block = TRUE;
                DebugPrintW(L"[Filter] %s will be blocked\n", recv.data.path);
            }

            recv_reply.hdr.MessageId = recv.hdr.MessageId;

            h_result = FilterReplyMessage(
                port_handle,
                &recv_reply.hdr,
                sizeof(recv_reply.hdr) + sizeof(recv_reply.data)
            );

            if (IS_ERROR(h_result)) {
                DebugPrintA("[Error] FilterReplyMessage failed (HRESULT = 0x%x)\n", h_result);
                throw runtime_error("FilterReplyMessage failed");
            }
        }
    }
    catch (const exception& e) {
        DebugPrintA("[Exception] %s\n", e.what());
    }

    if (port_handle) {
        FilterClose(port_handle);
    }

    return 0;
}

DWORD WINAPI DriverPortThread(LPVOID lpParam) {
    DriverPort();
    return 0;
}
