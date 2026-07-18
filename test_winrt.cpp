#include <windows.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Graphics.Capture.h>
#pragma comment(lib, "windowsapp")

int main() {
    winrt::init_apartment();
    return 0;
}
