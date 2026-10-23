#include "Window.h"
//#include "framework.h"
//#define VK_USE_PLATFORM_WIN32_KHR

//constexpr wchar_t Window::CLASS_NAME[];

 LRESULT CALLBACK  Window::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

    // Извлекаем указатель на наш объект Window
         Window* wnd = reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));



         switch (msg)
         {

         case WM_MOUSEWHEEL:
             if (wnd) {
                 wnd->mouseWheelDelta += GET_WHEEL_DELTA_WPARAM(wParam);
             }
             return 0;

         case WM_SIZE:
             if (wnd) {
                 // Взводим флаг изменения размера для любого изменения (разворачивание, растягивание)
                 wnd->setWindowResized();
             }
             return 0;

         case WM_DESTROY:
             PostQuitMessage(0);
             return 0;
         }
       
     return DefWindowProc(hwnd, msg, wParam, lParam);

}

 Window::Window(HINSTANCE  hInstance) : hInst(hInstance) {

     WinReg();
     hwnd = CreateWindowEx(
         0,
         CLASS_NAME,
         L"Title",
         WS_OVERLAPPEDWINDOW,
         CW_USEDEFAULT, CW_USEDEFAULT, 900, 500,
         NULL, NULL, hInstance, NULL);
     if (!hwnd) throw std::runtime_error("CreateWindowEx failed");
     // ОБЯЗАТЕЛЬНО: Привязываем указатель на текущий экземпляр класса Window к хэндлу HWND
     SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
     ShowWindow(hwnd, SW_SHOW);
 }

 Window::~Window() {

 }


void Window::WinReg() {
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClass(&wc))
        throw std::runtime_error("RegisterClass failed");

   
}

int Window::Run(std::function<void()> tick) {
    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        // Проверяем сообщения без блокировки потока
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            // Сообщений нет — рендерим кадр Vulkan!
            tick();
        }
    }
    return (int)msg.wParam;
}