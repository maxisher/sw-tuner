#include <windows.h>
#include <iostream>
#include <vector>
#include <thread>

#pragma comment(lib, "user32.lib")

HANDLE hProcess = NULL;
uintptr_t currentBlockAddr = 0;
float matrix[9] = { 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f };

void FindLiveBlock() {
    SYSTEM_INFO si; GetSystemInfo(&si);
    uintptr_t addr = 0x100000000;
    
    std::cout << "[*] Searching... Rotate the block (J, K, L) NOW!" << std::endl;
    
    while (currentBlockAddr == 0) {
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQueryEx(hProcess, (LPCVOID)addr, &mbi, sizeof(mbi))) {
            if (mbi.State == MEM_COMMIT && mbi.Protect == PAGE_READWRITE) {
                std::vector<float> buf(mbi.RegionSize / 4);
                if (ReadProcessMemory(hProcess, mbi.BaseAddress, buf.data(), mbi.RegionSize, NULL)) {
                    for (size_t i = 0; i < buf.size() - 9; i++) {
                        // Ищем активную матрицу r в памяти редактора
                        if (buf[i] == 1.0f && buf[i+4] == 1.0f && buf[i+8] == 1.0f) {
                            currentBlockAddr = (uintptr_t)mbi.BaseAddress + (i * 4);
                            std::cout << "[!] BLOCK CAPTURED at: 0x" << std::hex << currentBlockAddr << std::dec << std::endl;
                            return;
                        }
                    }
                }
            }
            addr += mbi.RegionSize;
        } else addr += 0x1000;
        if (addr > 0x600000000) addr = 0x100000000; // Цикличный поиск
        Sleep(100);
    }
}

void LiveUpdateLoop() {
    while (true) {
        if (currentBlockAddr && hProcess) {
            for (int i = 0; i < 9; i++) {
                uintptr_t target = currentBlockAddr + (i * 4);
                WriteProcessMemory(hProcess, (LPVOID)target, &matrix[i], sizeof(float), NULL);
            }
        }
        Sleep(1);
    }
}

int main() {
    // Установка кодировки для нормального текста
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);

    HWND hwnd = FindWindowA(NULL, "Stormworks");
    if (!hwnd) { std::cout << "Error: Stormworks not found!" << std::endl; return 1; }
    
    DWORD pid; GetWindowThreadProcessId(hwnd, &pid);
    hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);

    std::thread(LiveUpdateLoop).detach();
    
    FindLiveBlock();

    std::cout << "--- CONTROLS ---" << std::endl;
    std::cout << "Num 7/4: Z-Scale (Length)" << std::endl;
    std::cout << "Num 8/5: X-Scale (Width)" << std::endl;
    std::cout << "Num 9/6: Y-Scale (Height)" << std::endl;

    while(true) {
        if (GetAsyncKeyState(VK_NUMPAD7) & 1) { matrix[8] += 0.5f; std::cout << "Z-Scale: " << matrix[8] << std::endl; }
        if (GetAsyncKeyState(VK_NUMPAD4) & 1) { matrix[8] -= 0.5f; std::cout << "Z-Scale: " << matrix[8] << std::endl; }
        
        if (GetAsyncKeyState(VK_NUMPAD8) & 1) { matrix[0] += 0.5f; std::cout << "X-Scale: " << matrix[0] << std::endl; }
        if (GetAsyncKeyState(VK_NUMPAD5) & 1) { matrix[0] -= 0.5f; std::cout << "X-Scale: " << matrix[0] << std::endl; }

        if (GetAsyncKeyState(VK_NUMPAD9) & 1) { matrix[4] += 0.5f; std::cout << "Y-Scale: " << matrix[4] << std::endl; }
        if (GetAsyncKeyState(VK_NUMPAD6) & 1) { matrix[4] -= 0.5f; std::cout << "Y-Scale: " << matrix[4] << std::endl; }
        
        Sleep(10);
    }
    return 0;
}
