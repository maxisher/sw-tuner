#include <windows.h>
#include <iostream>
#include <vector>
#include <tlhelp32.h>
#include <string>

// Функция для поиска базового адреса модуля
uintptr_t GetModuleBaseAddress(DWORD procId, const char* modName) {
    uintptr_t modBaseAddr = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procId);
    if (hSnap != INVALID_HANDLE_VALUE) {
        MODULEENTRY32 modEntry;
        modEntry.dwSize = sizeof(modEntry);
        if (Module32First(hSnap, &modEntry)) {
            do {
                if (!_stricmp(modEntry.szModule, modName)) {
                    modBaseAddr = (uintptr_t)modEntry.modBaseAddr;
                    break;
                }
            } while (Module32Next(hSnap, &modEntry));
        }
    }
    CloseHandle(hSnap);
    return modBaseAddr;
}

// Функция сравнения байтов
bool CompareBytes(const unsigned char* data, const unsigned char* pattern, const char* mask) {
    for (; *mask; ++mask, ++data, ++pattern) {
        if (*mask == 'x' && *data != *pattern) return false;
    }
    return (*mask == 0);
}

// Поиск сигнатуры в памяти
uintptr_t FindPattern(HANDLE hProcess, uintptr_t start, DWORD size, const unsigned char* pattern, const char* mask) {
    std::vector<unsigned char> buffer(size);
    SIZE_T bytesRead;
    if (!ReadProcessMemory(hProcess, (LPVOID)start, buffer.data(), size, &bytesRead)) return 0;

    for (DWORD i = 0; i < bytesRead - strlen(mask); i++) {
        if (CompareBytes(&buffer[i], pattern, mask)) return start + i;
    }
    return 0;
}

int main() {
    SetConsoleTitleA("NetErrror - Ultimate SW Editor");
    std::cout << "Waiting for Stormworks..." << std::endl;

    HWND hwnd = NULL;
    while (!hwnd) {
        hwnd = FindWindowA(NULL, "Stormworks");
        Sleep(500);
    }

    DWORD procId;
    GetWindowThreadProcessId(hwnd, &procId);
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procId);
    uintptr_t baseAddr = GetModuleBaseAddress(procId, "stormworks64.exe");

    if (!hProcess || !baseAddr) {
        std::cout << "Error: Could not attach to process!" << std::endl;
        return 1;
    }

    // Сигнатура для поиска указателя на выбранный блок
    const unsigned char* pattern = (const unsigned char*)"\x48\x8B\x05\x00\x00\x00\x00\x48\x83\xC4\x20\x5B\xC3";
    const char* mask = "xxx????xxxxxx";
    
    std::cout << "Scanning for block pointer..." << std::endl;
    uintptr_t patternAddr = FindPattern(hProcess, baseAddr, 0x5000000, pattern, mask);

    if (!patternAddr) {
        std::cout << "Pattern not found! Make sure you are in the editor." << std::endl;
        return 1;
    }

    int32_t relativeAddr;
    ReadProcessMemory(hProcess, (LPVOID)(patternAddr + 3), &relativeAddr, sizeof(relativeAddr), NULL);
    uintptr_t selectedBlockPtr = patternAddr + 7 + relativeAddr;

    int currentIdx = 0;
    std::cout << "SUCCESS! Ready for manipulation." << std::endl;
    std::cout << "[Arrows Left/Right] - Select R-Matrix index (0-8)" << std::endl;
    std::cout << "[NUM + / NUM -] - Change value" << std::endl;

    while (true) {
        if (GetAsyncKeyState(VK_RIGHT) & 1) { currentIdx = (currentIdx + 1) % 9; std::cout << "Selected index: " << currentIdx << std::endl; }
        if (GetAsyncKeyState(VK_LEFT) & 1) { currentIdx = (currentIdx - 1 + 9) % 9; std::cout << "Selected index: " << currentIdx << std::endl; }

        uintptr_t activeBlock;
        ReadProcessMemory(hProcess, (LPVOID)selectedBlockPtr, &activeBlock, sizeof(activeBlock), NULL);

        if (activeBlock != 0) {
            uintptr_t rOffset = activeBlock + 0x50 + (currentIdx * 4);
            float val;
            ReadProcessMemory(hProcess, (LPVOID)rOffset, &val, sizeof(float), NULL);

            if (GetAsyncKeyState(VK_ADD) & 1) {
                val += 1.0f;
                WriteProcessMemory(hProcess, (LPVOID)rOffset, &val, sizeof(float), NULL);
                std::cout << "R[" << currentIdx << "] -> " << val << std::endl;
            }
            if (GetAsyncKeyState(VK_SUBTRACT) & 1) {
                val -= 1.0f;
                WriteProcessMemory(hProcess, (LPVOID)rOffset, &val, sizeof(float), NULL);
                std::cout << "R[" << currentIdx << "] -> " << val << std::endl;
            }
        }
        if (GetAsyncKeyState(VK_ESCAPE)) break;
        Sleep(10);
    }

    CloseHandle(hProcess);
    return 0;
}
