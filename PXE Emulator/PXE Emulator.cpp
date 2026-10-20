// PXE Emulator.cpp : Defines the entry point for the application.
// PXE Emulator rebuild in C++ inspired by @tova98 (https://pxe.nefal.world/)
//

#include "framework.h"
#include "PXE Emulator.h"
#include "pxe_engine.h"
#include <commdlg.h>
#include <fstream>
#include <iostream>
#include <vector>

#ifndef NOMINMAX
#define NOMINMAX
#endif

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

std::unique_ptr<PXE::PXEEngine> g_pxeEngine;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

static std::vector<uint8_t> ReadEntireFile(const char* filepath) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return {};
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    if (size <= 0) return {};
    std::vector<uint8_t> buffer(static_cast<size_t>(size));
    if (file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        return buffer;
    }
    return {};
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Initialize Global PXE Engine
    g_pxeEngine = std::make_unique<PXE::PXEEngine>();

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_PXEEMULATOR, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_PXEEMULATOR));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (g_pxeEngine && g_pxeEngine->isLoaded && !g_pxeEngine->isHalted()) {
            g_pxeEngine->step(5000);
        }
    }

    return (int) msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_PXEEMULATOR));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_PXEEMULATOR);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   // Screen size 240x400
   RECT rc = { 0, 0, PXE::SCREEN_WIDTH * 2, PXE::SCREEN_HEIGHT * 2 };
   AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, TRUE);

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            case IDM_OPENPXOFILE:
                {
                    char filename[MAX_PATH] = { 0 };
                    OPENFILENAMEA ofn = { 0 };
                    ofn.lStructSize = sizeof(ofn);
                    ofn.hwndOwner = hWnd;
                    ofn.lpstrFilter = "PXO Game Files (*.pxo)\0*.pxo\0All Files (*.*)\0*.*\0";
                    ofn.lpstrFile = filename;
                    ofn.nMaxFile = MAX_PATH;
                    ofn.lpstrTitle = "Select LG Music World .pxo Game File";
                    ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;

                    if (GetOpenFileNameA(&ofn))
                    {
                        std::vector<uint8_t> bytes = ReadEntireFile(filename);
                        if (g_pxeEngine && g_pxeEngine->loadPXOFile(bytes)) {
                            MessageBoxA(hWnd, "Successfully loaded PXO file!", "PXE Emulator", MB_OK | MB_ICONINFORMATION);
                            InvalidateRect(hWnd, nullptr, TRUE);
                        } else {
                            MessageBoxA(hWnd, "Failed to load PXO file.", "PXE Emulator Error", MB_OK | MB_ICONERROR);
                        }
                    }
                }
                break;
            case IDM_OPENFLSFILE:
                {
                    char filename[MAX_PATH] = { 0 };
                    OPENFILENAMEA ofn = { 0 };
                    ofn.lStructSize = sizeof(ofn);
                    ofn.hwndOwner = hWnd;
                    ofn.lpstrFilter = "FLS Firmware / Kernel Files (*.fls)\0*.fls\0All Files (*.*)\0*.*\0";
                    ofn.lpstrFile = filename;
                    ofn.nMaxFile = MAX_PATH;
                    ofn.lpstrTitle = "Select FLS Kernel or Executable File";
                    ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;

                    if (GetOpenFileNameA(&ofn))
                    {
                        std::vector<uint8_t> bytes = ReadEntireFile(filename);
                        if (g_pxeEngine && g_pxeEngine->loadFLSFile(bytes)) {
                            MessageBoxA(hWnd, "Successfully loaded FLS Kernel/Executable!", "PXE Emulator", MB_OK | MB_ICONINFORMATION);
                            InvalidateRect(hWnd, nullptr, TRUE);
                        } else {
                            MessageBoxA(hWnd, "Failed to load FLS file.", "PXE Emulator Error", MB_OK | MB_ICONERROR);
                        }
                    }
                }
                break;
            }
        }
        break;
    case WM_LBUTTONDOWN:
    case WM_MOUSEMOVE:
    case WM_LBUTTONUP:
        if (g_pxeEngine) {
            int x = LOWORD(lParam) / 2;
            int y = HIWORD(lParam) / 2;
            int action = (message == WM_LBUTTONDOWN) ? 0 : ((message == WM_MOUSEMOVE) ? 1 : 2);
            g_pxeEngine->sendTouchEvent(x, y, action);
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            RECT rect;
            GetClientRect(hWnd, &rect);

            // Draw PXE Engine frame buffer canvas
            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP hBitmap = CreateCompatibleBitmap(hdc, PXE::SCREEN_WIDTH, PXE::SCREEN_HEIGHT);
            HBITMAP hOldBmp = (HBITMAP)SelectObject(memDC, hBitmap);

            if (g_pxeEngine) {
                BITMAPINFO bmi = { 0 };
                bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                bmi.bmiHeader.biWidth = PXE::SCREEN_WIDTH;
                bmi.bmiHeader.biHeight = -PXE::SCREEN_HEIGHT; // Top-down
                bmi.bmiHeader.biPlanes = 1;
                bmi.bmiHeader.biBitCount = 32;
                bmi.bmiHeader.biCompression = BI_RGB;

                SetDIBits(memDC, hBitmap, 0, PXE::SCREEN_HEIGHT, g_pxeEngine->frameBuffer.data(), &bmi, DIB_RGB_COLORS);
            }

            StretchBlt(hdc, 0, 0, rect.right, rect.bottom, memDC, 0, 0, PXE::SCREEN_WIDTH, PXE::SCREEN_HEIGHT, SRCCOPY);

            SelectObject(memDC, hOldBmp);
            DeleteObject(hBitmap);
            DeleteDC(memDC);

            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box giving credit to @tova98
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        SetDlgItemTextA(hDlg, IDC_STATIC, "PXE Emulator C++ Edition\nBased on PXE Emulator by @tova98 (https://pxe.nefal.world/)\nSupports .pxo files and .fls kernel loader.");
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
