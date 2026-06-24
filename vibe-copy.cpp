#include <windows.h>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

// 全局变量
std::vector<std::pair<std::string, std::string>> g_snippets;
std::vector<HWND> g_buttons;
HWND g_hStatus = NULL;
HWND g_hTable = NULL;
int g_buttonCount = 0;
const int BUTTON_HEIGHT = 55;
const int BUTTON_WIDTH = 380;
const int PADDING = 10;
const int STATUS_HEIGHT = 35;

// 配置文件路径
std::string GetConfigPath() {
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    std::string fullPath(path);
    size_t pos = fullPath.find_last_of("\\/");
    if (pos != std::string::npos) {
        return fullPath.substr(0, pos + 1) + "copy.txt";
    }
    return "copy.txt";
}

// 读取配置文件
void LoadSnippets() {
    g_snippets.clear();
    std::string configPath = GetConfigPath();
    std::ifstream file(configPath);
    if (!file.is_open()) {
        // 创建空配置文件
        std::ofstream createFile(configPath);
        createFile.close();
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line.find_first_not_of(" \t\r\n") == std::string::npos) {
            continue;
        }

        std::string displayName, actualText;

        // 检查 == 分隔符
        size_t eqPos = line.find("==");
        if (eqPos != std::string::npos) {
            displayName = line.substr(0, eqPos);
            actualText = line.substr(eqPos + 2);
        }
        // 检查 (显示名) 实际文本 格式
        else if (line[0] == '(' || line[0] == '\xEF\xBC\x88') { // ( or （
            size_t closeParen = line.find(')');
            if (closeParen == std::string::npos) {
                closeParen = line.find('\xEF\xBC\x89'); // ）
            }
            if (closeParen != std::string::npos) {
                displayName = line.substr(1, closeParen - 1);
                actualText = line.substr(closeParen + 1);
            } else {
                displayName = line;
                actualText = line;
            }
        }
        // 默认：显示名=实际文本
        else {
            displayName = line;
            actualText = line;
        }

        // 去除首尾空格
        auto trim = [](std::string& s) {
            s.erase(0, s.find_first_not_of(" \t\r\n"));
            s.erase(s.find_last_not_of(" \t\r\n") + 1);
        };
        trim(displayName);
        trim(actualText);

        if (!displayName.empty()) {
            g_snippets.push_back({displayName, actualText});
        }
    }
    file.close();
}

// 复制到剪贴板
void CopyToClipboard(const std::string& text) {
    if (!OpenClipboard(NULL)) return;
    EmptyClipboard();
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
    if (hMem) {
        char* pMem = (char*)GlobalLock(hMem);
        memcpy(pMem, text.c_str(), text.size() + 1);
        GlobalUnlock(hMem);
        SetClipboardData(CF_TEXT, hMem);
    }
    CloseClipboard();
}

// 更新状态栏
void UpdateStatus(const std::string& text) {
    if (g_hStatus) {
        std::string display = "已复制: " + text.substr(0, 60);
        SetWindowTextA(g_hStatus, display.c_str());
    }
}

// 创建按钮
void CreateButtons(HWND hwnd) {
    // 清除旧按钮
    for (HWND hBtn : g_buttons) {
        DestroyWindow(hBtn);
    }
    g_buttons.clear();
    g_buttonCount = 0;

    // 计算布局
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    int tableWidth = clientRect.right - 2 * PADDING;
    int tableHeight = clientRect.bottom - STATUS_HEIGHT - 3 * PADDING;
    int btnWidth = (tableWidth - PADDING) / 2;
    int col = 0, row = 0;

    for (size_t i = 0; i < g_snippets.size(); i++) {
        int x = PADDING + col * (btnWidth + PADDING);
        int y = PADDING + row * (BUTTON_HEIGHT + PADDING);

        HWND hBtn = CreateWindowA(
            "BUTTON", g_snippets[i].first.c_str(),
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_MULTILINE,
            x, y, btnWidth, BUTTON_HEIGHT,
            hwnd, (HMENU)(INT_PTR)(1000 + i), NULL, NULL
        );

        // 设置字体
        HFONT hFont = CreateFontA(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");
        SendMessage(hBtn, WM_SETFONT, (WPARAM)hFont, TRUE);

        g_buttons.push_back(hBtn);
        g_buttonCount++;

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
            // 创建状态栏
            g_hStatus = CreateWindowA("STATIC", "准备就绪... 单击大色块即可复制",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                PADDING, 0, 0, STATUS_HEIGHT,
                hwnd, NULL, NULL, NULL);

            HFONT hFont = CreateFontA(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");
            SendMessage(g_hStatus, WM_SETFONT, (WPARAM)hFont, TRUE);

            // 加载配置并创建按钮
            LoadSnippets();
            CreateButtons(hwnd);
            break;
        }

        case WM_SIZE: {
            RECT clientRect;
            GetClientRect(hwnd, &clientRect);

            // 更新状态栏位置
            MoveWindow(g_hStatus, PADDING, clientRect.bottom - STATUS_HEIGHT - PADDING,
                clientRect.right - 2 * PADDING, STATUS_HEIGHT, TRUE);

            // 重新创建按钮
            CreateButtons(hwnd);
            break;
        }

        case WM_COMMAND: {
            int id = LOWORD(wParam);
            if (id >= 1000 && id < 1000 + (int)g_snippets.size()) {
                int index = id - 1000;
                CopyToClipboard(g_snippets[index].second);
                UpdateStatus(g_snippets[index].second);
            }
            break;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// 主函数
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // 注册窗口类
    WNDCLASSEXA wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "VibeCopyClass";
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    RegisterClassExA(&wc);

    // 创建窗口
    HWND hwnd = CreateWindowExA(
        WS_EX_TOPMOST,
        "VibeCopyClass", "VibeCopy 鼠标速复",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 300,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) return 1;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // 消息循环
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}
