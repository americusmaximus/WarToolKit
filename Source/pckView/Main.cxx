/*
Copyright (c) 2024 Americus Maximus

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "App.hxx"
#include "BitMap.hxx"
#include "Image.hxx"
#include "DirectDraw.hxx"
#include "Resources.hxx"

#define MAX_LOAD_STRING_LENGTH          100
#define MAX_WINDOW_DETAIL_TITLE_LENGTH  256
#define MAX_ERROR_MESSAGE_LENGTH        512

HINSTANCE hInst;
TCHAR szWindowTitle[MAX_LOAD_STRING_LENGTH];
TCHAR szWindowClass[MAX_LOAD_STRING_LENGTH];

IMAGECONTAINERPTR image;

TCHAR szWindowDetailTitle[MAX_WINDOW_DETAIL_TITLE_LENGTH];

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
    case WM_INITDIALOG: { return (INT_PTR)TRUE; }
    case WM_COMMAND:
    {
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));

            return (INT_PTR)TRUE;
        }

        break;
    }
    }

    return (INT_PTR)FALSE;
}

VOID UpdateTitle(HWND hWnd)
{
    if (image == NULL) { SetWindowText(hWnd, szWindowTitle); }
    else
    {
        wsprintf(szWindowDetailTitle, _T("pckView (%s) [%d/%d]"), image->Name, image->Index + 1, image->Frames);
        SetWindowText(hWnd, szWindowDetailTitle);
    }
}

BOOL OpenImage(HWND hWnd, IMAGECONTAINERPTR img)
{
    TCHAR message[MAX_ERROR_MESSAGE_LENGTH];

    OPENFILENAME context;
    ZeroMemory(&context, sizeof(OPENFILENAME));

    context.lStructSize = sizeof(OPENFILENAME);
    context.hwndOwner = hWnd;
    context.lpstrFile = img->Name;
    context.lpstrFile[0] = NULL;
    context.nMaxFile = sizeof(img->Name);
    context.lpstrFilter = _T("All (*.*)\0*.*\0PCK (*.pck)\0*.PCK\0");
    context.nFilterIndex = 2;
    context.lpstrFileTitle = NULL;
    context.nMaxFileTitle = 0;
    context.lpstrInitialDir = NULL;
    context.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    HANDLE hFile;
    BOOL result = FALSE;

    if (GetOpenFileName(&context))
    {
        hFile = CreateFile(img->Name, GENERIC_READ, FILE_SHARE_READ, NULL,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        if (hFile == INVALID_HANDLE_VALUE)
        {
            wsprintf(message, _T("Unable to open file %s"), img->Name);
            MessageBox(hWnd, message, szWindowTitle, MB_ICONEXCLAMATION | MB_OK);

            return FALSE;
        }

        result = OpenImage(hFile, img);

        if (!result)
        {
            wsprintf(message, _T("Unable to process file %s"), img->Name);
            MessageBox(hWnd, message, szWindowTitle, MB_ICONERROR | MB_OK);
        }

        CloseHandle(hFile);
    }

    return result;
}

VOID DrawImage(HWND hWnd, CONST BOOL transparent, CONST BOOL scaled)
{
    if (image == NULL) { return; }

    DrawImage(hWnd, image->Surfaces[image->Index], transparent, scaled);
}

VOID DrawImage(HWND hWnd)
{
    if (image == NULL) { return; }

    BOOL transparent = FALSE;
    BOOL scaled = FALSE;

    HMENU menu = GetMenu(hWnd);

    // Transparency
    {
        MENUITEMINFO item;
        ZeroMemory(&item, sizeof(MENUITEMINFO));

        item.cbSize = sizeof(MENUITEMINFO);
        item.fMask = MIIM_STATE;

        GetMenuItemInfo(menu, ID_SETTINGS_TRANSPARENCY, FALSE, &item);

        transparent = item.fState == MFS_CHECKED;
    }

    // Scale
    {
        MENUITEMINFO item;
        ZeroMemory(&item, sizeof(MENUITEMINFO));

        item.cbSize = sizeof(MENUITEMINFO);
        item.fMask = MIIM_STATE;

        GetMenuItemInfo(menu, ID_SETTINGS_SCALE, FALSE, &item);

        scaled = item.fState == MFS_CHECKED;
    }

    DrawImage(hWnd, transparent, scaled);
}

BOOL OpenImage(HWND hWnd)
{
    IMAGECONTAINERPTR current = image;

    IMAGECONTAINERPTR next = (IMAGECONTAINERPTR)malloc(sizeof(IMAGECONTAINER));
    if (next == NULL) { return FALSE; }
    ZeroMemory(next, sizeof(IMAGECONTAINER));

    if (!OpenImage(hWnd, next)) { free(next); return FALSE; }

    image = next;

    UpdateTitle(hWnd);
    DrawImage(hWnd);

    EnableMenuItem(GetMenu(hWnd), ID_FILE_SAVE, MF_ENABLED);

    // Clean up...
    if (current != NULL)
    {
        for (UINT x = 0; x < current->Frames; x++)
        {
            RELEASE(current->Surfaces[x]);
        }

        free(current->Surfaces);
        free(current);
    }

    return TRUE;
}

VOID SaveImage(HWND hWnd)
{
    if (image == NULL) { return; }

    CHAR name[MAX_PATH];
    ZeroMemory(name, sizeof(name));

    OPENFILENAMEA context;
    ZeroMemory(&context, sizeof(OPENFILENAMEA));

    context.lStructSize = sizeof(OPENFILENAMEA);
    context.hwndOwner = hWnd;
    context.lpstrFile = name;
    context.lpstrFile[0] = NULL;
    context.nMaxFile = sizeof(name);
    context.lpstrFilter = "All (*.*)\0*.*\0BMP (*.bmp)\0*.BMP\0";
    context.nFilterIndex = 2;
    context.lpstrFileTitle = NULL;
    context.nMaxFileTitle = 0;
    context.lpstrInitialDir = NULL;
    context.Flags = OFN_PATHMUSTEXIST;

    if (GetSaveFileNameA(&context))
    {
        LPVOID content = NULL;

        UINT stride, width, height = 0;

        if (AcquireDirectDrawSurfaceContent(image->Surfaces[image->Index], &content, &width, &height, &stride))
        {
            SavePixels(name, (USHORT*)content, width, height, stride / sizeof(USHORT));

            ReleaseDirectDrawSurfaceContent(image->Surfaces[image->Index]);
        }
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case ID_FILE_OPEN: { OpenImage(hWnd); break; }
        case ID_FILE_SAVE: { if (image != NULL) { SaveImage(hWnd); } break; }
        case ID_HELP_ABOUT: { DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About); break; }
        case ID_SETTINGS_TRANSPARENCY:
        case ID_SETTINGS_SCALE:
        {
            HMENU menu = GetMenu(hWnd);

            MENUITEMINFO item;
            ZeroMemory(&item, sizeof(MENUITEMINFO));

            item.cbSize = sizeof(MENUITEMINFO);
            item.fMask = MIIM_STATE;

            GetMenuItemInfo(menu, LOWORD(wParam), FALSE, &item);

            if (item.fState == MFS_CHECKED) { item.fState = MFS_UNCHECKED; }
            else { item.fState = MFS_CHECKED; }

            SetMenuItemInfo(menu, LOWORD(wParam), FALSE, &item);

            if (image != NULL) { DrawImage(hWnd); }

            break;
        }
        case ID_FILE_EXIT: { DestroyWindow(hWnd); break; }
        default: { return DefWindowProc(hWnd, message, wParam, lParam); }
        }

        break;
    }
    case WM_KEYUP:
    {
        if (image == NULL) { break; }

        BOOL changed = FALSE;

        if (wParam == VK_LEFT) { if (image->Index != 0) { image->Index = image->Index - 1; changed = TRUE; } }
        else if (wParam == VK_RIGHT || wParam == VK_SPACE) { if (image->Index < image->Frames - 1) { image->Index = image->Index + 1; changed = TRUE; } }

        if (changed) { UpdateTitle(hWnd); DrawImage(hWnd); }

        break;
    }
    case WM_MOVE:
    case WM_SIZE: { if (image != NULL) { DrawImage(hWnd); } break; }
    case WM_DESTROY: { ReleaseDirectDraw(); PostQuitMessage(EXIT_SUCCESS); break; }
    default: { return DefWindowProc(hWnd, message, wParam, lParam); }
    }

    return 0;
}

BOOL InitInstance(HINSTANCE hInstance, INT nCmdShow)
{
    WNDCLASSEX wcex;
    ZeroMemory(&wcex, sizeof(WNDCLASSEX));

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_STAR));
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wcex.lpszMenuName = MAKEINTRESOURCE(IDR_MENU);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = NULL;

    ATOM atom = RegisterClassEx(&wcex);

    if (!atom) { return FALSE; }

    HWND hWnd = CreateWindow(szWindowClass, szWindowTitle,
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, 800, 600, NULL, NULL, hInstance, NULL);

    if (!hWnd) { return FALSE; }

    BOOL colorkeys = FALSE;
    if (FAILED(InitDirectDraw(hWnd, &colorkeys)))
    {
        MessageBox(NULL, _T("Unable to initialize DirectDraw."), szWindowTitle, MB_ICONERROR | MB_OK);

        DestroyWindow(hWnd);

        return FALSE;
    }

    if (!colorkeys) { EnableMenuItem(GetMenu(hWnd), ID_SETTINGS_TRANSPARENCY, MF_DISABLED); }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

INT APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ INT       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    hInst = hInstance;

    LoadString(hInstance, IDS_WINDOW_TITLE, szWindowTitle, MAX_LOAD_STRING_LENGTH);
    LoadString(hInstance, IDS_WINDOW_CLASS, szWindowClass, MAX_LOAD_STRING_LENGTH);

    if (!InitInstance(hInstance, nCmdShow)) { return FALSE; }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_ACCELERATOR));

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (INT)msg.wParam;
}