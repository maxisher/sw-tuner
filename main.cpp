#include <windows.h>
#include <iostream>
#include <vector>
#include <string>

#pragma comment(lib, "user32.lib")

// Глобальные переменные
HANDLE hProc = NULL;
uintptr_t targetAddr = 0;
bool isScanning = false;

const char* labels[9] = {
    "X Scale/Mirror", "X-Skew Y", "X-Skew Z",
    "Y-Skew X", "Y Scale/Mirror", "Y-Skew Z",
    "Z-Skew X", "Z-Skew Y", "Z Scale/Mirror"
};

// Функция записи в память
void ModifyValue(int idx, float delta) {
    if (!targetAddr || !hProc) return;
    float current;
    uintptr_t finalAddr = targetAddr + (idx * 4);
    if (ReadProcessMemory(hProc, (LPVOID)finalAddr, &current, sizeof(float), NULL)) {
        current += delta;
        WriteProcessMemory(hProc, (LPVOID)finalAddr, &current, sizeof(float), NULL);
        std::cout << "Changed index " << idx << " to " << current << std::endl;
    }
}

// Функция быстрого сканирования
void RunScanner() {
    if (isScanning) return;
    isScanning = true;
    targetAddr = 0;

    SYSTEM_INFO si;
    GetSystemInfo(&si);
    uintptr_t addr = (uintptr_t)si.lpMinimumApplicationAddress;

    std::cout << "[*] Scanning... Please wait." << std::endl;

    while (addr < 0x7FFFFFFFFFFF) { // Ограничение диапазона для скорости
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQueryEx(hProc, (LPCVOID)addr, &mbi, sizeof(mbi))) {
            // Ищем только в записываемой памяти (куче)
            if (mbi.State == MEM_COMMIT && mbi.Protect == PAGE_READWRITE && mbi.RegionSize < 0x10000000) {
                std::vector<float> buffer(mbi.RegionSize / sizeof(float));
                SIZE_T bytesRead;
                if (ReadProcessMemory(hProc, mbi.BaseAddress, buffer.data(), mbi.RegionSize, &bytesRead)) {
                    for (size_t i = 0; i < (bytesRead / 4) - 9; i++) {
                        // Ищем паттерн стандартной матрицы 1,0,0,0,1,0,0,0,1
                        if (buffer[i] == 1.0f && buffer[i+1] == 0.0f && buffer[i+2] == 0.0f &&
                            buffer[i+4] == 1.0f && buffer[i+8] == 1.0f) {
                            targetAddr = (uintptr_t)mbi.BaseAddress + (i * sizeof(float));
                            std::cout << "[!] FOUND BLOCK AT: 0x" << std::hex << targetAddr << std::dec << std::endl;
                            isScanning = false;
                            return;
                        }
                    }
                }
            }
            addr += mbi.RegionSize;
        } else { addr += 0x1000; }
        if (targetAddr) break;
    }
    if (!targetAddr) std::cout << "[?] Not found. Move the block and try again." << std::endl;
    isScanning = false;
}

// Интерфейс
LRESULT CALLBACK WinProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE:
        for (int i = 0; i < 9; i++) {
            CreateWindowA("BUTTON", "-", WS_VISIBLE | WS_CHILD, 10, 10 + (i * 35), 30, 30, hwnd, (HMENU)(100 + i), NULL, NULL);
            CreateWindowA("STATIC", labels[i], WS_VISIBLE | WS_CHILD, 50, 15 + (i * 35), 130, 20, hwnd, NULL, NULL, NULL);
            CreateWindowA("BUTTON", "+", WS_VISIBLE | WS_CHILD, 190, 10 + (i * 35), 30, 30, hwnd, (HMENU)(200 + i), NULL, NULL);
        }
        CreateWindowA("BUTTON", "SCAN BLOCK (F2)", WS_VISIBLE | WS_CHILD, 10, 330, 210, 40, hwnd, (HMENU)500, NULL, NULL);
        break;
    case WM_COMMAND: {
        int id = LOWORD(wp);
        if (id >= 100 && id < 109) ModifyValue(id - 100, -1.0f);
        if (id >= 200 && id < 209) ModifyValue(id - 200, 1.0f);
        if (id == 500) CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)RunScanner, NULL, 0, NULL);
        break;
    }
    case WM_DESTROY: PostQuitMessage(0); break;
    default: return DefWindowProc(hwnd, msg, wp, lp);
    }
    return 0;
}

int main() {
    HWND gameHwnd = FindWindowA(NULL, "Stormworks");
    if (!gameHwnd) { std::cout << "Start Stormworks first!" << std::endl; system("pause"); return 1; }
    DWORD pId; GetWindowThreadProcessId(gameHwnd, &pId);
    hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pId);

    WNDCLASSA wc = {0};
    wc.lpfnWndProc = WinProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "SW_Class";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassA(&wc);
    
    std::cout << "Controller started. Use the GUI window." << std::endl;
    CreateWindowA("SW_Class", "SW Matrix Editor", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, 250, 420, NULL, NULL, NULL, NULL);

    MSG m;
    while (GetMessage(&m, NULL, 0, 0)) { TranslateMessage(&m); DispatchMessage(&m); }
    return 0;
}
