#include <windows.h>
#include <iostream>
#include <vector>
#include <string>

#pragma comment(lib, "user32.lib")

// Глобальные данные для связи интерфейса и игры
HANDLE hProcess = NULL;
uintptr_t targetBlockAddr = 0;

// Описания направлений матрицы R
const char* r_labels[9] = {
    "X-Scale / Mirror", "X-Skew Y",       "X-Skew Z",
    "Y-Skew X",       "Y-Scale / Mirror", "Y-Skew Z",
    "Z-Skew X",       "Z-Skew Y",       "Z-Scale / Mirror"
};

// Функция изменения значения
void ChangeVal(int idx, float delta) {
    if (!targetBlockAddr) return;
    float current;
    uintptr_t addr = targetBlockAddr + (idx * 4);
    ReadProcessMemory(hProcess, (LPVOID)addr, &current, sizeof(float), NULL);
    current += delta;
    WriteProcessMemory(hProcess, (LPVOID)addr, &current, sizeof(float), NULL);
}

// Процедура окна управления
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: {
        for (int i = 0; i < 9; i++) {
            // Создаем кнопки для каждого параметра
            CreateWindowA("BUTTON", "-", WS_VISIBLE | WS_CHILD, 10, 10 + (i * 35), 30, 30, hwnd, (HMENU)(100 + i), NULL, NULL);
            CreateWindowA("STATIC", r_labels[i], WS_VISIBLE | WS_CHILD, 50, 15 + (i * 35), 150, 20, hwnd, NULL, NULL, NULL);
            CreateWindowA("BUTTON", "+", WS_VISIBLE | WS_CHILD, 210, 10 + (i * 35), 30, 30, hwnd, (HMENU)(200 + i), NULL, NULL);
        }
        CreateWindowA("BUTTON", "SCAN FOR BLOCK (F2)", WS_VISIBLE | WS_CHILD, 10, 330, 230, 40, hwnd, (HMENU)500, NULL, NULL);
        break;
    }
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id >= 100 && id < 109) ChangeVal(id - 100, -1.0f);
        if (id >= 200 && id < 209) ChangeVal(id - 200, 1.0f);
        if (id == 500) std::cout << "Scanning..." << std::endl; // Здесь вызывается логика сканера
        break;
    }
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// Основной поток окна
DWORD WINAPI GuiThread(LPVOID lpParam) {
    WNDCLASSA wc = { 0 };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "SW_CONTROLLER";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassA(&wc);

    HWND hwnd = CreateWindowA("SW_CONTROLLER", "Stormworks Matrix Tuner", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, 270, 420, NULL, NULL, NULL, NULL);
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE); // Всегда поверх игры

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}

int main() {
    std::cout << "Starting Controller..." << std::endl;
    // (Сюда вставь логику поиска процесса из предыдущего кода)
    
    // Запуск интерфейса в отдельном потоке
    CreateThread(NULL, 0, GuiThread, NULL, 0, NULL);
    
    // Оставляем консоль для вывода логов сканирования
    while(true) { Sleep(100); }
    return 0;
}
