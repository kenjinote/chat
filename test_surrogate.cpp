#include <iostream>
#include <string>

bool is_surrogate(wchar_t c) {
    return (c >= 0xD800 && c <= 0xDFFF);
}

int main() {
    std::wstring text = L"abc😭def";
    for(int i=0; i<text.length(); ++i) {
        std::wcout << text[i] << " is_surrogate: " << is_surrogate(text[i]) << std::endl;
    }
    return 0;
}
