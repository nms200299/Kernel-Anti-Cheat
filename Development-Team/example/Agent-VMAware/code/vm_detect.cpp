#include "vm_detect.h"
#include "../include/vmaware_MIT.hpp"  // include 폴더에 위치한 경우

extern "C" {

    VMDETECT_API bool IsVMwarePresent() {
        return VM::detect();
    }

    VMDETECT_API const char* GetVMBrand() {
        static std::string brand = VM::brand();
        return brand.c_str();
    }

    VMDETECT_API const char* GetVMType() {
        static std::string type = VM::type();
        return type.c_str();
    }

    VMDETECT_API int GetVMCertainty() {
        return static_cast<int>(VM::percentage());
    }

}
