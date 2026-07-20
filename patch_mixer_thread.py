import re

def patch():
    with open('Source.cpp', 'r', encoding='utf-8') as f:
        content = f.read()

    # Start audio mixer thread in WM_CREATE
    old_init = """        InitAudio();

        g_oldListProc = (WNDPROC)SetWindowLongPtr(g_hUserList, GWLP_WNDPROC, (LONG_PTR)ListBoxSubclassProc);"""
    
    new_init = """        InitAudio();
        g_audioMixerRunning = true;
        g_audioMixerThread = std::thread(AudioMixerLoop);

        g_oldListProc = (WNDPROC)SetWindowLongPtr(g_hUserList, GWLP_WNDPROC, (LONG_PTR)ListBoxSubclassProc);"""
    content = content.replace(old_init, new_init)

    # Clean up thread in WM_DESTROY
    old_destroy = """    case WM_DESTROY: {
        RevokeDragDrop(hWnd);"""
    
    new_destroy = """    case WM_DESTROY: {
        g_audioMixerRunning = false;
        if (g_hAudioEvent) SetEvent(g_hAudioEvent);
        if (g_audioMixerThread.joinable()) g_audioMixerThread.join();
        
        RevokeDragDrop(hWnd);"""
    content = content.replace(old_destroy, new_destroy)

    with open('Source.cpp', 'w', encoding='utf-8') as f:
        f.write(content)
    print("Patched thread start/stop")

if __name__ == '__main__':
    patch()
