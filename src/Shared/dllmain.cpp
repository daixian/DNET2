// dllmain.cpp : 定义 DLL 应用程序的入口点。
#if defined(_WIN32) || defined(_WIN64)

#    include <SDKDDKVer.h>
#    define WIN32_LEAN_AND_MEAN // 从 Windows 头文件中排除极少使用的内容
// Windows 头文件
#    include <windows.h>

BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD ul_reason_for_call,
                      LPVOID lpReserved)
{
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:

    case DLL_PROCESS_DETACH: //卸载dll的时候自动释放
        //目前发现貌似最后一次进入是0
        if (ul_reason_for_call == 0) {
            //执行一点关闭动作吧
        }
        break;
    }
    return TRUE;
}

#endif