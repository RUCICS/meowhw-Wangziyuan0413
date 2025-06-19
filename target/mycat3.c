#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

// 获取系统内存分配粒度（通常是页面大小）
size_t io_blocksize() {
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    return sys_info.dwAllocationGranularity;
}

// 对齐内存分配函数
char* align_alloc(size_t size) {
    // 获取系统内存分配粒度
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    size_t alignment = sys_info.dwAllocationGranularity;
    
    // 分配额外空间用于对齐和存储原始指针
    size_t total_size = size + alignment + sizeof(void*);
    void* original_ptr = malloc(total_size);
    if (!original_ptr) return NULL;
    
    // 计算对齐后的指针位置
    void* aligned_ptr = (void*)(((size_t)original_ptr + alignment + sizeof(void*)) & ~(alignment - 1));
    
    // 在对齐指针前存储原始指针
    *((void**)((char*)aligned_ptr - sizeof(void*))) = original_ptr;
    
    return (char*)aligned_ptr;
}

// 对齐内存释放函数
void align_free(void* ptr) {
    if (!ptr) return;
    
    // 从对齐指针前获取原始指针
    void* original_ptr = *((void**)((char*)ptr - sizeof(void*)));
    free(original_ptr);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    // 打开文件
    HANDLE hFile = CreateFileA(
        argv[1],
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Error opening file: %lu\n", GetLastError());
        return 1;
    }

    // 获取对齐的缓冲区
    size_t buf_size = io_blocksize();
    char *buf = align_alloc(buf_size);
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
            align_free(buf);
            CloseHandle(hFile);
            return 1;
        }
    }

    align_free(buf);
    if (!CloseHandle(hFile)) {
        fprintf(stderr, "Error closing file: %lu\n", GetLastError());
        return 1;
    }

    return 0;
}