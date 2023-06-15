// Copyright Chad Engler

#include "he/editor/services/platform_service.h"

#include "he/core/enum_ops.h"
#include "he/core/log.h"
#include "he/core/path.h"
#include "he/core/result.h"
#include "he/core/result_fmt.h"
#include "he/core/scope_guard.h"
#include "he/core/string.h"
#include "he/core/string_ops.h"
#include "he/core/wstr.h"

#if defined(HE_PLATFORM_API_WIN32)

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
    #define NOMINMAX
#endif

#include <Objbase.h>
#include <Shlobj.h>
#include <Shobjidl.h>
#include <Windows.h>

namespace he::editor
{
    class ScopedCOMInit
    {
    public:
        ScopedCOMInit()
        {
            m_hr= ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
            if (!SUCCEEDED(m_hr) && m_hr != RPC_E_CHANGED_MODE)
            {
                HE_LOG_ERROR(editor,
                    HE_MSG("Failed to initialize COM for platform service."),
                    HE_KV(result, Win32Result(m_hr)),
                    HE_KV(hresult, m_hr));
            }
        }

        ~ScopedCOMInit()
        {
            // Only uninitialize if successful, RPC_E_CHANGED_MODE does not refcount COM
            if (SUCCEEDED(m_hr))
                ::CoUninitialize();
        }

        [[nodiscard]] explicit operator bool() const { return SUCCEEDED(m_hr); }

        HRESULT m_hr;
    };

    static wchar_t* MBToWCStr_Alloc(const char* src)
    {
        const uint32_t len = MBToWCStr(nullptr, 0, src);
        HE_ASSERT(len > 0);

        wchar_t* dst = Allocator::GetDefault().Malloc<wchar_t>(len);

        MBToWCStr(dst, len, src);
        return dst;
    }

    static void AddFiltersToDialog(IFileDialog* dialog, const FileDialogFilter* filters, uint32_t filterCount)
    {
        constexpr wchar_t Wildcard[] = L"*.*";

        if (filters == nullptr || filterCount == 0)
            return;

        // Add one for the wildcard we plan to add at the end
        COMDLG_FILTERSPEC* filterSpecs = Allocator::GetDefault().Malloc<COMDLG_FILTERSPEC>(filterCount);

        uint32_t specCount = 0;

        for (uint32_t i = 0; i < filterCount; ++i)
        {
            const FileDialogFilter& filter = filters[i];

            if (StrEmpty(filter.name) || StrEmpty(filter.spec))
            {
                HE_LOG_WARN(editor,
                    HE_MSG("Invalid filter spec for file dialog. Missing name or spec."),
                    HE_KV(name, filter.name ? filter.name : ""),
                    HE_KV(spec, filter.spec ? filter.spec : ""));
                continue;
            }

            COMDLG_FILTERSPEC& filterSpec = filterSpecs[specCount++];
            filterSpec.pszName = MBToWCStr_Alloc(filter.name);
            filterSpec.pszSpec = MBToWCStr_Alloc(filter.spec);
        }

        dialog->SetFileTypes(specCount, filterSpecs);

        // free wchar specs
        for (uint32_t i = 0; i < specCount; ++i)
        {
            COMDLG_FILTERSPEC& filterSpec = filterSpecs[i];
            Allocator::GetDefault().Free(const_cast<wchar_t*>(filterSpec.pszName));
            Allocator::GetDefault().Free(const_cast<wchar_t*>(filterSpec.pszSpec));
        }
        Allocator::GetDefault().Free(filterSpecs);
    }

    static void SetDefaultPath(IFileDialog* dialog, const char* defaultPath)
    {
        if (StrEmpty(defaultPath))
            return;

        wchar_t* defaultPathW = HE_TO_WCSTR(defaultPath);

        IShellItem* folder = nullptr;
        HRESULT hr = ::SHCreateItemFromParsingName(defaultPathW, NULL, IID_PPV_ARGS(&folder));

        HE_AT_SCOPE_EXIT([&]() { if (folder) folder->Release(); });

        if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) || hr == HRESULT_FROM_WIN32(ERROR_INVALID_DRIVE))
        {
            HE_LOG_WARN(editor,
                HE_MSG("Default path for file dialog does not exist."),
                HE_KV(path, defaultPath),
                HE_KV(result, Win32Result(hr)),
                HE_KV(hresult, hr));
            return;
        }

        if (FAILED(hr))
        {
            HE_LOG_ERROR(editor,
                HE_MSG("Failed to create shell item representing default path for file dialog."),
                HE_KV(path, defaultPath),
                HE_KV(result, Win32Result(hr)),
                HE_KV(hresult, hr));
            return;
        }

        // Could also call SetDefaultFolder(), but this guarantees defaultPath -- more consistency across API.
        dialog->SetFolder(folder);
    }

    static bool CopyShellItemName(IShellItem* shellItem, String& outPath)
    {
        wchar_t* filePathW = nullptr;
        HRESULT hr = shellItem->GetDisplayName(SIGDN_FILESYSPATH, &filePathW);

        HE_AT_SCOPE_EXIT([&]() { ::CoTaskMemFree(filePathW); });

        if (FAILED(hr))
        {
            HE_LOG_ERROR(editor,
                HE_MSG("Failed to get file path for selected item from file dialog."),
                HE_KV(result, Win32Result(hr)),
                HE_KV(hresult, hr));
            return false;
        }

        WCToMBStr(outPath, filePathW);
        return true;
    }

    static PIDLIST_ABSOLUTE ParseDisplayName(const char* path)
    {
        wchar_t* pathW = HE_TO_WCSTR(path);

        PIDLIST_ABSOLUTE pidl = nullptr;
        SFGAOF flags = 0;
        HRESULT hr = ::SHParseDisplayName(pathW, nullptr, &pidl, 0, &flags);
        if (FAILED(hr))
        {
            HE_LOG_ERROR(editor,
                HE_MSG("Failed to parse path name for shell usage."),
                HE_KV(path, path),
                HE_KV(result, Win32Result(hr)),
                HE_KV(hresult, hr));
            return nullptr;
        }

        return pidl;
    }

    static bool ShowInExplorer(PIDLIST_ABSOLUTE pidl, PCUITEMID_CHILD_ARRAY childPidls, uint32_t childPidlCount)
    {
        HRESULT hr = ::SHOpenFolderAndSelectItems(pidl, childPidlCount, childPidls, 0);
        if (FAILED(hr))
        {
            HE_LOG_ERROR(editor,
                HE_MSG("Failed to open file explorer to directory."),
                HE_KV(result, Win32Result(hr)),
                HE_KV(hresult, hr));
            return false;
        }

        return true;
    }

    static bool CreateOpenFileDialog(const FileDialogConfig& config, DWORD flags, IFileOpenDialog*& dialog)
    {
        HRESULT hr = ::CoCreateInstance(
            CLSID_FileOpenDialog,
            nullptr,
            CLSCTX_ALL,
            IID_PPV_ARGS(&dialog));

        auto guard = MakeScopeGuard([&]() { if (dialog) dialog->Release(); });

        if (FAILED(hr))
        {
            HE_LOG_ERROR(editor,
                HE_MSG("Failed to create file dialog."),
                HE_KV(result, Win32Result(hr)),
                HE_KV(hresult, hr));
            return false;
        }

        AddFiltersToDialog(dialog, config.filters, config.filterCount);
        SetDefaultPath(dialog, config.defaultPath);

        DWORD options = 0;
        hr = dialog->GetOptions(&options);
        if (FAILED(hr))
        {
            HE_LOG_ERROR(editor,
                HE_MSG("Failed to get file dialog options."),
                HE_KV(result, Win32Result(hr)),
                HE_KV(hresult, hr));
            return false;
        }

        options |= flags | FOS_FORCEFILESYSTEM;

        hr = dialog->SetOptions(options);
        if (FAILED(hr))
        {
            HE_LOG_ERROR(editor,
                HE_MSG("Failed to set file dialog options."),
                HE_KV(result, Win32Result(hr)),
                HE_KV(hresult, hr));
            return false;
        }

        hr = dialog->Show(nullptr);

        if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED))
            return false;

        if (FAILED(hr))
        {
            HE_LOG_ERROR(editor,
                HE_MSG("File to show file dialog."),
                HE_KV(result, Win32Result(hr)),
                HE_KV(hresult, hr));
            return false;
        }

        guard.Dismiss();
        return true;
    }

    bool PlatformService::OpenFilesDialog(Vector<String>& paths, const FileDialogConfig& config)
    {
        ScopedCOMInit init;
        if (!init)
            return false;

        IFileOpenDialog* dialog = nullptr;
        if (!CreateOpenFileDialog(config, FOS_ALLOWMULTISELECT, dialog))
            return false;

        HE_AT_SCOPE_EXIT([&]() { if (dialog) dialog->Release(); });

        IShellItemArray* shellItems = nullptr;
        HRESULT hr = dialog->GetResults(&shellItems);

        HE_AT_SCOPE_EXIT([&]() { if (shellItems) shellItems->Release(); });

        if (FAILED(hr))
        {
            HE_LOG_ERROR(editor,
                HE_MSG("Failed to get shell items array from file dialog."),
                HE_KV(result, Win32Result(hr)),
                HE_KV(hresult, hr));
            return false;
        }

        DWORD itemCount = 0;
        hr = shellItems->GetCount(&itemCount);
        if (FAILED(hr))
        {
            HE_LOG_ERROR(editor,
                HE_MSG("Failed to get shell items count from file dialog."),
                HE_KV(result, Win32Result(hr)),
                HE_KV(hresult, hr));
            return false;
        }

        uint32_t pathsSize = paths.Size();
        paths.Resize(pathsSize + itemCount);
        for (DWORD i = 0; i < itemCount; ++i)
        {
            IShellItem* shellItem = nullptr;
            hr = shellItems->GetItemAt(i, &shellItem);

            HE_AT_SCOPE_EXIT([&]() { if (shellItem) shellItem->Release(); });

            if (FAILED(hr))
            {
                HE_LOG_ERROR(editor,
                    HE_MSG("Failed to get shell item from file dialog."),
                    HE_KV(item_index, i),
                    HE_KV(result, Win32Result(hr)),
                    HE_KV(hresult, hr));
                return false;
            }

            String& path = paths[pathsSize + i];
            if (!CopyShellItemName(shellItem, path))
                return false;
        }

        return true;
    }

    bool PlatformService::OpenFileDialog(String& path, const FileDialogConfig& config)
    {
        ScopedCOMInit init;
        if (!init)
            return false;

        IFileOpenDialog* dialog = nullptr;
        if (!CreateOpenFileDialog(config, 0, dialog))
            return false;

        HE_AT_SCOPE_EXIT([&]() { if (dialog) dialog->Release(); });

        IShellItem* shellItem = nullptr;
        HRESULT hr = dialog->GetResult(&shellItem);

        HE_AT_SCOPE_EXIT([&]() { if (shellItem) shellItem->Release(); });

        if (FAILED(hr))
        {
            HE_LOG_ERROR(editor,
                HE_MSG("Failed to get shell item from file dialog."),
                HE_KV(result, Win32Result(hr)),
                HE_KV(hresult, hr));
            return false;
        }

        return CopyShellItemName(shellItem, path);
    }

    bool PlatformService::OpenFolderDialog(String& path, const FileDialogConfig& config)
    {
        ScopedCOMInit init;
        if (!init)
            return false;

        IFileOpenDialog* dialog = nullptr;
        if (!CreateOpenFileDialog(config, FOS_PICKFOLDERS, dialog))
            return false;

        HE_AT_SCOPE_EXIT([&]() { if (dialog) dialog->Release(); });

        IShellItem* shellItem = nullptr;
        HRESULT hr = dialog->GetResult(&shellItem);

        HE_AT_SCOPE_EXIT([&]() { if (shellItem) shellItem->Release(); });

        if (FAILED(hr))
        {
            HE_LOG_ERROR(editor,
                HE_MSG("Failed to get shell item from file dialog."),
                HE_KV(result, Win32Result(hr)),
                HE_KV(hresult, hr));
            return false;
        }

        return CopyShellItemName(shellItem, path);
    }

    bool PlatformService::SaveFileDialog(String& path, const FileDialogConfig& config)
    {
        ScopedCOMInit init;
        if (!init)
            return false;

        IFileSaveDialog* dialog = nullptr;
        HRESULT hr = ::CoCreateInstance(
            CLSID_FileSaveDialog,
            nullptr,
            CLSCTX_ALL,
            IID_PPV_ARGS(&dialog));

        HE_AT_SCOPE_EXIT([&]() { if (dialog) dialog->Release(); });

        if (FAILED(hr))
        {
            HE_LOG_ERROR(editor,
                HE_MSG("Failed to create file dialog."),
                HE_KV(result, Win32Result(hr)),
                HE_KV(hresult, hr));
            return false;
        }

        AddFiltersToDialog(dialog, config.filters, config.filterCount);
        SetDefaultPath(dialog, config.defaultPath);

        hr = dialog->Show(nullptr);

        if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED))
            return false;

        if (FAILED(hr))
        {
            HE_LOG_ERROR(editor,
                HE_MSG("File to show file dialog."),
                HE_KV(result, Win32Result(hr)),
                HE_KV(hresult, hr));
            return false;
        }

        IShellItem* shellItem = nullptr;
        hr = dialog->GetResult(&shellItem);

        HE_AT_SCOPE_EXIT([&]() { if (shellItem) shellItem->Release(); });

        if (FAILED(hr))
        {
            HE_LOG_ERROR(editor,
                HE_MSG("Failed to get shell item from file dialog."),
                HE_KV(result, Win32Result(hr)),
                HE_KV(hresult, hr));
            return false;
        }

        const bool result = CopyShellItemName(shellItem, path);
        return result;
    }

    bool PlatformService::OpenInFileExplorer(const char* directory, const char* selectItem)
    {
        PIDLIST_ABSOLUTE pidl = ParseDisplayName(directory);
        if (!pidl)
            return false;

        if (StrEmpty(selectItem))
            return ShowInExplorer(pidl, nullptr, 0);

        String path;
        path = directory;
        ConcatPath(path, selectItem);

        PCUITEMID_CHILD childPidl = ParseDisplayName(path.Data());
        if (!childPidl)
            return false;

        return ShowInExplorer(pidl, &childPidl, childPidl ? 1 : 0);
    }
}

#endif
