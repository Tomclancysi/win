// WindowsProject1.cpp : 定义应用程序的入口点。
//
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "framework.h"
#include "WindowsProject1.h"
#include <sstream>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <map>
#include <set>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#include <wininet.h>
#include <cstdarg>
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "wininet.lib")

#define MAX_LOADSTRING 100

// UNC路径工具函数
namespace UNCHelper
{
    // 判断一个路径是否是UNC share root (格式: \\server\share)
    bool IsUNCShareRoot(const std::wstring& path)
    {
        if (path.length() < 5) return false; // 最短UNC路径: \\a\b
        
        // 必须以\\开头
        if (path[0] != L'\\' || path[1] != L'\\') return false;
        
        // 查找第一个\的位置（服务器名结束）
        size_t firstSlash = path.find(L'\\', 2);
        if (firstSlash == std::wstring::npos) return false;
        
        // 查找第二个\的位置（共享名结束）
        size_t secondSlash = path.find(L'\\', firstSlash + 1);
        
        // 如果没有第二个\，说明是share root
        return secondSlash == std::wstring::npos;
    }
    
    // 提取路径中的share root (返回 \\server\share)
    std::wstring ExtractShareRoot(const std::wstring& path)
    {
        if (path.length() < 5) return L""; // 最短UNC路径: \\a\b
        
        // 必须以\\开头
        if (path[0] != L'\\' || path[1] != L'\\') return L"";
        
        // 查找第二个\的位置
        size_t firstSlash = path.find(L'\\', 2);
        if (firstSlash == std::wstring::npos) return L"";
        
        size_t secondSlash = path.find(L'\\', firstSlash + 1);
        
        // 如果没有第二个\，说明本身就是share root
        if (secondSlash == std::wstring::npos)
        {
            return path;
        }
        
        // 返回 \\server\share 部分
        return path.substr(0, secondSlash);
    }
}

// 字符串转换工具函数
namespace StringHelper
{
    // 将std::string转换为std::wstring
    std::wstring StringToWString(const std::string& str)
    {
        if (str.empty()) return L"";
        int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
        std::wstring wstr(size - 1, 0);
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], size);
        return wstr;
    }
    
    // 将std::wstring转换为std::string
    std::string WStringToString(const std::wstring& wstr)
    {
        if (wstr.empty()) return "";
        int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
        std::string str(size - 1, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], size, nullptr, nullptr);
        return str;
    }
}

namespace DebugHelper
{
    // 输出调试信息（支持格式化）
    void DebugPrint(const std::wstring& message)
    {
        OutputDebugStringW((message + L"\n").c_str());
    }
    
    // 输出调试信息（支持格式化参数）
    void DebugPrintf(const wchar_t* format, ...)
    {
        wchar_t buffer[1024];
        va_list args;
        va_start(args, format);
        vswprintf_s(buffer, format, args);
        va_end(args);
        OutputDebugStringW(buffer);
    }

    class Logger
    {
    public:
        Logger(const char* file, int line)
            : _file(file), _line(line)
        {
            // 获取当前系统时间
            auto now = std::chrono::system_clock::now();
            std::time_t t = std::chrono::system_clock::to_time_t(now);
            struct tm local_tm;
            localtime_s(&local_tm, &t);
            std::wostringstream oss;
            oss << L"["
                << StringHelper::StringToWString(std::string(_file)) << L" "
                << _line << L" "
                << std::put_time(&local_tm, L"%H:%M:%S")
                << L"] ";
            _prefix = oss.str();
        }

        ~Logger()
        {
            std::wstring fullMsg = _prefix + _ss.str() + L"\n";
            OutputDebugStringW(fullMsg.c_str());
        }

        template <typename T>
        Logger& operator<<(const T& value)
        {
            _ss << value;
            return *this;
        }

    private:
        const char* _file;
        int _line;
        std::wostringstream _ss;
        std::wstring _prefix;
    };
}

// 用法: LOG_INFO << L"abc" << 123 << ...;
#define LOG_INFO DebugHelper::Logger(__FILE__, __LINE__)

// Logger使用示例：
// LOG_INFO << L"这是一条调试信息";
// LOG_INFO << L"变量值: " << variable << L", 状态: " << status;
// LOG_INFO << L"连接成功，IP: " << ipAddress;

// 网络连接测试工具函数
namespace NetworkHelper
{
    // 测试UNC路径连接性
    bool TestUNCConnection(const std::wstring& uncPath)
    {
        try
        {
            DWORD attributes = GetFileAttributesW(uncPath.c_str());
            return attributes != INVALID_FILE_ATTRIBUTES;
        }
        catch (...)
        {
            return false;
        }
    }
    
    // 获取连接错误描述
    std::wstring GetConnectionErrorString(DWORD error)
    {
        switch (error)
        {
        case ERROR_PATH_NOT_FOUND:
            return L"路径未找到";
        case ERROR_BAD_NETPATH:
            return L"网络路径错误";
        case ERROR_ACCESS_DENIED:
            return L"访问被拒绝";
        case ERROR_NETWORK_UNREACHABLE:
            return L"网络不可达";
        case ERROR_HOST_UNREACHABLE:
            return L"主机不可达";
        case ERROR_CONNECTION_REFUSED:
            return L"连接被拒绝";
        default:
            return L"未知错误: " + std::to_wstring(error);
        }
    }
}

// 线程同步工具函数
namespace ThreadHelper
{
    // 可中断的等待函数
    void InterruptibleSleep(int milliseconds, const std::atomic<bool>& shouldStop)
    {
        int steps = milliseconds / 100;
        for (int i = 0; i < steps && !shouldStop.load(); i++)
        {
            Sleep(100);
        }
    }
    
    // 安全地停止线程
    void SafeStopThread(std::thread& thread, std::atomic<bool>& flag)
    {
        flag.store(false);
        if (thread.joinable())
        {
            thread.join();
        }
    }
}

//

// 回调函数，用于枚举显示器
BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
	std::vector<MONITORINFOEX>* monitors = reinterpret_cast<std::vector<MONITORINFOEX>*>(dwData);

	MONITORINFOEX monitorInfo;
	monitorInfo.cbSize = sizeof(MONITORINFOEX);

	if (GetMonitorInfo(hMonitor, &monitorInfo))
	{
		monitors->push_back(monitorInfo);
	}

	return TRUE; // 继续枚举
}

// 主函数：检测显示器数量和分辨率
void DetectDisplayMonitors()
{
	std::vector<MONITORINFOEX> monitors;

	// 枚举所有显示器
	EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, reinterpret_cast<LPARAM>(&monitors));

	// 输出显示器数量
	LOG_INFO << L"检测到 " << monitors.size() << L" 个显示器";

	// 输出每个显示器的信息
	for (size_t i = 0; i < monitors.size(); ++i)
	{
		std::wstringstream monitorInfo;
		monitorInfo << L"显示器 " << (i + 1) << L":\n";
		monitorInfo << L"  设备名称: " << monitors[i].szDevice << L"\n";
		monitorInfo << L"  分辨率: " << (monitors[i].rcMonitor.right - monitors[i].rcMonitor.left)
			<< L" x " << (monitors[i].rcMonitor.bottom - monitors[i].rcMonitor.top) << L"\n";
		monitorInfo << L"  位置: (" << monitors[i].rcMonitor.left << L", " << monitors[i].rcMonitor.top
			<< L") 到 (" << monitors[i].rcMonitor.right << L", " << monitors[i].rcMonitor.bottom << L")\n";

		if (monitors[i].dwFlags & MONITORINFOF_PRIMARY)
		{
			monitorInfo << L"  主显示器: 是\n";
		}
		else
		{
			monitorInfo << L"  主显示器: 否\n";
		}

		monitorInfo << L"\n";
		LOG_INFO << monitorInfo.str();
	}
}

// 简化版本（如果只需要基本分辨率信息）
void DetectDisplayMonitorsSimple()
{
	// 获取系统DC
	HDC hdc = GetDC(NULL);
	if (hdc)
	{
		int screenWidth = GetSystemMetrics(SM_CXSCREEN);
		int screenHeight = GetSystemMetrics(SM_CYSCREEN);
		int virtualWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
		int virtualHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
		int monitorCount = GetSystemMetrics(SM_CMONITORS);

		LOG_INFO << L"显示器数量: " << monitorCount;
		LOG_INFO << L"主显示器分辨率: " << screenWidth << L" x " << screenHeight;
		LOG_INFO << L"虚拟屏幕大小: " << virtualWidth << L" x " << virtualHeight;

		ReleaseDC(NULL, hdc);
	}
}

void GetScreenInfo()
{
	HDC hdc = GetDC(NULL);

	// 获取屏幕分辨率（逻辑像素）
	int screenWidth = GetDeviceCaps(hdc, HORZRES);    // 1920
	int screenHeight = GetDeviceCaps(hdc, VERTRES);   // 1080

	// 获取逻辑DPI
	int dpiX = GetDeviceCaps(hdc, LOGPIXELSX);        // 96
	int dpiY = GetDeviceCaps(hdc, LOGPIXELSY);        // 96

	LOG_INFO << L"屏幕分辨率: " << screenWidth << L" x " << screenHeight;
	LOG_INFO << L"逻辑DPI: " << dpiX << L" x " << dpiY;

	ReleaseDC(NULL, hdc);
}

// 全局变量:
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名

// 网络监控相关全局变量
HWND g_hCollectButton = nullptr;                // 收集按钮句柄
HWND g_hWarmUpButton = nullptr;                 // WarmUp按钮句柄
HWND g_hIpList = nullptr;                       // IP列表控件句柄
std::atomic<bool> g_bCollecting(false);         // 是否正在收集
std::atomic<bool> g_bWarmUpRunning(false);      // WarmUp是否运行中
std::thread g_collectThread;                    // 收集线程
std::thread g_warmUpThread;                     // WarmUp线程
std::mutex g_ipListMutex;                       // IP列表互斥锁
std::set<std::string> g_collectedIPs;           // 收集到的IP集合
std::vector<std::string> g_warmUpTargets;       // WarmUp目标列表

// 此代码模块中包含的函数的前向声明:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

// 网络监控和WarmUp功能函数声明
void                CollectNetworkConnections();
void                WarmUpConnections();
void                UpdateIpList();
std::string         IpToString(DWORD ip);
void                AddIpToList(const std::string& ip);

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
      CW_USEDEFAULT, 0, 800, 600, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   // 创建控件
   g_hCollectButton = CreateWindowW(L"BUTTON", L"开始收集SMB连接(端口445)",
       WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
       10, 10, 180, 30, hWnd, (HMENU)IDC_COLLECT_BUTTON, hInstance, nullptr);

   g_hWarmUpButton = CreateWindowW(L"BUTTON", L"WarmUp连接",
       WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
       200, 10, 150, 30, hWnd, (HMENU)IDC_WARMUP_BUTTON, hInstance, nullptr);

   g_hIpList = CreateWindowW(L"LISTBOX", L"",
       WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL | LBS_NOTIFY,
       10, 50, 760, 500, hWnd, (HMENU)IDC_IP_LIST, hInstance, nullptr);

   // 初始化WarmUp目标列表
   //g_warmUpTargets = {
   //    "\\\\192.168.1.1\\share",
   //};

   {// 设置scroll range
	   SetScrollRange(hWnd, SB_VERT, 0, 5, TRUE);
	   SetScrollRange(hWnd, SB_HORZ, 0, 5, TRUE);
	   // SetScrollInfo同样可以设置滚动条信息，并且更加灵活
       SCROLLINFO si;
	   si.cbSize = sizeof(SCROLLBARINFO);
	   
   }
   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   DetectDisplayMonitors(); // 检测显示器信息
   GetScreenInfo();

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
            case IDC_COLLECT_BUTTON:
                {
                    if (!g_bCollecting.load())
                    {
                        // 开始收集
                        g_bCollecting.store(true);
                        SetWindowTextW(g_hCollectButton, L"停止收集SMB连接(端口445)");
                        g_collectThread = std::thread(CollectNetworkConnections);
                    }
                    else
                    {
                        // 停止收集
                        g_bCollecting.store(false);
                        SetWindowTextW(g_hCollectButton, L"开始收集SMB连接(端口445)");
                        if (g_collectThread.joinable())
                        {
                            g_collectThread.join();
                        }
                    }
                }
                break;
            case IDC_WARMUP_BUTTON:
                {
                    if (!g_bWarmUpRunning.load())
                    {
                        // 开始WarmUp
                        g_bWarmUpRunning.store(true);
                        SetWindowTextW(g_hWarmUpButton, L"停止WarmUp");
                        g_warmUpThread = std::thread(WarmUpConnections);
                    }
                    else
                    {
                        // 停止WarmUp
                        g_bWarmUpRunning.store(false);
                        SetWindowTextW(g_hWarmUpButton, L"WarmUp连接");
                        if (g_warmUpThread.joinable())
                        {
                            g_warmUpThread.join();
                        }
                    }
                }
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
			LOG_INFO << L"centerX=" << centerX << L", centerY=" << centerY << L", radius=" << radius;

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
        {
            // 停止所有线程
            g_bCollecting.store(false);
            g_bWarmUpRunning.store(false);
            
            if (g_collectThread.joinable())
            {
                g_collectThread.join();
            }
            if (g_warmUpThread.joinable())
            {
                g_warmUpThread.join();
            }
        }
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
        break;
    case WM_SIZE:
    {
		// 当初始化窗口之后，会触发WM_SIZE消息，所以在此处可以记载一些大小信息
		int width = LOWORD(lParam);
		int height = HIWORD(lParam);

    }
        break;
    case WM_USER + 1:
        {
            // 更新IP列表显示
            UpdateIpList();
        }
        break;
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

// 网络连接收集功能实现
void CollectNetworkConnections()
{
    while (g_bCollecting.load())
    {
        try
        {
            MIB_TCPTABLE_OWNER_PID* pTcpTable = nullptr;
            DWORD dwSize = 0;
            DWORD dwRetVal = 0;

            // 获取所需缓冲区大小
            dwRetVal = GetExtendedTcpTable(pTcpTable, &dwSize, TRUE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0);
            if (dwRetVal == ERROR_INSUFFICIENT_BUFFER)
            {
                pTcpTable = (MIB_TCPTABLE_OWNER_PID*)malloc(dwSize);
                if (pTcpTable == nullptr)
                {
                    Sleep(1000);
                    continue;
                }
            }

            // 获取TCP表
            dwRetVal = GetExtendedTcpTable(pTcpTable, &dwSize, TRUE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0);
            if (dwRetVal == NO_ERROR)
            {
                for (DWORD i = 0; i < pTcpTable->dwNumEntries; i++)
                {
                    if (!g_bCollecting.load()) break; // 检查是否应该停止
                    
                    MIB_TCPROW_OWNER_PID row = pTcpTable->table[i];
                    
                    // 检查是否为Remote端口445的连接（SMB协议）
                    if (ntohs(row.dwRemotePort) == 445 && row.dwState == MIB_TCP_STATE_ESTAB)
                    {
                        std::string ip = IpToString(row.dwRemoteAddr);
                        AddIpToList(ip);
                    }
                }
            }

            if (pTcpTable)
            {
                free(pTcpTable);
                pTcpTable = nullptr;
            }
        }
        catch (...)
        {
            LOG_INFO << L"网络连接收集过程中发生异常";
        }

        // 每2秒检查一次，但允许提前退出
        for (int i = 0; i < 20 && g_bCollecting.load(); i++)
        {
            Sleep(100);
        }
    }
}

// WarmUp连接功能实现
void WarmUpConnections()
{
    while (g_bWarmUpRunning.load())
    {
        for (const auto& target : g_warmUpTargets)
        {
            if (!g_bWarmUpRunning.load()) break;

            try
            {
                // 尝试访问SMB共享
                std::wstring wtarget(target.begin(), target.end());
                DWORD attributes = GetFileAttributesW(wtarget.c_str());
                
                if (attributes != INVALID_FILE_ATTRIBUTES)
                {
                    LOG_INFO << L"WarmUp成功: " << wtarget;
                }
                else
                {
                    DWORD error = GetLastError();
                    // 只记录非预期的错误（如网络不可达等）
                    if (error != ERROR_PATH_NOT_FOUND && error != ERROR_BAD_NETPATH)
                    {
                        LOG_INFO << L"WarmUp失败: " << wtarget << L" 错误: " << std::to_wstring(error);
                    }
                }
            }
            catch (...)
            {
                LOG_INFO << L"WarmUp异常: " << std::wstring(target.begin(), target.end());
            }

            // 每个目标间隔5秒，但允许提前退出
            for (int i = 0; i < 50 && g_bWarmUpRunning.load(); i++)
            {
                Sleep(100);
            }
        }
        
        // 一轮结束后等待30秒，但允许提前退出
        for (int i = 0; i < 300 && g_bWarmUpRunning.load(); i++)
        {
            Sleep(100);
        }
    }
}

// 更新IP列表显示
void UpdateIpList()
{
    if (g_hIpList)
    {
        SendMessageW(g_hIpList, LB_RESETCONTENT, 0, 0);
        
        std::lock_guard<std::mutex> lock(g_ipListMutex);
        for (const auto& ip : g_collectedIPs)
        {
            std::wstring wip(ip.begin(), ip.end());
            SendMessageW(g_hIpList, LB_ADDSTRING, 0, (LPARAM)wip.c_str());
        }
    }
}

// IP地址转换为字符串
std::string IpToString(DWORD ip)
{
    struct in_addr addr;
    addr.s_addr = ip;
    return std::string(inet_ntoa(addr));
}

// 添加IP到列表
void AddIpToList(const std::string& ip)
{
    std::lock_guard<std::mutex> lock(g_ipListMutex);
    if (g_collectedIPs.find(ip) == g_collectedIPs.end())
    {
        g_collectedIPs.insert(ip);
        // 使用PostMessage通知主线程更新UI
        PostMessageW(GetParent(g_hIpList), WM_USER + 1, 0, 0);
    }
}
