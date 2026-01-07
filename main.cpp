#include <windows.h>
#include <iostream>
#include <vector>
#include <tlhelp32.h>
#include <string>

#pragma comment(lib, "user32.lib")

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

bool CompareBytes(const unsigned char* data, const unsigned char* pattern, const char* mask) {
    for (; *mask; ++mask, ++data, ++pattern) {
        if (*mask == 'x' && *data != *pattern) return false;
    }
    return (*mask == 0);
}

uintptr_t FindPattern(HANDLE hProcess, uintptr_t start, DWORD size, const unsigned char* pattern, const char* mask) {
    std::vector<unsigned char> buffer(size);
    SIZE_T bytesRead;
    if (!ReadProcessMemory(hProcess, (LPVOID)start, buffer.data(), size, &bytesRead)) return 0;
    size_t maskLen = strlen(mask);
    for (DWORD i = 0; i < (DWORD)(bytesRead - maskLen); i++) {
        if (CompareBytes(&buffer[i], pattern, mask)) return start + i;
    }
    return 0;
}

int main() {
    std::cout << "=== NetErrror SW Tuner DEBUG MODE ===" << std::endl;

    HWND hwnd = FindWindowA(NULL, "Stormworks");
    if (!hwnd) {
        std::cout << "[!] ERROR: Stormworks window not found! Start the game first." << std::endl;
        system("pause");
        return 1;
    }

    DWORD procId;
    GetWindowThreadProcessId(hwnd, &procId);
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procId);
    if (!hProcess) {
        std::cout << "[!] ERROR: Access Denied! Run this program AS ADMINISTRATOR." << std::endl;
        system("pause");
        return 1;
    }

    uintptr_t baseAddr = GetModuleBaseAddress(procId, "stormworks64.exe");
    if (!baseAddr) {
        std::cout << "[!] ERROR: Could not find stormworks64.exe module." << std::endl;
        system("pause");
        return 1;
    }

    const unsigned char* pattern = (const unsigned char*)"\x48\x8B\x05\x00\x00\x00\x00\x48\x83\xC4\x20\x5B\xC3";
    const char* mask = "xxx????xxxxxx";
    
    std::cout << "[*] Scanning memory for block pointers..." << std::endl;
    uintptr_t patternAddr = FindPattern(hProcess, baseAddr, 0x5000000, pattern, mask);
    
    if (!patternAddr) {
        std::cout << "[!] ERROR: Pattern not found. The game might have updated." << std::endl;
        system("pause");
        return 1;
    }

    int32_t relativeAddr;
    ReadProcessMemory(hProcess, (LPVOID)(patternAddr + 3), &relativeAddr, sizeof(relativeAddr), NULL);
    uintptr_t selectedBlockPtr = patternAddr + 7 + relativeAddr;

    std::cout << "[SUCCESS] Script is running!" << std::endl;
    std::cout << "Select a block in the editor and use ARROWS + Num+/-" << std::endl;

    int currentIdx = 0;
    while (true) {
        if (GetAsyncKeyState(VK_RIGHT) & 1) { currentIdx = (currentIdx + 1) % 9; std::cout << "Matrix Index: " << currentIdx << std::endl; }
        if (GetAsyncKeyState(VK_LEFT) & 1) { currentIdx = (currentIdx - 1 + 9) % 9; std::cout << "Matrix Index: " << currentIdx << std::endl; }

        uintptr_t activeBlock;
        if (ReadProcessMemory(hProcess, (LPVOID)selectedBlockPtr, &activeBlock, sizeof(activeBlock), NULL) && activeBlock != 0) {
            uintptr_t rOffset = activeBlock + 0x50 + (currentIdx * 4);
            float val;
            ReadProcessMemory(hProcess, (LPVOID)rOffset, &val, sizeof(float), NULL);

            if (GetAsyncKeyState(VK_ADD) & 1) {
                val += 1.0f;
                WriteProcessMemory(hProcess, (LPVOID)rOffset, &val, sizeof(float), NULL);
                std::cout << "Updated R[" << currentIdx << "] to " << val << std::endl;
            }
            if (GetAsyncKeyState(VK_SUBTRACT) & 1) {
                val -= 1.0f;
                WriteProcessMemory(hProcess, (LPVOID)rOffset, &val, sizeof(float), NULL);
                std::cout << "Updated R[" << currentIdx << "] to " << val << std::endl;
            }
        }
        if (GetAsyncKeyState(VK_ESCAPE)) break;
        Sleep(10);
    }

    CloseHandle(hProcess);
    return 0;
}
