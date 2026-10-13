#include "FileSystem.h"
#include "../logging/Logger.h"

#include <filesystem>
#include <system_error>

namespace StonePotatoes {
namespace IO {

namespace {

// Interpret incoming paths as UTF-8 so non-ASCII paths work on Windows.
// 將傳入路徑視為 UTF-8，讓 Windows 上的非 ASCII 路徑正常運作。
std::filesystem::path ToPath(const char* path) {
    if (!path) {
        return {};
    }
    return std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(path)));
}

const char* ToModeString(FileMode mode) {
    switch (mode) {
        case FileMode::Read:      return "rb";
        case FileMode::Write:     return "wb";
        case FileMode::Append:    return "ab";
        case FileMode::ReadWrite: return "r+b";
    }
    return "rb";
}

int ToSeekOrigin(SeekOrigin origin) {
    switch (origin) {
        case SeekOrigin::Begin:   return SEEK_SET;
        case SeekOrigin::Current: return SEEK_CUR;
        case SeekOrigin::End:     return SEEK_END;
    }
    return SEEK_SET;
}

} // namespace

// ---------------------------------------------------------------------------
// File
// ---------------------------------------------------------------------------

File::File()
    : m_Handle(nullptr) {
}

File::~File() {
    Close();
}

File::File(File&& other) noexcept
    : m_Handle(other.m_Handle) {
    other.m_Handle = nullptr;
}

File& File::operator=(File&& other) noexcept {
    if (this != &other) {
        Close();
        m_Handle = other.m_Handle;
        other.m_Handle = nullptr;
    }
    return *this;
}

bool File::Open(const char* path, FileMode mode) {
    Close();
    if (!path) {
        return false;
    }

#if defined(_WIN32)
    // fopen_s avoids deprecation warnings; _wfopen_s keeps UTF-8 paths working.
    const wchar_t* wmode = L"rb";
    switch (mode) {
        case FileMode::Read:      wmode = L"rb";  break;
        case FileMode::Write:     wmode = L"wb";  break;
        case FileMode::Append:    wmode = L"ab";  break;
        case FileMode::ReadWrite: wmode = L"r+b"; break;
    }
    const std::filesystem::path fsPath = ToPath(path);
    if (_wfopen_s(&m_Handle, fsPath.c_str(), wmode) != 0) {
        m_Handle = nullptr;
    }
#else
    m_Handle = std::fopen(path, ToModeString(mode));
#endif

    if (!m_Handle) {
        STONEPOTATOES_LOG_ERROR("File::Open failed: %s", path);
        return false;
    }
    return true;
}

void File::Close() {
    if (m_Handle) {
        std::fclose(m_Handle);
        m_Handle = nullptr;
    }
}

bool File::IsOpen() const {
    return m_Handle != nullptr;
}

size_t File::Read(void* buffer, size_t size) {
    if (!m_Handle || !buffer || size == 0) {
        return 0;
    }
    return std::fread(buffer, 1, size, m_Handle);
}

size_t File::Write(const void* buffer, size_t size) {
    if (!m_Handle || !buffer || size == 0) {
        return 0;
    }
    return std::fwrite(buffer, 1, size, m_Handle);
}

bool File::Seek(int64_t offset, SeekOrigin origin) {
    if (!m_Handle) {
        return false;
    }
#if defined(_WIN32)
    return _fseeki64(m_Handle, offset, ToSeekOrigin(origin)) == 0;
#else
    return std::fseek(m_Handle, static_cast<long>(offset), ToSeekOrigin(origin)) == 0;
#endif
}

int64_t File::Tell() const {
    if (!m_Handle) {
        return -1;
    }
#if defined(_WIN32)
    return _ftelli64(m_Handle);
#else
    return static_cast<int64_t>(std::ftell(m_Handle));
#endif
}

int64_t File::Size() {
    if (!m_Handle) {
        return -1;
    }
    const int64_t current = Tell();
    if (current < 0 || !Seek(0, SeekOrigin::End)) {
        return -1;
    }
    const int64_t end = Tell();
    Seek(current, SeekOrigin::Begin);
    return end;
}

bool File::Flush() {
    return m_Handle && std::fflush(m_Handle) == 0;
}

// ---------------------------------------------------------------------------
// FileSystem
// ---------------------------------------------------------------------------

bool FileSystem::Exists(const char* path) {
    std::error_code ec;
    return std::filesystem::exists(ToPath(path), ec);
}

bool FileSystem::IsFile(const char* path) {
    std::error_code ec;
    return std::filesystem::is_regular_file(ToPath(path), ec);
}

bool FileSystem::IsDirectory(const char* path) {
    std::error_code ec;
    return std::filesystem::is_directory(ToPath(path), ec);
}

int64_t FileSystem::GetFileSize(const char* path) {
    std::error_code ec;
    const auto size = std::filesystem::file_size(ToPath(path), ec);
    return ec ? -1 : static_cast<int64_t>(size);
}

bool FileSystem::ReadTextFile(const char* path, std::string& outContents) {
    File file;
    if (!file.Open(path, FileMode::Read)) {
        return false;
    }
    const int64_t size = file.Size();
    if (size < 0) {
        return false;
    }
    outContents.resize(static_cast<size_t>(size));
    const size_t read = file.Read(outContents.data(), outContents.size());
    outContents.resize(read);
    return true;
}

bool FileSystem::ReadBinaryFile(const char* path, std::vector<uint8_t>& outData) {
    File file;
    if (!file.Open(path, FileMode::Read)) {
        return false;
    }
    const int64_t size = file.Size();
    if (size < 0) {
        return false;
    }
    outData.resize(static_cast<size_t>(size));
    const size_t read = file.Read(outData.data(), outData.size());
    outData.resize(read);
    return true;
}

bool FileSystem::WriteTextFile(const char* path, const std::string& contents) {
    return WriteBinaryFile(path, contents.data(), contents.size());
}

bool FileSystem::WriteBinaryFile(const char* path, const void* data, size_t size) {
    File file;
    if (!file.Open(path, FileMode::Write)) {
        return false;
    }
    return file.Write(data, size) == size;
}

bool FileSystem::CreateDirectories(const char* path) {
    std::error_code ec;
    if (std::filesystem::exists(ToPath(path), ec)) {
        return IsDirectory(path);
    }
    return std::filesystem::create_directories(ToPath(path), ec);
}

bool FileSystem::Delete(const char* path) {
    std::error_code ec;
    return std::filesystem::remove_all(ToPath(path), ec) > 0 || !std::filesystem::exists(ToPath(path), ec);
}

bool FileSystem::Copy(const char* from, const char* to, bool overwrite) {
    std::error_code ec;
    const auto options = overwrite
        ? std::filesystem::copy_options::overwrite_existing
        : std::filesystem::copy_options::none;
    return std::filesystem::copy_file(ToPath(from), ToPath(to), options, ec);
}

std::vector<std::string> FileSystem::ListDirectory(const char* path) {
    std::vector<std::string> entries;
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(ToPath(path), ec)) {
        const auto u8 = entry.path().filename().u8string();
        entries.emplace_back(reinterpret_cast<const char*>(u8.c_str()));
    }
    if (ec) {
        STONEPOTATOES_LOG_ERROR("FileSystem::ListDirectory failed: %s", path ? path : "(null)");
    }
    return entries;
}

std::string FileSystem::GetDirectory(const char* path) {
    const auto parent = ToPath(path).parent_path();
    const auto u8 = parent.u8string();
    return std::string(reinterpret_cast<const char*>(u8.c_str()));
}

std::string FileSystem::GetFilename(const char* path) {
    const auto u8 = ToPath(path).filename().u8string();
    return std::string(reinterpret_cast<const char*>(u8.c_str()));
}

std::string FileSystem::GetStem(const char* path) {
    const auto u8 = ToPath(path).stem().u8string();
    return std::string(reinterpret_cast<const char*>(u8.c_str()));
}

std::string FileSystem::GetExtension(const char* path) {
    const auto u8 = ToPath(path).extension().u8string();
    return std::string(reinterpret_cast<const char*>(u8.c_str()));
}

std::string FileSystem::Combine(const char* lhs, const char* rhs) {
    const auto u8 = (ToPath(lhs) / ToPath(rhs)).u8string();
    return std::string(reinterpret_cast<const char*>(u8.c_str()));
}

std::string FileSystem::Normalize(const char* path) {
    const auto u8 = ToPath(path).lexically_normal().u8string();
    return std::string(reinterpret_cast<const char*>(u8.c_str()));
}

} // namespace IO
} // namespace StonePotatoes
