// WindowsProject1.cpp : 定义应用程序的入口点。
//

#include "framework.h"
#include "WindowsProject1.h"

#define MAX_LOADSTRING 100

// 全局变量:
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名

// 此代码模块中包含的函数的前向声明:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 在此处放置代码。

    // 初始化全局字符串
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_WINDOWSPROJECT1, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 执行应用程序初始化:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WINDOWSPROJECT1));

    MSG msg;

    // 主消息循环:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  函数: MyRegisterClass()
//
//  目标: 注册窗口类。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WINDOWSPROJECT1));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_WINDOWSPROJECT1);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   函数: InitInstance(HINSTANCE, int)
//
//   目标: 保存实例句柄并创建主窗口
//
//   注释:
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 将实例句柄存储在全局变量中

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, 
       WS_OVERLAPPEDWINDOW | WS_VSCROLL | WS_HSCROLL,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   {// 设置scroll range
	   SetScrollRange(hWnd, SB_VERT, 0, 5, TRUE);
	   SetScrollRange(hWnd, SB_HORZ, 0, 5, TRUE);
   }
   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目标: 处理主窗口的消息。
//
//  WM_COMMAND  - 处理应用程序菜单
//  WM_PAINT    - 绘制主窗口
//  WM_DESTROY  - 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 分析菜单选择:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
		    PAINTSTRUCT ps;
		    HDC hdc = BeginPaint(hWnd, &ps);
		    RECT rect;
		    GetClientRect(hWnd, &rect);
		    int centerX = (rect.right - rect.left) / 2;
		    int centerY = (rect.bottom - rect.top) / 2;
		    int radius = min(centerX, centerY) / 2;

		    // 获取滚动条位置
		    int vScrollPos = GetScrollPos(hWnd, SB_VERT);
		    int hScrollPos = GetScrollPos(hWnd, SB_HORZ);

		    // 根据滚动条位置调整绘制位置
		    centerX -= hScrollPos * 20; // 20为每步移动的像素，可根据需要调整
		    centerY -= vScrollPos * 20;

			// debug print centerX, centerY, radius
			wchar_t buf[100];
			wsprintf(buf, L"centerX=%d, centerY=%d, radius=%d\n", centerX, centerY, radius);
			OutputDebugString(buf);

            // 1. 画脸部（黄色填充圆）
            HBRUSH hBrushFace = CreateSolidBrush(RGB(255, 255, 0));
            HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrushFace);
            HPEN hPenFace = CreatePen(PS_SOLID, 2, RGB(0, 0, 0));
            HPEN hOldPen = (HPEN)SelectObject(hdc, hPenFace);
            Ellipse(hdc, centerX - radius, centerY - radius, centerX + radius, centerY + radius);

            // 2. 画左眼（黑色小圆）
            int eyeRadius = radius / 6;
            int eyeOffsetX = radius / 2;
            int eyeOffsetY = radius / 3;
            HBRUSH hBrushEye = CreateSolidBrush(RGB(0, 0, 0));
            SelectObject(hdc, hBrushEye);
            Ellipse(hdc, centerX - eyeOffsetX - eyeRadius, centerY - eyeOffsetY - eyeRadius,
                         centerX - eyeOffsetX + eyeRadius, centerY - eyeOffsetY + eyeRadius);

            // 3. 画右眼（黑色小圆）
            Ellipse(hdc, centerX + eyeOffsetX - eyeRadius, centerY - eyeOffsetY - eyeRadius,
                         centerX + eyeOffsetX + eyeRadius, centerY - eyeOffsetY + eyeRadius);

            // 4. 画嘴（黑色弧线）
            int mouthRadius = radius / 2;
            int mouthTop = centerY + radius / 4;
            Arc(hdc,
                centerX - mouthRadius, mouthTop - mouthRadius / 2,
                centerX + mouthRadius, mouthTop + mouthRadius / 2,
                centerX - mouthRadius / 2, mouthTop,
                centerX + mouthRadius / 2, mouthTop);

            // 清理GDI对象
            SelectObject(hdc, hOldBrush);
            SelectObject(hdc, hOldPen);
            DeleteObject(hBrushFace);
            DeleteObject(hBrushEye);
            DeleteObject(hPenFace);

            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_VSCROLL:
    {
        int scrollPos = GetScrollPos(hWnd, SB_VERT);
        switch (LOWORD(wParam))
        {
        case SB_LINEUP: // 上滚一行
            scrollPos -= 1;
            break;
        case SB_LINEDOWN: // 下滚一行
            scrollPos += 1;
            break;
        case SB_PAGEUP: // 上滚一页
            scrollPos -= 1;
            break;
        case SB_PAGEDOWN: // 下滚一页
            scrollPos += 1;
            break;
        case SB_THUMBTRACK: // 拖动滚动条
            scrollPos = HIWORD(wParam);
            break;
   //     case SB_THUMBPOSITION: // 最后位置
   //         scrollPos = HIWORD(wParam);
			//break;
        default:
            break;
        }
        // 设置新的滚动位置，范围在0到5之间
        scrollPos = max(0, min(scrollPos, 5));
        SetScrollPos(hWnd, SB_VERT, scrollPos, TRUE);
        InvalidateRect(hWnd, NULL, TRUE); // 重绘窗口
	}
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
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
