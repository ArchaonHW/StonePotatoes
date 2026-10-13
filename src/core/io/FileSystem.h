#pragma once

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace StonePotatoes {
namespace IO {

enum class FileMode {
    Read,
    Write,
    Append,
    ReadWrite
};

enum class SeekOrigin {
    Begin,
    Current,
    End
};

// Low-level file handle for streamed reads/writes.
// 用於串流讀寫的底層檔案控制代碼。
class File {
public:
    File();
    ~File();

    File(const File&) = delete;
    File& operator=(const File&) = delete;
    File(File&& other) noexcept;
    File& operator=(File&& other) noexcept;

    bool Open(const char* path, FileMode mode);
    void Close();
    bool IsOpen() const;

    size_t Read(void* buffer, size_t size);
    size_t Write(const void* buffer, size_t size);

    bool Seek(int64_t offset, SeekOrigin origin);
    int64_t Tell() const;
    int64_t Size();
    bool Flush();

private:
    FILE* m_Handle;
};

// Static file/directory utilities built on std::filesystem.
// 基於 std::filesystem 的靜態檔案與目錄工具。
class FileSystem {
public:
    // Queries / 查詢
    static bool Exists(const char* path);
    static bool IsFile(const char* path);
    static bool IsDirectory(const char* path);
    static int64_t GetFileSize(const char* path);

    // Whole-file helpers / 整檔讀寫
    static bool ReadTextFile(const char* path, std::string& outContents);
    static bool ReadBinaryFile(const char* path, std::vector<uint8_t>& outData);
    static bool WriteTextFile(const char* path, const std::string& contents);
    static bool WriteBinaryFile(const char* path, const void* data, size_t size);

    // Directories / 目錄
    static bool CreateDirectories(const char* path);
    static bool Delete(const char* path);
    static bool Copy(const char* from, const char* to, bool overwrite = false);
    static std::vector<std::string> ListDirectory(const char* path);

    // Path helpers (return UTF-8) / 路徑工具（回傳 UTF-8）
    static std::string GetDirectory(const char* path);
    static std::string GetFilename(const char* path);
    static std::string GetStem(const char* path);
    static std::string GetExtension(const char* path);
    static std::string Combine(const char* lhs, const char* rhs);
    static std::string Normalize(const char* path);
};

} // namespace IO
} // namespace StonePotatoes
