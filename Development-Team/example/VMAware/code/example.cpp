#include <iostream>
#include "../vm_detect/vm_detect.h"  // vm_detect.h 경로에 맞게 조정

#pragma comment(lib, "../Release/vm_detect.lib")  // lib 경로에 맞게 조정

int main() {
    if (IsVMwarePresent()) {
        std::cout << "[+] VMware 환경이 감지되었습니다!\n";
        std::cout << "브랜드: " << GetVMBrand() << "\n";
        std::cout << "유형: " << GetVMType() << "\n";
        std::cout << "확신도: " << GetVMCertainty() << "%\n";
    }
    else {
        std::cout << "[-] 실제 머신 환경입니다.\n";
    }

    return 0;
}
