#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <fileapi.h>

// 实验确定的最佳倍数 (根据实际测试结果调整)
#define BASE_BLOCK_SIZE 4096  // 4KB基础块大小
#define OPTIMAL_MULTIPLIER 512  // 2048KB / 4KB = 512

size_t io_blocksize(const char* filename) {
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    size_t mem_granularity = sys_info.dwAllocationGranularity;

    // 获取文件系统块大小
    HANDLE hFile = CreateFileA(
        filename,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    size_t fs_block_size = 4096;  // 默认值
    if (hFile != INVALID_HANDLE_VALUE) {
        FILE_STORAGE_INFO fsInfo;
        if (GetFileInformationByHandleEx(
            hFile,
            FileStorageInfo,
            &fsInfo,
            sizeof(fsInfo))) {
            
            if (fsInfo.PhysicalBytesPerSectorForPerformance >= 512 && 
                (fsInfo.PhysicalBytesPerSectorForPerformance & (fsInfo.PhysicalBytesPerSectorForPerformance - 1)) == 0) {
                fs_block_size = fsInfo.PhysicalBytesPerSectorForPerformance;
            }
        }
        CloseHandle(hFile);
    }

    // 计算基础缓冲区大小
    size_t base_size = (mem_granularity > fs_block_size) ? mem_granularity : fs_block_size;
    
    // 应用实验确定的最佳倍数
    size_t buffer_size = base_size * OPTIMAL_MULTIPLIER;
    
    // 限制最大缓冲区大小(16MB)
    const size_t max_buffer_size = 4 * 1024 * 1024; 
    if (buffer_size > max_buffer_size) {
        buffer_size = max_buffer_size;
    }

    return buffer_size;
}

// 对齐内存分配函数(与之前相同)
char* align_alloc(size_t size) {
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    size_t alignment = sys_info.dwAllocationGranularity;
    
    size_t total_size = size + alignment + sizeof(void*);
    void* original_ptr = malloc(total_size);
    if (!original_ptr) return NULL;
    
    void* aligned_ptr = (void*)(((size_t)original_ptr + alignment + sizeof(void*)) & ~(alignment - 1));
    *((void**)((char*)aligned_ptr - sizeof(void*))) = original_ptr;
    
    return (char*)aligned_ptr;
}

void align_free(void* ptr) {
    if (!ptr) return;
    void* original_ptr = *((void**)((char*)ptr - sizeof(void*)));
    free(original_ptr);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    HANDLE hFile = CreateFileA(
        argv[1],
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,  // 添加顺序读取标志
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Error opening file: %lu\n", GetLastError());
        return 1;
    }

    size_t buf_size = io_blocksize(argv[1]);
    printf("Using optimized buffer size: %zu bytes (%.1f KB)\n", 
           buf_size, buf_size / 1024.0);

    char *buf = align_alloc(buf_size);
    if (!buf) {
        fprintf(stderr, "Memory allocation failed\n");
        CloseHandle(hFile);
        return 1;
    }

    DWORD bytesRead;
    BOOL result;
    
    LARGE_INTEGER fileSize;
    GetFileSizeEx(hFile, &fileSize);
    LARGE_INTEGER totalRead = {0};

    while ((result = ReadFile(hFile, buf, buf_size, &bytesRead, NULL)) && bytesRead > 0) {
        totalRead.QuadPart += bytesRead;
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