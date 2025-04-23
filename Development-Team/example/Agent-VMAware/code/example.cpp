#include <iostream>
#include "../vm_detect/vm_detect.h"  // vm_detect.h ��ο� �°� ����

#pragma comment(lib, "../Release/vm_detect.lib")  // lib ��ο� �°� ����

int main() {
    if (IsVMwarePresent()) {
        std::cout << "[+] VMware ȯ���� �����Ǿ����ϴ�!\n";
        std::cout << "�귣��: " << GetVMBrand() << "\n";
        std::cout << "����: " << GetVMType() << "\n";
        std::cout << "Ȯ�ŵ�: " << GetVMCertainty() << "%\n";
    }
    else {
        std::cout << "[-] ���� �ӽ� ȯ���Դϴ�.\n";
    }

    return 0;
}
