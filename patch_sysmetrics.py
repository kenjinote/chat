import re

def patch():
    with open('Source.cpp', 'r', encoding='utf-8') as f:
        content = f.read()

    # Replace GetSystemMetrics(X) with GetSystemMetricsForDpi(X, g_dpi)
    # Be careful not to replace already patched lines
    content = re.sub(r'GetSystemMetrics\((SM_[A-Z_]+)\)', r'GetSystemMetricsForDpi(\1, g_dpi)', content)

    with open('Source.cpp', 'w', encoding='utf-8') as f:
        f.write(content)
    print("Patched GetSystemMetrics")

if __name__ == '__main__':
    patch()
