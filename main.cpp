#include <windows.h>
#include <iostream>
#include <vector>
#include <map>
#include <string>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

HANDLE hProc = NULL;
uintptr_t targetAddr = 0;
std::map<uintptr_t, float> memorySnapshot;

// Функция изменения значений матрицы
void ModifyValue(int idx, float delta) {
    if (!targetAddr || !hProc) return;
    float current;
    uintptr_t finalAddr = targetAddr + (idx * 4);
    if (ReadProcessMemory(hProc, (LPVOID)finalAddr, &current, sizeof(float), NULL)) {
        current += delta;
        WriteProcessMemory(hProc, (LPVOID)finalAddr, &current, sizeof(float), NULL);
        std::cout << "Index [" << idx << "] set to: " << current << std::endl;
    }
}

// Поиск адреса через изменение (вращение блока)
void CaptureActiveBlock() {
    memorySnapshot.clear();
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    uintptr_t addr = 0x100000000; 

    std::cout << "[*] Step 1: Taking snapshot. PLEASE WAIT..." << std::endl;

    while (addr < 0x600000000) {
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQueryEx(hProc, (LPCVOID)addr, &mbi, sizeof(mbi))) {
            if (mbi.State == MEM_COMMIT && mbi.Protect == PAGE_READWRITE && mbi.RegionSize < 0x5000000) {
                std::vector<float> buffer(mbi.RegionSize / sizeof(float));
                if (ReadProcessMemory(hProc, mbi.BaseAddress, buffer.data(), mbi.RegionSize, NULL)) {
                    for (size_t i = 0; i < buffer.size(); i++) {
                        if (buffer[i] == 1.0f || buffer[i] == -1.0f || buffer[i] == 0.0f) {
                            memorySnapshot[(uintptr_t)mbi.BaseAddress + (i * 4)] = buffer[i];
                        }
                    }
                }
            }
            addr += mbi.RegionSize;
        } else addr += 0x1000;
    }

    std::cout << "[!] Snapshot done. ROTATE THE BLOCK in game and PRESS F3!" << std::endl;
    while (!(GetAsyncKeyState(VK_F3) & 1)) Sleep(10);

    for (auto const& [ptr, oldVal] : memorySnapshot) {
        float newVal;
        if (ReadProcessMemory(hProc, (LPVOID)ptr, &newVal, sizeof(float), NULL)) {
            if (newVal != oldVal && (newVal == 1.0f || newVal == -1.0f || newVal == 0.0f)) {
                targetAddr = ptr - (ptr % 36); 
                std::cout << "[SUCCESS] Found block matrix at: 0x" << std::hex << targetAddr << std::dec << std::endl;
                return;
            }
        }
    }
    std::cout << "[FAIL] No rotation detected." << std::endl;
}

LRESULT CALLBACK WinProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    const char* labels[9] = {"X-Scale", "X-Skew Y", "X-Skew Z", "Y-Skew X", "Y-Scale", "Y-Skew Z", "Z-Skew X", "Z-Skew Y", "Z-Scale"};
    switch (msg) {
    case WM_CREATE:
        for (int i = 0; i < 9; i++) {
            CreateWindowA("BUTTON", "-", WS_VISIBLE | WS_CHILD, 10, 10 + (i * 35), 30, 30, hwnd, (HMENU)(100 + i), NULL, NULL);
            CreateWindowA("STATIC", labels[i], WS_VISIBLE | WS_CHILD, 50, 15 + (i * 35), 130, 20, hwnd, NULL, NULL, NULL);
            CreateWindowA("BUTTON", "+", WS_VISIBLE | WS_CHILD, 190, 10 + (i * 35), 30, 30, hwnd, (HMENU)(200 + i), NULL, NULL);
        }
        CreateWindowA("BUTTON", "CAPTURE BLOCK (F2)", WS_VISIBLE | WS_CHILD, 10, 330, 210, 40, hwnd, (HMENU)500, NULL, NULL);
        break;
    case WM_COMMAND: {
        int id = LOWORD(wp);
        if (id >= 100 && id < 109) ModifyValue(id - 100, -1.0f);
        if (id >= 200 && id < 209) ModifyValue(id - 200, 1.0f);
        if (id == 500) CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CaptureActiveBlock, NULL, 0, NULL);
        break;
    }
    case WM_DESTROY: PostQuitMessage(0); break;
    default: return DefWindowProc(hwnd, msg, wp, lp);
    }
    return 0;
}

int main() {
    HWND gHwnd = FindWindowA(NULL, "Stormworks");
    if (!gHwnd) { 
        std::cout << "Error: Stormworks not found!" << std::endl; 
        Sleep(3000); return 1; 
    }
    DWORD pId; 
    GetWindowThreadProcessId(gHwnd, &pId);
    hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pId);

    WNDCLASSA wc = {0};
    wc.lpfnWndProc = WinProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "SW_Tuner_Class";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassA(&wc);
    
    CreateWindowA("SW_Tuner_Class", "SW Matrix Editor", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, 250, 420, NULL, NULL, NULL, NULL);
    std::cout << "Tuner Interface Ready. Start CAPTURE." << std::endl;

    MSG m;
    while (GetMessage(&m, NULL, 0, 0)) { TranslateMessage(&m); DispatchMessage(&m); }
    return 0;
}
