#include <windows.h>
#include <stdio.h>

int main() {
    printf("SM_CYCAPTION: %d\n", GetSystemMetricsForDpi(SM_CYCAPTION, 96));
    printf("SM_CYSIZE: %d\n", GetSystemMetricsForDpi(SM_CYSIZE, 96));
    printf("SM_CXSIZE: %d\n", GetSystemMetricsForDpi(SM_CXSIZE, 96));
    
    printf("SM_CYCAPTION(192): %d\n", GetSystemMetricsForDpi(SM_CYCAPTION, 192));
    printf("SM_CYSIZE(192): %d\n", GetSystemMetricsForDpi(SM_CYSIZE, 192));
    printf("SM_CXSIZE(192): %d\n", GetSystemMetricsForDpi(SM_CXSIZE, 192));
    return 0;
}
