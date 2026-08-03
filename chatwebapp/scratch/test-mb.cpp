#include <windows.h>
#include <iostream>
#include <string>
#include <vector>

int main() {
    std::string targetA = "33083ccf-dd1a-3949-b7a0-9445a35bfacd";
    int wsize = MultiByteToWideChar(CP_UTF8, 0, targetA.c_str(), -1, NULL, 0);
    std::wstring target;
    target.resize(wsize - 1);
    int res = MultiByteToWideChar(CP_UTF8, 0, targetA.c_str(), -1, &target[0], wsize - 1);
    
    std::cout << "wsize: " << wsize << "\n";
    std::cout << "res: " << res << "\n";
    std::cout << "GetLastError: " << GetLastError() << "\n";
    return 0;
}
