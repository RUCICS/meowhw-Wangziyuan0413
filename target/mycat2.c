#include <stdio.h>
#include <stdlib.h>
#include <windows.h>  // Windows API头文件

size_t io_blocksize() {
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);  // 获取系统信息
    return sys_info.dwAllocationGranularity;  // 返回内存分配粒度（通常是页面大小）
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    // 打开文件（Windows风格）
    HANDLE hFile = CreateFileA(
        argv[1],                // 文件名
        GENERIC_READ,           // 只读访问
        FILE_SHARE_READ,        // 允许其他进程读取
        NULL,                   // 默认安全属性
        OPEN_EXISTING,          // 只打开已存在的文件
        FILE_ATTRIBUTE_NORMAL,  // 普通文件属性
        NULL                    // 不使用模板文件
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Error opening file: %lu\n", GetLastError());
        return 1;
    }

    size_t buf_size = io_blocksize();
    char *buf = malloc(buf_size);
    if (!buf) {
        fprintf(stderr, "Memory allocation failed\n");
        CloseHandle(hFile);
        return 1;
    }

    DWORD bytesRead;
    BOOL result;
    
    // 循环读取并输出文件内容
    while ((result = ReadFile(hFile, buf, buf_size, &bytesRead, NULL)) && bytesRead > 0) {
        if (fwrite(buf, 1, bytesRead, stdout) != bytesRead) {
            fprintf(stderr, "Error writing to stdout\n");
            free(buf);
            CloseHandle(hFile);
            return 1;
        }
    }

    free(buf);
    if (!CloseHandle(hFile)) {
        fprintf(stderr, "Error closing file: %lu\n", GetLastError());
        return 1;
    }

    return 0;
}