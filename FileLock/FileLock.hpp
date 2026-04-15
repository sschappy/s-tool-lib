/*
代码由Cursor生成，通过本地验证。
*/
#ifdef _WIN32
#include <Windows.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#endif

#include <string>
#include <system_error>
#include <ghc/filesystem.hpp>

struct VoxelSource::FileLock
{
    explicit FileLock(const std::string& fileName)
        : m_lockPath(fileName + ".lock")
    {
#ifdef _WIN32
        // 1) 打开或创建 .lock 文件
        const auto wpath = m_lockPath.wstring();
        HANDLE h = CreateFileW(
            wpath.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ,          // 允许其他进程读，不允许写共享
            nullptr,
            OPEN_ALWAYS,              // 不存在则创建
            FILE_ATTRIBUTE_NORMAL,
            nullptr);

        if (h == INVALID_HANDLE_VALUE)
        {
            return;
        }

        // 2) 对 .lock 文件加原生排他锁（非阻塞）
        OVERLAPPED ov = {};
        if (LockFileEx(
                h,
                LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY,
                0,
                MAXDWORD,
                MAXDWORD,
                &ov) == 0)
        {
            CloseHandle(h);
            return;
        }

        m_handle = h;
        m_locked = true;
#else
        // 1) 打开或创建 .lock 文件
        m_fd = ::open(m_lockPath.c_str(), O_CREAT | O_RDWR, 0644);
        if (m_fd < 0)
        {
            return;
        }

        // 2) 对 .lock 文件加原生排他锁（非阻塞）
        struct flock lk = {};
        lk.l_type = F_WRLCK;
        lk.l_whence = SEEK_SET;
        lk.l_start = 0;
        lk.l_len = 0; // 0 表示到 EOF（整文件）

        if (::fcntl(m_fd, F_SETLK, &lk) == -1)
        {
            ::close(m_fd);
            m_fd = -1;
            return;
        }

        m_locked = true;
#endif
    }

    ~FileLock()
    {
        release();
    }

    FileLock(const FileLock&) = delete;
    FileLock& operator=(const FileLock&) = delete;

    FileLock(FileLock&& other) noexcept
        : m_lockPath(std::move(other.m_lockPath))
        , m_locked(other.m_locked)
#ifdef _WIN32
        , m_handle(other.m_handle)
#else
        , m_fd(other.m_fd)
#endif
    {
        other.m_locked = false;
#ifdef _WIN32
        other.m_handle = nullptr;
#else
        other.m_fd = -1;
#endif
    }

    FileLock& operator=(FileLock&& other) noexcept
    {
        if (this != &other)
        {
            release();
            m_lockPath = std::move(other.m_lockPath);
            m_locked = other.m_locked;
#ifdef _WIN32
            m_handle = other.m_handle;
            other.m_handle = nullptr;
#else
            m_fd = other.m_fd;
            other.m_fd = -1;
#endif
            other.m_locked = false;
        }
        return *this;
    }

    bool isLocked() const
    {
        return m_locked;
    }

private:
    void release() noexcept
    {
        if (!m_locked)
        {
            return;
        }

#ifdef _WIN32
        if (m_handle != nullptr && m_handle != INVALID_HANDLE_VALUE)
        {
            OVERLAPPED ov = {};
            (void)UnlockFileEx(m_handle, 0, MAXDWORD, MAXDWORD, &ov);
            CloseHandle(m_handle);
            m_handle = nullptr;
        }
#else
        if (m_fd >= 0)
        {
            struct flock lk = {};
            lk.l_type = F_UNLCK;
            lk.l_whence = SEEK_SET;
            lk.l_start = 0;
            lk.l_len = 0;
            (void)::fcntl(m_fd, F_SETLK, &lk);
            ::close(m_fd);
            m_fd = -1;
        }
#endif

        // 析构时删除 .lock 文件
        std::error_code ec;
        ghc::filesystem::remove(m_lockPath, ec);

        m_locked = false;
    }

private:
    ghc::filesystem::path m_lockPath;
    bool m_locked{ false };

#ifdef _WIN32
    HANDLE m_handle{ nullptr };
#else
    int m_fd{ -1 };
#endif
};
