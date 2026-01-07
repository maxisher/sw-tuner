#include <windows.h>
#include <iostream>
#include <vector>

#pragma comment(lib, "user32.lib")

void ScanMemory() {
    HWND hwnd = FindWindowA(NULL, "Stormworks");
    if (!hwnd) { std::cout << "Error: Run Stormworks!" << std::endl; return; }

    DWORD procId;
    GetWindowThreadProcessId(hwnd, &procId);
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procId);

    std::cout << "Target: Stormworks (PID: " << procId << ")" << std::endl;
    std::cout << "Action: Point at a block and press F2..." << std::endl;
    while (!(GetAsyncKeyState(VK_F2) & 1)) Sleep(10);

    SYSTEM_INFO si;
    GetSystemInfo(&si);
    uintptr_t addr = (uintptr_t)si.lpMinimumApplicationAddress;

    std::cout << "Scanning ALL memory. This may take a minute..." << std::endl;

    while (addr < (uintptr_t)si.lpMaximumApplicationAddress) {
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQueryEx(hProcess, (LPCVOID)addr, &mbi, sizeof(mbi))) {
            if (mbi.State == MEM_COMMIT && (mbi.Protect & PAGE_READWRITE)) {
                std::vector<float> buffer(mbi.RegionSize / sizeof(float));
                if (ReadProcessMemory(hProcess, mbi.BaseAddress, buffer.data(), mbi.RegionSize, NULL)) {
                    for (size_t i = 0; i < buffer.size() - 9; i++) {
                        // Ищем паттерн матрицы: 1, 0, 0, 0, 1, 0, 0, 0, 1
                        if (buffer[i] == 1.0f && buffer[i+1] == 0.0f && buffer[i+2] == 0.0f &&
                            buffer[i+3] == 0.0f && buffer[i+4] == 1.0f && buffer[i+5] == 0.0f &&
                            buffer[i+8] == 1.0f) {
                            
                            uintptr_t foundAddr = (uintptr_t)mbi.BaseAddress + (i * sizeof(float));
                            std::cout << "FOUND structure at: 0x" << std::hex << foundAddr << std::endl;
                            std::cout << "Use Arrows (L/R) to select index, Num+/- to change." << std::endl;

                            int currentIdx = 0;
                            while (true) {
                                if (GetAsyncKeyState(VK_RIGHT) & 1) { currentIdx = (currentIdx + 1) % 9; std::cout << "Index: " << std::dec << currentIdx << std::endl; }
                                if (GetAsyncKeyState(VK_LEFT) & 1) { currentIdx = (currentIdx - 1 + 9) % 9; std::cout << "Index: " << std::dec << currentIdx << std::endl; }

                                float val;
                                uintptr_t target = foundAddr + (currentIdx * 4);
                                ReadProcessMemory(hProcess, (LPVOID)target, &val, sizeof(float), NULL);

                                if (GetAsyncKeyState(VK_ADD) & 1) {
                                    val += 1.0f;
                                    WriteProcessMemory(hProcess, (LPVOID)target, &val, sizeof(float), NULL);
                                    std::cout << "r[" << currentIdx << "] = " << val << std::endl;
                                }
                                if (GetAsyncKeyState(VK_SUBTRACT) & 1) {
                                    val -= 1.0f;
                                    WriteProcessMemory(hProcess, (LPVOID)target, &val, sizeof(float), NULL);
                                    std::cout << "r[" << currentIdx << "] = " << val << std::endl;
                                }
                                if (GetAsyncKeyState(VK_ESCAPE)) return;
                                Sleep(10);
                            }
                        }
                    }
                }
            }
            addr += mbi.RegionSize;
        } else {
            addr += 0x1000;
        }
    }
    std::cout << "Not found. Try moving the block or picking it up again." << std::endl;
}

int main() {
    ScanMemory();
    system("pause");
    return 0;
}
