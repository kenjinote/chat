#include <windows.h>
#include <wincrypt.h>
#include <string>
#include <vector>
#include <iostream>

LPSTR w2a(LPCWSTR lpszText) {
    const DWORD dwSize = WideCharToMultiByte(CP_UTF8, 0, lpszText, -1, 0, 0, 0, 0);
    LPSTR lpszReturnText = (char*)GlobalAlloc(GPTR, dwSize * sizeof(char));
    if (lpszReturnText) {
        WideCharToMultiByte(CP_UTF8, 0, lpszText, -1, lpszReturnText, dwSize, 0, 0);
    }
    return lpszReturnText;
}

std::string HashRoomName(const std::wstring& roomName) {
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    std::string result = """";
    
    if (CryptAcquireContext(&hProv, 0, 0, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        if (CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
            LPSTR utf8Str = w2a(roomName.c_str());
            if (utf8Str) {
                if (CryptHashData(hHash, (const BYTE*)utf8Str, (DWORD)strlen(utf8Str), 0)) {
                    DWORD hashLen = 0;
                    DWORD cbHashLen = sizeof(DWORD);
                    if (CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE*)&hashLen, &cbHashLen, 0)) {
                        std::vector<BYTE> buffer(hashLen);
                        if (CryptGetHashParam(hHash, HP_HASHVAL, buffer.data(), &hashLen, 0)) {
                            char hex[3];
                            for (DWORD i = 0; i < hashLen; i++) {
                                sprintf_s(hex, "%02x", buffer[i]);
                                result += hex;
                            }
                        }
                    }
                }
                GlobalFree(utf8Str);
            }
            CryptDestroyHash(hHash);
        }
        CryptReleaseContext(hProv, 0);
    }
    return result;
}

int main() {
    std::cout << HashRoomName(L"debug") << std::endl;
    return 0;
}
