// Copyright Chad Engler

#include "he/core/directory.h"

#include "he/core/alloca.h"
#include "he/core/assert.h"
#include "he/core/memory_ops.h"
#include "he/core/path.h"
#include "he/core/scope_guard.h"
#include "he/core/wstr.h"

#if defined(HE_PLATFORM_API_WIN32)

#include "he/core/win32_min.h"

#include <Shlobj.h>

namespace he::Directory
{
    struct ScannerImpl
    {
        bool first{ true };
        HANDLE handle{ INVALID_HANDLE_VALUE };
        WIN32_FIND_DATAW findData;
    };

    Scanner::Scanner(Allocator& allocator)
        : m_allocator(allocator)
        , m_impl(allocator.New<ScannerImpl>())
    {}

    Scanner::~Scanner()
    {
        Close();

        if (m_impl)
        {
            m_allocator.Delete(static_cast<ScannerImpl*>(m_impl));
        }
    }

    Result Scanner::Open(const char* path)
    {
        ScannerImpl* impl = static_cast<ScannerImpl*>(m_impl);
        HE_ASSERT(impl);
        HE_ASSERT(impl->handle == INVALID_HANDLE_VALUE);

        constexpr const wchar_t PatternSuffix[] = L"/*.*";

        int32_t requiredLen = ::MultiByteToWideChar(CP_UTF8, 0, path, -1, nullptr, 0);
        if (requiredLen <= 0)
            return Result::FromLastError();

        requiredLen += HE_LENGTH_OF(PatternSuffix);
        wchar_t* pattern = m_allocator.Malloc<wchar_t>(requiredLen);
        HE_AT_SCOPE_EXIT([&]()
        {
            m_allocator.Free(pattern);
        });

        const int32_t len = ::MultiByteToWideChar(CP_UTF8, 0, path, -1, pattern, requiredLen);
        if (len <= 0)
            return Result::FromLastError();

        MemCopy(pattern + (len - 1), PatternSuffix, sizeof(wchar_t) * HE_LENGTH_OF(PatternSuffix));

        impl->first = true;
        impl->handle = ::FindFirstFileW(pattern, &impl->findData);
        if (impl->handle == INVALID_HANDLE_VALUE)
            return Result::FromLastError();

        return Result::Success;
    }

    void Scanner::Close()
    {
        ScannerImpl* impl = static_cast<ScannerImpl*>(m_impl);
        HE_ASSERT(impl);

        if (impl->handle != INVALID_HANDLE_VALUE)
        {
            ::FindClose(impl->handle);
            impl->handle = INVALID_HANDLE_VALUE;
        }
    }

    bool Scanner::NextEntry(String& outName, bool* outIsDirectory)
    {
        ScannerImpl* impl = static_cast<ScannerImpl*>(m_impl);
        HE_ASSERT(impl);

        while (true)
        {
            if (!impl->first)
            {
                if (!::FindNextFileW(impl->handle, &impl->findData))
                {
                    HE_ASSERT(::GetLastError() == ERROR_NO_MORE_FILES);
                    return false;
                }
            }
            impl->first = false;

            const wchar_t* fname = impl->findData.cFileName;

            // If its "." or ".." then skip it
            if (fname[0] == L'.' && (fname[1] == L'\0' || (fname[1] == L'.' && fname[2] == L'\0')))
                continue;

            WCToMBStr(outName, fname);

            if (outIsDirectory)
                *outIsDirectory = !!(impl->findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);

            return true;
        }
    }

    Result GetSpecial(String& dst, SpecialId dir)
    {
        wchar_t* path = nullptr;

        switch (dir)
        {
            case SpecialId::LocalAppData:
                if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &path)))
                    return Result::NotSupported;
                break;
            case SpecialId::SharedAppData:
                if (FAILED(SHGetKnownFolderPath(FOLDERID_ProgramData, 0, nullptr, &path)))
                    return Result::NotSupported;
                break;
            case SpecialId::Documents:
                if (FAILED(SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &path)))
                    return Result::NotSupported;
                break;
            case SpecialId::Temp:
            {
                // The maximum possible path size is MAX_PATH+1 (261).
                // See: https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-gettemppathw
                path = HE_ALLOCA(wchar_t, MAX_PATH + 2);
                const DWORD pathLen = GetTempPathW(MAX_PATH + 2, path);
                if (pathLen == 0)
                    return Result::FromLastError();
            }
        }

        HE_ASSERT(path);
        WCToMBStr(dst, path);
        return Result::Success;
    }

    Result GetCurrent(String& dst)
    {
        const DWORD requiredLen = GetCurrentDirectoryW(0, nullptr);
        if (requiredLen == 0)
            return Result::FromLastError();

        wchar_t* path = HE_ALLOCA(wchar_t, requiredLen);
        const DWORD pathLen = GetCurrentDirectoryW(requiredLen, path);
        if (pathLen == 0)
            return Result::FromLastError();

        WCToMBStr(dst, path);
        return Result::Success;
    }

    Result SetCurrent(const char* path)
    {
        if (!SetCurrentDirectoryW(HE_TO_WSTR(path)))
            return Result::FromLastError();

        return Result::Success;
    }

    Result Rename(const char* oldPath, const char* newPath)
    {
        if (!MoveFileW(HE_TO_WSTR(oldPath), HE_TO_WSTR(newPath)))
            return Result::FromLastError();

        return Result::Success;
    }

    bool Exists(const char* path)
    {
        const DWORD attr = GetFileAttributesW(HE_TO_WSTR(path));
        return attr != INVALID_FILE_ATTRIBUTES && !!(attr & FILE_ATTRIBUTE_DIRECTORY);
    }

    Result Create(const char* path, bool parents)
    {
        wchar_t* widePath = HE_TO_WSTR(path);

        // Create the parent directories along the path
        if (parents)
        {
            wchar_t* start = IsAbsolutePath(path) ? widePath + 3 : widePath;
            const bool isUNC = String::EqualN(path, "\\\\", 2) || String::EqualN(path, "//", 2);
            bool first = true;

            for (wchar_t* p = start; *p; ++p)
            {
                if ((*p == L'/' || *p == L'\\') && p[1] && p[1] != '/' && p[1] != '\\')
                {
                    // Skip the first component in UNC paths because that's the server
                    if (!first || !isUNC)
                    {
                        // Truncate the path at the separator, create the parent directory, then restore the separator
                        wchar_t sep = *p;
                        *p = L'\0';
                        if (!::CreateDirectoryW(widePath, nullptr) && ::GetLastError() != ERROR_ALREADY_EXISTS)
                            return Result::FromLastError();
                        *p = sep;
                    }
                    first = false;
                }
            }
        }

        if (!::CreateDirectoryW(widePath, nullptr) && ::GetLastError() != ERROR_ALREADY_EXISTS)
            return Result::FromLastError();

        return Result::Success;
    }

    Result Remove(const char* path)
    {
        if (!::RemoveDirectoryW(HE_TO_WSTR(path)))
            return Result::FromLastError();

        return Result::Success;
    }
}

#endif
