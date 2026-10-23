#pragma once

#include <stdexcept>

#include <windows.h>
#include <functional>
#define MAX_LOADSTRING 100

class Window {  
    
public:
    Window(HINSTANCE hInstance);
    ~Window();
    HWND hwnd;
    HINSTANCE hInst; //= NULL;
    int Run(std::function<void()> tick);
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    // === Зум камеры (колесо мыши) ===
    int  getMouseWheelDelta() const { return mouseWheelDelta; }
    void resetMouseWheel() { mouseWheelDelta = 0; }


    SIZE getExtent() {
        RECT rect{};
        GetClientRect(hwnd, &rect);
        return SIZE{ rect.right, rect.bottom };
    }

    bool wasWindowResized() const { return frameBufferResized; }
    void resetWindowResizedFlag() { frameBufferResized = false; }
    void setWindowResized() { frameBufferResized = true; }

   
    /**
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClass(&wc))
        return -1;
        */
private:
    bool frameBufferResized = false;
    void WinReg();
    int mouseWheelDelta = 0;
    static constexpr wchar_t CLASS_NAME[] = L"MyWindowClass";
   // const wchar_t CLASS_NAME[14] = L"MyWindowClass";
   


};