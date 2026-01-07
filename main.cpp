#include <iostream>
#include <windows.h>
#include <vector>

// ... (функции AttachToGame и FindPattern из предыдущих ответов остаются теми же) ...

int main() {
    SetConsoleTitle("NetErrror - Matrix R-Param Editor");
    // [Подключение к игре аналогично предыдущим кодам]

    int currentIndex = 0; // Индекс в матрице от 0 до 8
    float matrix[9] = {1, 0, 0, 0, 1, 0, 0, 0, 1}; // Стандартная матрица

    std::cout << "[+] Режим редактирования матрицы R активирован!" << std::endl;
    std::cout << "[Arrows Left/Right] - Выбор параметра (r11 - r33)" << std::endl;
    std::cout << "[Num + / Num -]     - Изменение значения (-1, 0, 1)" << std::endl;

    while (true) {
        // Выбор индекса в матрице r="0,1,2,3,4,5,6,7,8"
        if (GetAsyncKeyState(VK_RIGHT) & 1) { 
            currentIndex = (currentIndex + 1) % 9;
            std::cout << "[*] Выбран индекс R: " << currentIndex << std::endl;
        }
        if (GetAsyncKeyState(VK_LEFT) & 1) { 
            currentIndex = (currentIndex - 1 + 9) % 9;
            std::cout << "[*] Выбран индекс R: " << currentIndex << std::endl;
        }

        uintptr_t activeBlock = FindSelectedBlock(); // Находит адрес по сигнатуре

        if (activeBlock != 0) {
            // Офсеты матрицы вращения обычно начинаются после координат
            // В Stormworks матрица R в памяти часто идет блоком по 4 байта (float)
            uintptr_t rOffset = activeBlock + 0x50 + (currentIndex * 4);
            
            float currentR;
            ReadProcessMemory(hProcess, (LPVOID)rOffset, &currentR, sizeof(float), NULL);

            if (GetAsyncKeyState(VK_ADD) & 1) {
                currentR += 1.0f; // В матрице вращения значения обычно целые (-1, 0, 1)
                WriteProcessMemory(hProcess, (LPVOID)rOffset, &currentR, sizeof(float), NULL);
                std::cout << "[!] r[" << currentIndex << "] изменен на: " << currentR << std::endl;
            }
            if (GetAsyncKeyState(VK_SUBTRACT) & 1) {
                currentR -= 1.0f;
                WriteProcessMemory(hProcess, (LPVOID)rOffset, &currentR, sizeof(float), NULL);
                std::cout << "[!] r[" << currentIndex << "] изменен на: " << currentR << std::endl;
            }
        }
        if (GetAsyncKeyState(VK_ESCAPE)) break;
        Sleep(10);
    }
    return 0;
}
