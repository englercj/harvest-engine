// Copyright Chad Engler

#include "he/core/directory.h"

#include "he/core/alloca.h"
#include "he/core/assert.h"
#include "he/core/memory_ops.h"
#include "he/core/path.h"
#include "he/core/scope_guard.h"
#include "he/core/utils.h"
#include "he/core/wstr.h"

#if defined(HE_PLATFORM_API_WIN32)

#include "he/core/win32_min.h"

#include <Shlobj.h>

namespace he
{
    struct DirectoryScannerImpl
    {
        bool first{ true };
        HANDLE handle{ INVALID_HANDLE_VALUE };
        WIN32_FIND_DATAW findData;
    };

    Result DirectoryScanner::Open(const char* path)
    {
        if (!HE_VERIFY(m_impl == nullptr))
            return Result::InvalidParameter;

        DirectoryScannerImpl* impl = m_allocator.New<DirectoryScannerImpl>();
        m_impl = impl;

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

    void DirectoryScanner::Close()
    {
        if (m_impl)
        {
            DirectoryScannerImpl* impl = static_cast<DirectoryScannerImpl*>(m_impl);

            if (impl->handle != INVALID_HANDLE_VALUE)
            {
                ::FindClose(impl->handle);
                impl->handle = INVALID_HANDLE_VALUE;
            }

            m_allocator.Delete(impl);
            m_impl = nullptr;
        }
    }

    bool DirectoryScanner::NextEntry(Entry& outEntry)
    {
        if (!HE_VERIFY(m_impl))
            return false;

        DirectoryScannerImpl* impl = static_cast<DirectoryScannerImpl*>(m_impl);

        outEntry.name.Clear();

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

            WCToMBStr(outEntry.name, fname);
            outEntry.isDirectory = HasFlag(impl->findData.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY);

            return true;
        }
    }

    Result Directory::GetSpecial(String& dst, SpecialDirectory dir)
    {
        wchar_t* path = nullptr;

        switch (dir)
        {
            case SpecialDirectory::LocalAppData:
                if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &path)))
                    return Result::NotSupported;
                break;
            case SpecialDirectory::SharedAppData:
                if (FAILED(SHGetKnownFolderPath(FOLDERID_ProgramData, 0, nullptr, &path)))
                    return Result::NotSupported;
                break;
            case SpecialDirectory::Documents:
                if (FAILED(SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &path)))
                    return Result::NotSupported;
                break;
            case SpecialDirectory::Temp:
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

    Result Directory::GetCurrent(String& dst)
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

    Result Directory::SetCurrent(const char* path)
    {
        if (!SetCurrentDirectoryW(HE_TO_WCSTR(path)))
            return Result::FromLastError();

        return Result::Success;
    }

    Result Directory::Rename(const char* oldPath, const char* newPath)
    {
        if (!MoveFileW(HE_TO_WCSTR(oldPath), HE_TO_WCSTR(newPath)))
            return Result::FromLastError();

        return Result::Success;
    }

    bool Directory::Exists(const char* path)
    {
        const DWORD attr = GetFileAttributesW(HE_TO_WCSTR(path));
        return attr != INVALID_FILE_ATTRIBUTES && HasFlag(attr, FILE_ATTRIBUTE_DIRECTORY);
    }

    Result Directory::Create(const char* path, bool parents)
    {
        wchar_t* widePath = HE_TO_WCSTR(path);

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

    Result Directory::Remove(const char* path)
    {
        if (!::RemoveDirectoryW(HE_TO_WCSTR(path)))
            return Result::FromLastError();

        return Result::Success;
    }
}

#endif
