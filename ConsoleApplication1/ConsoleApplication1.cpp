// ConsoleApplication1.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <windows.h>
#include <iostream>
#include <thread>

#include <windows.h>
#include <winnetwk.h>

// 线程池回调函数

//void CALLBACK NetworkConnectionCallback(PTP_CALLBACK_INSTANCE instance, PVOID context, PTP_WORK work)
//{
//	auto params = static_cast<ConnectionParams*>(context);
//	// 模拟网络连接操作
//	Sleep(1000);
//	params->errorCode = 0;  // 假设成功
//	SetEvent(params->completionEvent);
//	CloseThreadpoolWork(work);
//}

VOID CALLBACK NetworkConnectionCallback(PTP_CALLBACK_INSTANCE instance, PVOID context, PTP_WORK work)
{
    auto* params = static_cast<struct ConnectionParams*>(context);
    
    NETRESOURCE netResource = { 0 };
    netResource.dwType = RESOURCETYPE_DISK;
    netResource.lpRemoteName = const_cast<LPWSTR>(params->remoteName);
    
    DWORD result = WNetAddConnection2(&netResource, 
                                    params->password, 
                                    params->username, 
                                    0);
    
    // 通知主线程结果
    SetEvent(params->completionEvent);
    params->result = result;
}

struct ConnectionParams {
    LPCWSTR remoteName;
    LPCWSTR password;
    LPCWSTR username;
    HANDLE completionEvent;
    DWORD result;
};

PTP_WORK ConnectToShareAsync(const std::wstring& server,
    const std::wstring& share,
    const std::wstring& username,
    const std::wstring& password,
    HANDLE& outEvent)
{
    auto params = new ConnectionParams{
    (L"\\\\\\" + server + L"\\" + share).c_str(),
    password.c_str(),
    username.c_str(),
    CreateEvent(NULL, TRUE, FALSE, NULL),
    0
    };

    if (!params->completionEvent) {
    std::wcerr << L"CreateEvent failed: " << GetLastError() << std::endl;
    delete params;
    return nullptr;
    }

    PTP_WORK work = CreateThreadpoolWork(NetworkConnectionCallback, params, nullptr);
    if (!work) {
    std::wcerr << L"CreateThreadpoolWork failed: " << GetLastError() << std::endl;
    CloseHandle(params->completionEvent);
    delete params;
    return nullptr;
    }

    SubmitThreadpoolWork(work);
    outEvent = params->completionEvent;
    return work;
}

void TestCancelSyncIo() {
    // https://learn.microsoft.com/en-us/windows/win32/fileio/canceling-pending-i-o-operations
    // 不是所有的driver都支持取消同步IO，也不是所有的阶段都能够被取消，具体需要根据driver的实现来确定。
    std::thread t([]{
        HANDLE hFile = CreateFileA("\\\\192.168.1.100\\share\\test.txt", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE) {
            std::cout << "CreateFileA failed: " << GetLastError() << std::endl;
            return;
        }
        CloseHandle(hFile);
        std::cout << "CreateFileA succeeded" << std::endl;
    });

    Sleep(1000);
    CancelSynchronousIo(t.native_handle());
    if (t.joinable()) {
        t.join();
    }
    std::cout << "TestCancelSyncIo finished" << std::endl;
}

int main()
{
    TestCancelSyncIo();
    return 0;
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
