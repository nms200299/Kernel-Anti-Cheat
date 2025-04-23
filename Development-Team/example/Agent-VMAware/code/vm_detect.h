#pragma once

#ifdef VMDETECT_EXPORTS
#define VMDETECT_API __declspec(dllexport)
#else
#define VMDETECT_API __declspec(dllimport)
#endif

extern "C" {
    VMDETECT_API bool IsVMwarePresent();
    VMDETECT_API const char* GetVMBrand();
    VMDETECT_API const char* GetVMType();
    VMDETECT_API int GetVMCertainty();
}
