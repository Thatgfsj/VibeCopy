#include <windows.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

// 全局变量
struct Snippet {
    std::wstring displayName;
    std::wstring actualText;
};

std::vector<Snippet> g_snippets;
std::vector<HWND> g_buttons;
HWND g_hStatus = NULL;
const int BUTTON_HEIGHT = 55;
const int PADDING = 10;
const int STATUS_HEIGHT = 35;

// UTF-8 转 UTF-16
std::wstring Utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, NULL, 0);
    std::wstring wide(size - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wide[0], size);
    return wide;
}

// 获取配置文件路径
std::wstring GetConfigPath() {
    wchar_t userProfile[MAX_PATH];
    if (GetEnvironmentVariableW(L"USERPROFILE", userProfile, MAX_PATH)) {
        std::wstring path = std::wstring(userProfile) + L"\\.vibecopy\\copy.txt";
        CreateDirectoryW((std::wstring(userProfile) + L"\\.vibecopy").c_str(), NULL);
        return path;
    }
    return L"copy.txt";
}

// 读取配置文件
void LoadSnippets() {
    g_snippets.clear();
    std::wstring configPath = GetConfigPath();

    // 转换为窄字符串用于文件操作
    int len = WideCharToMultiByte(CP_UTF8, 0, configPath.c_str(), -1, NULL, 0, NULL, NULL);
    std::string narrowPath(len - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, configPath.c_str(), -1, &narrowPath[0], len, NULL, NULL);

    std::ifstream file(narrowPath);
    if (!file.is_open()) {
        std::ofstream createFile(narrowPath);
        createFile.close();
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line.find_first_not_of(" \t\r\n") == std::string::npos) {
            continue;
        }

        std::string displayName, actualText;

        size_t eqPos = line.find("==");
        if (eqPos != std::string::npos) {
            displayName = line.substr(0, eqPos);
            actualText = line.substr(eqPos + 2);
        }
        else if (!line.empty() && (line[0] == '(' || (unsigned char)line[0] == 0xEF)) {
            size_t closeParen = line.find(')');
            if (closeParen == std::string::npos) {
                // 查找中文右括号 ）的 UTF-8 编码
                closeParen = line.find('\xEF\xBC\x89');
                if (closeParen != std::string::npos) {
                    closeParen += 2; // 跳过 3 字节中的前 2 字节
                }
            }
            if (closeParen != std::string::npos) {
                displayName = line.substr(1, closeParen - 1);
                actualText = line.substr(closeParen + 1);
            } else {
                displayName = line;
                actualText = line;
            }
        }
        else {
            displayName = line;
            actualText = line;
        }

        auto trim = [](std::string& s) {
            s.erase(0, s.find_first_not_of(" \t\r\n"));
            s.erase(s.find_last_not_of(" \t\r\n") + 1);
        };
        trim(displayName);
        trim(actualText);

        if (!displayName.empty()) {
            g_snippets.push_back({Utf8ToWide(displayName), Utf8ToWide(actualText)});
        }
    }
    file.close();
}

// 复制到剪贴板
void CopyToClipboard(const std::wstring& text) {
    if (!OpenClipboard(NULL)) return;
    EmptyClipboard();

    // 复制 Unicode 文本
    size_t len = (text.size() + 1) * sizeof(wchar_t);
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
    if (hMem) {
        wchar_t* pMem = (wchar_t*)GlobalLock(hMem);
        memcpy(pMem, text.c_str(), len);
        GlobalUnlock(hMem);
        SetClipboardData(CF_UNICODETEXT, hMem);
    }
    CloseClipboard();
}

// 更新状态栏
void UpdateStatus(const std::wstring& text) {
    if (g_hStatus) {
        std::wstring display = L"已复制: " + text.substr(0, 60);
        SetWindowTextW(g_hStatus, display.c_str());
    }
}

// 创建按钮
void CreateButtons(HWND hwnd) {
    for (HWND hBtn : g_buttons) {
        DestroyWindow(hBtn);
    }
    g_buttons.clear();

    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    int tableWidth = clientRect.right - 2 * PADDING;
    int btnWidth = (tableWidth - PADDING) / 2;
    int col = 0, row = 0;

    for (size_t i = 0; i < g_snippets.size(); i++) {
        int x = PADDING + col * (btnWidth + PADDING);
        int y = PADDING + row * (BUTTON_HEIGHT + PADDING);

        HWND hBtn = CreateWindowW(
            L"BUTTON", g_snippets[i].displayName.c_str(),
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_MULTILINE,
            x, y, btnWidth, BUTTON_HEIGHT,
            hwnd, (HMENU)(INT_PTR)(1000 + i), NULL, NULL
        );

        HFONT hFont = CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        SendMessage(hBtn, WM_SETFONT, (WPARAM)hFont, TRUE);

        g_buttons.push_back(hBtn);

        col++;
        if (col >= 2) {
            col = 0;
            row++;
        }
    }
}

// 窗口过程
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            g_hStatus = CreateWindowW(L"STATIC", L"准备就绪... 单击大色块即可复制",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                PADDING, 0, 0, STATUS_HEIGHT,
                hwnd, NULL, NULL, NULL);

            HFONT hFont = CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
            SendMessage(g_hStatus, WM_SETFONT, (WPARAM)hFont, TRUE);

            LoadSnippets();
            CreateButtons(hwnd);
            break;
        }

        case WM_SIZE: {
            RECT clientRect;
            GetClientRect(hwnd, &clientRect);
            MoveWindow(g_hStatus, PADDING, clientRect.bottom - STATUS_HEIGHT - PADDING,
                clientRect.right - 2 * PADDING, STATUS_HEIGHT, TRUE);
            CreateButtons(hwnd);
            break;
        }

        case WM_COMMAND: {
            int id = LOWORD(wParam);
            if (id >= 1000 && id < 1000 + (int)g_snippets.size()) {
                int index = id - 1000;
                CopyToClipboard(g_snippets[index].actualText);
                UpdateStatus(g_snippets[index].actualText);
            }
            break;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// 主函数
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"VibeCopyClass";
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    RegisterClassExW(&wc);

    HWND hwnd = CreateWindowExW(
        WS_EX_TOPMOST,
        L"VibeCopyClass", L"VibeCopy 鼠标速复",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 300,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) return 1;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}
