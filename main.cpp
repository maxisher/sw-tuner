#include <windows.h>
#include <iostream>
#include <vector>
#include <tlhelp32.h>

#pragma comment(lib, "user32.lib")

int main() {
    SetConsoleTitleA("NetErrror - Universal SW Scanner");
    std::cout << "=== NetErrror UNIVERSAL SCANNER MODE ===" << std::endl;

    HWND hwnd = FindWindowA(NULL, "Stormworks");
    if (!hwnd) { std::cout << "[!] Run Stormworks first!" << std::endl; system("pause"); return 1; }

    DWORD procId;
    GetWindowThreadProcessId(hwnd, &procId);
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procId);

    // Вместо паттерна мы будем искать адрес вручную через небольшую хитрость
    std::cout << "[*] Направьте камеру на блок в редакторе и нажмите F2..." << std::endl;
    
    while (!(GetAsyncKeyState(VK_F2) & 1)) { Sleep(10); }

    // Сканируем память на наличие стандартной матрицы [1, 0, 0, 0, 1, 0, 0, 0, 1]
    // В Stormworks блоки часто выравниваются по определенным адресам.
    std::cout << "[*] Поиск структуры блока в памяти (может занять 10-20 сек)..." << std::endl;

    uintptr_t foundAddr = 0;
    std::vector<unsigned char> buffer(1024 * 1024); // Буфер 1Мб
    SIZE_T bytesRead;

    // Сканируем только регион, где обычно лежат данные объектов (Heap)
    for (uintptr_t i = 0x200000000; i < 0x500000000; i += buffer.size()) {
        if (ReadProcessMemory(hProcess, (LPVOID)i, buffer.data(), buffer.size(), &bytesRead)) {
            for (size_t j = 0; j < bytesRead - 36; j += 4) {
                float* vals = (float*)&buffer[j];
                // Проверяем паттерн матрицы 1,0,0,0,1,0,0,0,1 (float)
                if (vals[0] == 1.0f && vals[1] == 0.0f && vals[2] == 0.0f &&
                    vals[4] == 1.0f && vals[8] == 1.0f) {
                    foundAddr = i + j;
                    break;
                }
            }
        }
        if (foundAddr) break;
    }

    if (!foundAddr) {
        std::cout << "[!] Структура не найдена автоматически. Попробуем другой регион..." << std::endl;
        system("pause"); return 1;
    }

    std::cout << "[SUCCESS] Блок найден по адресу: " << std::hex << foundAddr << std::endl;
    std::cout << "Управление: [Arrows] - Индекс | [Num +/-] - Значение" << std::endl;

    int currentIdx = 0;
    while (true) {
        if (GetAsyncKeyState(VK_RIGHT) & 1) { currentIdx = (currentIdx + 1) % 9; std::cout << "Index: " << currentIdx << std::endl; }
        if (GetAsyncKeyState(VK_LEFT) & 1) { currentIdx = (currentIdx - 1 + 9) % 9; std::cout << "Index: " << currentIdx << std::endl; }

        float val;
        uintptr_t target = foundAddr + (currentIdx * 4);
        ReadProcessMemory(hProcess, (LPVOID)target, &val, sizeof(float), NULL);

        if (GetAsyncKeyState(VK_ADD) & 1) {
            val += 1.0f;
            WriteProcessMemory(hProcess, (LPVOID)target, &val, sizeof(float), NULL);
            std::cout << "R[" << currentIdx << "] = " << val << std::endl;
        }
        if (GetAsyncKeyState(VK_SUBTRACT) & 1) {
            val -= 1.0f;
            WriteProcessMemory(hProcess, (LPVOID)target, &val, sizeof(float), NULL);
            std::cout << "R[" << currentIdx << "] = " << val << std::endl;
        }
        if (GetAsyncKeyState(VK_ESCAPE)) break;
        Sleep(10);
    }

    CloseHandle(hProcess);
    return 0;
}
