#include <windows.h>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

struct Snippet {
    std::wstring displayName;
    std::wstring actualText;
};

std::vector<Snippet> g_snippets;
HWND g_hStatus = NULL;
HWND g_hEdit = NULL;
HWND g_hReload = NULL;
HWND g_hTop = NULL;
HWND g_hScrollArea = NULL;
HFONT g_hFont = NULL;
bool g_isTopmost = true;
int g_scrollY = 0;
int g_totalHeight = 0;

std::wstring Utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, NULL, 0);
    std::wstring wide(size - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wide[0], size);
    return wide;
}

std::string WideToUtf8(const std::wstring& wide) {
    if (wide.empty()) return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, NULL, 0, NULL, NULL);
    std::string utf8(size - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, &utf8[0], size, NULL, NULL);
    return utf8;
}

std::wstring GetConfigPath() {
    wchar_t userProfile[MAX_PATH];
    if (GetEnvironmentVariableW(L"USERPROFILE", userProfile, MAX_PATH)) {
        std::wstring path = std::wstring(userProfile) + L"\\.vibecopy\\copy.txt";
        CreateDirectoryW((std::wstring(userProfile) + L"\\.vibecopy").c_str(), NULL);
        return path;
    }
    return L"copy.txt";
}

void LoadSnippets() {
    g_snippets.clear();
    std::wstring configPath = GetConfigPath();
    std::string narrowPath = WideToUtf8(configPath);

    std::ifstream file(narrowPath);
    if (!file.is_open()) {
        std::ofstream createFile(narrowPath);
        createFile << "继续\n";
        createFile << "（端对端测试）在虚拟环境中进行一次端对端的测试\n";
        createFile.close();
        file.open(narrowPath);
        if (!file.is_open()) return;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line.find_first_not_of(" \t\r\n") == std::string::npos) continue;

        std::string displayName, actualText;
        size_t eqPos = line.find("==");
        if (eqPos != std::string::npos) {
            displayName = line.substr(0, eqPos);
            actualText = line.substr(eqPos + 2);
        }
        else if (!line.empty() && (line[0] == '(' ||
                 (line.size() >= 3 && (unsigned char)line[0] == 0xEF &&
                  (unsigned char)line[1] == 0xBC && (unsigned char)line[2] == 0x88))) {
            size_t closeParen = std::string::npos;
            closeParen = line.find(')');
            if (closeParen == std::string::npos) {
                for (size_t i = 0; i + 2 < line.size(); i++) {
                    if ((unsigned char)line[i] == 0xEF &&
                        (unsigned char)line[i+1] == 0xBC &&
                        (unsigned char)line[i+2] == 0x89) {
                        closeParen = i;
                        break;
                    }
                }
            }
            if (closeParen != std::string::npos) {
                size_t openLen = (line[0] == '(') ? 1 : 3;
                displayName = line.substr(openLen, closeParen - openLen);
                size_t closeLen = (line[closeParen] == ')') ? 1 : 3;
                actualText = line.substr(closeParen + closeLen);
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

void UpdateScrollBar(HWND hwnd) {
    RECT rc;
    GetClientRect(g_hScrollArea, &rc);
    int visibleHeight = rc.bottom;

    SCROLLINFO si = {0};
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMin = 0;
    si.nMax = g_totalHeight;
    si.nPage = visibleHeight;
    si.nPos = g_scrollY;
    SetScrollInfo(g_hScrollArea, SB_VERT, &si, TRUE);
}

void CreateButtons(HWND hwnd) {
    // 清除滚动区域内的子窗口
    HWND child = GetWindow(g_hScrollArea, GW_CHILD);
    while (child) {
        HWND next = GetWindow(child, GW_HWNDNEXT);
        DestroyWindow(child);
        child = next;
    }

    RECT rc;
    GetClientRect(g_hScrollArea, &rc);
    int tableWidth = rc.right - 10;
    int btnWidth = (tableWidth - 5) / 2;
    int btnHeight = 50;
    int col = 0, row = 0;

    for (size_t i = 0; i < g_snippets.size(); i++) {
        int x = 5 + col * (btnWidth + 5);
        int y = 5 + row * (btnHeight + 5) - g_scrollY;

        HWND hBtn = CreateWindowW(L"BUTTON", g_snippets[i].displayName.c_str(),
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_MULTILINE,
            x, y, btnWidth, btnHeight,
            g_hScrollArea, (HMENU)(INT_PTR)(1000 + i), NULL, NULL);
        SendMessage(hBtn, WM_SETFONT, (WPARAM)g_hFont, TRUE);

        col++;
        if (col >= 2) { col = 0; row++; }
    }

    g_totalHeight = 5 + (row + (col > 0 ? 1 : 0)) * (btnHeight + 5);
    UpdateScrollBar(hwnd);
}

void CopyToClipboard(const std::wstring& text) {
    if (!OpenClipboard(NULL)) return;
    EmptyClipboard();
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

LRESULT CALLBACK ScrollAreaProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_VSCROLL: {
            RECT rc;
            GetClientRect(hwnd, &rc);
            int visibleHeight = rc.bottom;

            SCROLLINFO si = {0};
            si.cbSize = sizeof(SCROLLINFO);
            si.fMask = SIF_ALL;
            GetScrollInfo(hwnd, SB_VERT, &si);

            int oldPos = si.nPos;
            switch (LOWORD(wParam)) {
                case SB_LINEUP: si.nPos -= 30; break;
                case SB_LINEDOWN: si.nPos += 30; break;
                case SB_PAGEUP: si.nPos -= visibleHeight; break;
                case SB_PAGEDOWN: si.nPos += visibleHeight; break;
                case SB_THUMBTRACK: si.nPos = si.nTrackPos; break;
            }

            si.nPos = (std::max)(0, (std::min)(si.nPos, g_totalHeight - visibleHeight));
            if (si.nPos != oldPos) {
                g_scrollY = si.nPos;
                SetScrollPos(hwnd, SB_VERT, si.nPos, TRUE);
                // 移动所有子窗口
                int dy = oldPos - si.nPos;
                HWND child = GetWindow(hwnd, GW_CHILD);
                while (child) {
                    RECT btnRc;
                    GetWindowRect(child, &btnRc);
                    MapWindowPoints(GetParent(child), hwnd, (LPPOINT)&btnRc, 2);
                    MoveWindow(child, btnRc.left, btnRc.top + dy, btnRc.right - btnRc.left, btnRc.bottom - btnRc.top, TRUE);
                    child = GetWindow(child, GW_HWNDNEXT);
                }
            }
            return 0;
        }
        case WM_MOUSEWHEEL: {
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            SendMessage(hwnd, WM_VSCROLL, delta > 0 ? SB_LINEUP : SB_LINEDOWN, 0);
            SendMessage(hwnd, WM_VSCROLL, delta > 0 ? SB_LINEUP : SB_LINEDOWN, 0);
            return 0;
        }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            g_hFont = CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

            // 注册滚动区域窗口类
            WNDCLASSEXW wc = {0};
            wc.cbSize = sizeof(WNDCLASSEXW);
            wc.lpfnWndProc = ScrollAreaProc;
            wc.hInstance = GetModuleHandle(NULL);
            wc.hCursor = LoadCursor(NULL, IDC_ARROW);
            wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
            wc.lpszClassName = L"VibeCopyScrollArea";
            RegisterClassExW(&wc);

            // 创建滚动区域
            g_hScrollArea = CreateWindowExW(0, L"VibeCopyScrollArea", L"",
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_CLIPCHILDREN,
                10, 10, 780, 175, hwnd, NULL, NULL, NULL);

            // 底部按钮
            g_hEdit = CreateWindowW(L"BUTTON", L"编辑配置", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                10, 195, 250, 50, hwnd, (HMENU)1, NULL, NULL);
            g_hReload = CreateWindowW(L"BUTTON", L"刷新列表", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                275, 195, 250, 50, hwnd, (HMENU)2, NULL, NULL);
            g_hTop = CreateWindowW(L"BUTTON", L"取消置顶", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                540, 195, 250, 50, hwnd, (HMENU)3, NULL, NULL);

            // 状态栏
            g_hStatus = CreateWindowW(L"STATIC", L"准备就绪... 单击大色块即可复制",
                WS_CHILD | WS_VISIBLE | SS_LEFT, 10, 255, 780, 35, hwnd, NULL, NULL, NULL);

            SendMessage(g_hEdit, WM_SETFONT, (WPARAM)g_hFont, TRUE);
            SendMessage(g_hReload, WM_SETFONT, (WPARAM)g_hFont, TRUE);
            SendMessage(g_hTop, WM_SETFONT, (WPARAM)g_hFont, TRUE);
            SendMessage(g_hStatus, WM_SETFONT, (WPARAM)g_hFont, TRUE);

            LoadSnippets();
            CreateButtons(hwnd);
            break;
        }

        case WM_COMMAND: {
            int id = LOWORD(wParam);
            if (id == 1) {
                std::wstring path = GetConfigPath();
                ShellExecuteW(NULL, L"open", L"notepad.exe", path.c_str(), NULL, SW_SHOW);
            }
            else if (id == 2) {
                g_scrollY = 0;
                LoadSnippets();
                CreateButtons(hwnd);
            }
            else if (id == 3) {
                g_isTopmost = !g_isTopmost;
                SetWindowPos(hwnd, g_isTopmost ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                SetWindowTextW(g_hTop, g_isTopmost ? L"取消置顶" : L"窗口置顶");
            }
            else if (id >= 1000) {
                int index = id - 1000;
                if (index >= 0 && index < (int)g_snippets.size()) {
                    CopyToClipboard(g_snippets[index].actualText);
                    std::wstring status = L"已复制: " + g_snippets[index].actualText.substr(0, 60);
                    SetWindowTextW(g_hStatus, status.c_str());
                }
            }
            break;
        }

        case WM_SIZE: {
            RECT rc;
            GetClientRect(hwnd, &rc);
            int scrollHeight = rc.bottom - 110;

            MoveWindow(g_hScrollArea, 10, 10, rc.right - 20, scrollHeight, TRUE);

            int bottomY = rc.bottom - 90;
            MoveWindow(g_hEdit, 10, bottomY, 250, 50, TRUE);
            MoveWindow(g_hReload, 275, bottomY, 250, 50, TRUE);
            MoveWindow(g_hTop, 540, bottomY, 250, 50, TRUE);
            MoveWindow(g_hStatus, 10, rc.bottom - 35, rc.right - 20, 35, TRUE);

            g_scrollY = 0;
            CreateButtons(hwnd);
            break;
        }

        case WM_DESTROY:
            if (g_hFont) DeleteObject(g_hFont);
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow) {
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"VibeCopyClass";
    RegisterClassExW(&wc);

    HWND hwnd = CreateWindowExW(WS_EX_TOPMOST, L"VibeCopyClass", L"VibeCopy 鼠标速复",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 300,
        NULL, NULL, hInstance, NULL);

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
