#include <windows.h>
#include <string>
#include <iostream>

int main() {
    std::wstring pUser = L"123e4567-e89b-12d3-a456-426614174000";
    std::string targetA;
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, pUser.c_str(), -1, NULL, 0, NULL, NULL);
    std::cout << "size_needed: " << size_needed << std::endl;
    if (size_needed > 0) {
        targetA.resize(size_needed - 1);
        int res = WideCharToMultiByte(CP_UTF8, 0, pUser.c_str(), -1, &targetA[0], size_needed - 1, NULL, NULL);
        std::cout << "res: " << res << std::endl;
        std::cout << "GetLastError: " << GetLastError() << std::endl;
    }
    return 0;
}
