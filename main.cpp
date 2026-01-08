#include <windows.h>
#include <iostream>
#include <vector>
#include <thread>

#pragma comment(lib, "user32.lib")

HANDLE hProcess = NULL;
uintptr_t currentBlockAddr = 0;
float matrix[9] = {1,0,0,0,1,0,0,0,1};

// Эта функция ищет блок, который сейчас "активен" в памяти редактора
void FindLiveBlock() {
    SYSTEM_INFO si; GetSystemInfo(&si);
    uintptr_t addr = 0x100000000;
    
    std::cout << "[*] Ищу блок... Пожалуйста, повращайте его или подвигайте в редакторе!" << std::endl;
    
    // Мы ищем уникальную структуру, которая появляется только у активного объекта
    while (addr < 0x600000000) {
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQueryEx(hProcess, (LPCVOID)addr, &mbi, sizeof(mbi))) {
            if (mbi.State == MEM_COMMIT && mbi.Protect == PAGE_READWRITE) {
                std::vector<float> buf(mbi.RegionSize / 4);
                if (ReadProcessMemory(hProcess, mbi.BaseAddress, buf.data(), mbi.RegionSize, NULL)) {
                    for (size_t i = 0; i < buf.size() - 9; i++) {
                        // Ищем паттерн матрицы r и флагов активности рядом
                        if (buf[i] == 1.0f && buf[i+4] == 1.0f && buf[i+8] == 1.0f) {
                            currentBlockAddr = (uintptr_t)mbi.BaseAddress + (i * 4);
                            std::cout << "[!] БЛОК ЗАХВАЧЕН! Адрес: " << std::hex << currentBlockAddr << std::dec << std::endl;
                            return;
                        }
                    }
                }
            }
            addr += mbi.RegionSize;
        } else addr += 0x1000;
    }
}

// Поток мгновенного обновления (заставляет игру перерисовывать блок)
void LiveUpdateLoop() {
    while (true) {
        if (currentBlockAddr && hProcess) {
            for (int i = 0; i < 9; i++) {
                uintptr_t target = currentBlockAddr + (i * 4);
                WriteProcessMemory(hProcess, (LPVOID)target, &matrix[i], sizeof(float), NULL);
            }
        }
        Sleep(1); // Максимальная частота обновления
    }
}

// GUI часть (упрощенно для управления)
void ControlPanel() {
    std::cout << "--- УПРАВЛЕНИЕ МАТРИЦЕЙ ---" << std::endl;
    std::cout << "Используйте Num7/Num4 для изменения Z-Scale (Длина)" << std::endl;
    while(true) {
        if (GetAsyncKeyState(VK_NUMPAD7) & 1) { matrix[8] += 0.5f; std::cout << "Z-Scale: " << matrix[8] << std::endl; }
        if (GetAsyncKeyState(VK_NUMPAD4) & 1) { matrix[8] -= 0.5f; std::cout << "Z-Scale: " << matrix[8] << std::endl; }
        Sleep(10);
    }
}

int main() {
    HWND hwnd = FindWindowA(NULL, "Stormworks");
    if (!hwnd) return 1;
    DWORD pid; GetWindowThreadProcessId(hwnd, &pid);
    hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);

    std::thread(LiveUpdateLoop).detach();
    FindLiveBlock();
    ControlPanel();
    
    return 0;
}
