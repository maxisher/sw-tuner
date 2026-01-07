#include <windows.h>
#include <iostream>
#include <vector>
#include <map>

#pragma comment(lib, "user32.lib")

HANDLE hProc = NULL;
uintptr_t targetAddr = 0;

// Функция для захвата адреса через изменение
void FindActiveAddress() {
    std::cout << "[*] ВРАЩАЙТЕ БЛОК В ИГРЕ (J, K, L)..." << std::endl;
    
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    
    // Делаем снимок памяти
    std::map<uintptr_t, float> snapshot;
    uintptr_t addr = 0x200000000; // Начало типичной кучи
    
    // Быстрый поиск потенциальных кандидатов
    for (int attempt = 0; attempt < 10; attempt++) {
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQueryEx(hProc, (LPCVOID)addr, &mbi, sizeof(mbi))) {
            if (mbi.State == MEM_COMMIT && mbi.Protect == PAGE_READWRITE) {
                std::vector<float> buffer(mbi.RegionSize / sizeof(float));
                if (ReadProcessMemory(hProc, mbi.BaseAddress, buffer.data(), mbi.RegionSize, NULL)) {
                    for (size_t i = 0; i < buffer.size(); i++) {
                        if (buffer[i] == 1.0f || buffer[i] == -1.0f || buffer[i] == 0.0f) {
                            snapshot[(uintptr_t)mbi.BaseAddress + (i * 4)] = buffer[i];
                        }
                    }
                }
            }
            addr += mbi.RegionSize;
        }
        if (addr > 0x500000000) break;
    }

    std::cout << "[*] Снимок сделан. Теперь поверните блок и нажмите F3..." << std::endl;
    while (!(GetAsyncKeyState(VK_F3) & 1)) Sleep(10);

    // Сверяем изменения
    for (auto const& [ptr, oldVal] : snapshot) {
        float newVal;
        ReadProcessMemory(hProc, (LPVOID)ptr, &newVal, sizeof(float), NULL);
        if (newVal != oldVal && (newVal == 1.0f || newVal == -1.0f || newVal == 0.0f)) {
            targetAddr = ptr; // Нашли адрес, который изменился при повороте!
            std::cout << "[!] ЗАХВАЧЕН АКТИВНЫЙ АДРЕС: 0x" << std::hex << targetAddr << std::dec << std::endl;
            return;
        }
    }
    std::cout << "[?] Не удалось поймать изменение. Попробуйте еще раз." << std::endl;
}

// (Остальная часть кода с кнопками GUI из предыдущего сообщения остается такой же, 
// но в WM_COMMAND добавьте вызов FindActiveAddress() для новой кнопки)
