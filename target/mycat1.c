#include <stdio.h>
#include <windows.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    // 转换为宽字符路径（支持中文路径）
    wchar_t wfilename[MAX_PATH];
    MultiByteToWideChar(CP_ACP, 0, argv[1], -1, wfilename, MAX_PATH);

    // 使用Windows API打开文件
    HANDLE hFile = CreateFileW(
        wfilename,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        fprintf(stderr, "无法打开文件，错误码: %lu\n", error);
        return 1;
    }

    printf("文件已成功打开，正在逐字符读取...\n");

    char c;
    DWORD bytesRead;

    // 逐字符读取
    while (ReadFile(hFile, &c, 1, &bytesRead, NULL) && bytesRead > 0) {
        if (putchar(c) == EOF) {
            fprintf(stderr, "输出错误\n");
            CloseHandle(hFile);
            return 1;
        }
    }

    if (!CloseHandle(hFile)) {
        fprintf(stderr, "关闭文件错误\n");
        return 1;
    }

    return 0;
}