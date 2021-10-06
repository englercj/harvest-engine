// Copyright Chad Engler

#include "platform_service.h"

#include "he/core/enum_ops.h"
#include "he/core/log.h"
#include "he/core/path.h"
#include "he/core/scope_guard.h"
#include "he/core/string.h"
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
            m_result = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
            if (!SUCCEEDED(m_result) && m_result != RPC_E_CHANGED_MODE)
            {
                HE_LOGF_ERROR(editor, "Failed to initialize COM. HRESULT: {:#10x}", m_result);
            }
        }

        ~ScopedCOMInit()
        {
            // Only uninitialize if successful, RPC_E_CHANGED_MODE does not refcount COM
            if (SUCCEEDED(m_result))
                ::CoUninitialize();
        }

        operator bool() const { return SUCCEEDED(m_result); }

        HRESULT m_result;
    };

    static wchar_t* MBToWCStr_Alloc(Allocator& allocator, const char* src)
    {
        const uint32_t len = MBToWCStr(nullptr, 0, src);
        HE_ASSERT(len > 0);

        wchar_t* dst = allocator.Malloc<wchar_t>(len);

        MBToWCStr(dst, len, src);
        return dst;
    }

    static void AddFiltersToDialog(IFileDialog* dialog, const FileDialogFilter* filters, uint32_t filterCount)
    {
        constexpr wchar_t Wildcard[] = L"*.*";

        if (filters == nullptr || filterCount == 0)
            return;

        Allocator& allocator = Allocator::GetTemp();

        // Add one for the wildcard we plan to add at the end
        COMDLG_FILTERSPEC* filterSpecs = allocator.Malloc<COMDLG_FILTERSPEC>(filterCount);

        uint32_t specCount = 0;

        for (uint32_t i = 0; i < filterCount; ++i)
        {
            const FileDialogFilter& filter = filters[i];

            if (String::IsEmpty(filter.name) || String::IsEmpty(filter.spec))
            {
                const char* name = filter.name ? filter.name : "";
                const char* spec = filter.spec ? filter.spec : "";
                HE_LOG_WARN(editor, HE_MSG("Invalid filter spec for file dialog. Missing name or spec."), HE_KV(name, name), HE_KV(spec, spec));
                continue;
            }

            COMDLG_FILTERSPEC& filterSpec = filterSpecs[specCount++];
            filterSpec.pszName = MBToWCStr_Alloc(allocator, filter.name);
            filterSpec.pszSpec = MBToWCStr_Alloc(allocator, filter.spec);
        }

        dialog->SetFileTypes(specCount, filterSpecs);

        // free wchar specs
        for (uint32_t i = 0; i < specCount; ++i)
        {
            COMDLG_FILTERSPEC& filterSpec = filterSpecs[i];
            allocator.Free(const_cast<wchar_t*>(filterSpec.pszName));
            allocator.Free(const_cast<wchar_t*>(filterSpec.pszSpec));
        }
        allocator.Free(filterSpecs);
    }

    static void SetDefaultPath(IFileDialog* dialog, const char* defaultPath)
    {
        if (String::IsEmpty(defaultPath))
            return;

        wchar_t* defaultPathW = HE_TO_WSTR(defaultPath);

        IShellItem* folder = nullptr;
        HRESULT result = SHCreateItemFromParsingName(defaultPathW, NULL, IID_PPV_ARGS(&folder));

        if (result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) || result == HRESULT_FROM_WIN32(ERROR_INVALID_DRIVE))
            return;

        if (!SUCCEEDED(result))
        {
            HE_LOGF_ERROR(editor, "Failed to create shell item representing default path for file dialog: {}", defaultPath);
            return;
        }

        // Could also call SetDefaultFolder(), but this guarantees defaultPath -- more consistency across API.
        dialog->SetFolder(folder);

        folder->Release();
    }

    static bool CopyShellItemName(IShellItem* shellItem, String& outPath)
    {
        wchar_t* filePathW = nullptr;
        HRESULT result = shellItem->GetDisplayName(SIGDN_FILESYSPATH, &filePathW);
        if (!SUCCEEDED(result))
        {
            HE_LOGF_ERROR(editor, "Failed to get file path for selected item from file dialog. HRESULT: {:#10x}", result);
            shellItem->Release();
            return false;
        }

        const int filePathWLen = static_cast<int>(wcslen(filePathW));
        int charsNeeded = WideCharToMultiByte(CP_UTF8, 0, filePathW, filePathWLen, nullptr, 0, nullptr, nullptr);
        ++charsNeeded;

        outPath.Resize(charsNeeded, '\0');
        int charsWritten = WideCharToMultiByte(CP_UTF8, 0, filePathW, -1, outPath.Data(), charsNeeded, nullptr, nullptr);
        HE_ASSERT(charsWritten > 0);
        HE_UNUSED(charsWritten);

        if (outPath.Back() == '\0')
            outPath.PopBack();

        CoTaskMemFree(filePathW);
        return true;
    }

    bool PlatformService::OpenFileDialog(const FileDialogConfig& config, Vector<String>& paths)
    {
        ScopedCOMInit init;
        if (!init)
            return false;

        IFileOpenDialog* dialog = nullptr;
        HRESULT result = ::CoCreateInstance(
            CLSID_FileOpenDialog,
            nullptr,
            CLSCTX_ALL,
            IID_PPV_ARGS(&dialog));

        if (!SUCCEEDED(result))
        {
            HE_LOGF_ERROR(editor, "Failed to create file dialog. HRESULT: {:#10x}", result);
            return false;
        }

        HE_AT_SCOPE_EXIT([&]() { dialog->Release(); });

        AddFiltersToDialog(dialog, config.filters, config.filterCount);
        SetDefaultPath(dialog, config.defaultPath);

        DWORD options = 0;
        if (!SUCCEEDED(dialog->GetOptions(&options)))
        {
            HE_LOGF_ERROR(editor, "Failed to get file dialog options. HRESULT: {:#10x}", result);
            return false;
        }

        if (config.allowMultiSelect)
            options |= FOS_ALLOWMULTISELECT;

        if (config.folderSelect)
            options |= FOS_PICKFOLDERS;

        if (!SUCCEEDED(dialog->SetOptions(options)))
        {
            HE_LOGF_ERROR(editor, "Failed to set file dialog options. HRESULT: {:#10x}", result);
            return false;
        }

        result = dialog->Show(nullptr);

        if (result == HRESULT_FROM_WIN32(ERROR_CANCELLED))
            return false;

        if (!SUCCEEDED(result))
        {
            HE_LOGF_ERROR(editor, "File to show file dialog. HRESULT: {:#10x}", result);
            return false;
        }

        if (config.allowMultiSelect)
        {
            IShellItemArray* shellItems = nullptr;
            result = dialog->GetResults(&shellItems);
            if (!SUCCEEDED(result))
            {
                HE_LOGF_ERROR(editor, "Failed to get shell items array from file dialog. HRESULT: {:#10x}", result);
                return false;
            }

            HE_AT_SCOPE_EXIT([&]() { shellItems->Release(); });

            DWORD itemCount = 0;
            result = shellItems->GetCount(&itemCount);
            if (!SUCCEEDED(result))
            {
                HE_LOGF_ERROR(editor, "Failed to get shell items count from file dialog. HRESULT: {:#10x}", result);
                return false;
            }

            uint32_t pathsSize = paths.Size();
            paths.Resize(pathsSize + itemCount);
            for (DWORD i = 0; i < itemCount; ++i)
            {
                IShellItem* shellItem = nullptr;
                result = shellItems->GetItemAt(i, &shellItem);
                if (!SUCCEEDED(result))
                {
                    HE_LOGF_ERROR(editor, "Failed to get shell item index {} from file dialog. HRESULT: {:#10x}", i, result);
                    return false;
                }

                HE_AT_SCOPE_EXIT([&]() { shellItem->Release(); });

                // Confirm SFGAO_FILESYSTEM is true for this shellitem, or ignore it.
                SFGAOF attribs = 0;
                result = shellItem->GetAttributes(SFGAO_FILESYSTEM, &attribs);
                if (!SUCCEEDED(result))
                {
                    HE_LOGF_ERROR(editor, "Failed to get shell item attributes from file dialog. HRESULT: {:#10x}", result);
                    return false;
                }

                if ((attribs & SFGAO_FILESYSTEM) == 0)
                    continue;

                String& path = paths[pathsSize + i];
                if (!CopyShellItemName(shellItem, path))
                    return false;
            }

            return true;
        }
        else
        {
            IShellItem* shellItem = nullptr;
            result = dialog->GetResult(&shellItem);
            if (!SUCCEEDED(result))
            {
                HE_LOGF_ERROR(editor, "Failed to get shell item from file dialog. HRESULT: {:#10x}", result);
                return false;
            }

            HE_AT_SCOPE_EXIT([&]() { shellItem->Release(); });

            String& path = paths.EmplaceBack();
            return CopyShellItemName(shellItem, path);
        }
    }

    bool PlatformService::SaveFileDialog(const FileDialogConfig& config, String& path)
    {
        ScopedCOMInit init;
        if (!init)
            return false;

        IFileSaveDialog* dialog = nullptr;
        HRESULT result = ::CoCreateInstance(
            CLSID_FileSaveDialog,
            nullptr,
            CLSCTX_ALL,
            IID_PPV_ARGS(&dialog));

        if (!SUCCEEDED(result))
        {
            HE_LOGF_ERROR(editor, "Failed to create file dialog. HRESULT: {:#10x}", result);
            return false;
        }

        HE_AT_SCOPE_EXIT([&]() { dialog->Release(); });

        AddFiltersToDialog(dialog, config.filters, config.filterCount);
        SetDefaultPath(dialog, config.defaultPath);

        result = dialog->Show(nullptr);

        if (result == HRESULT_FROM_WIN32(ERROR_CANCELLED))
            return false;

        if (!SUCCEEDED(result))
        {
            HE_LOGF_ERROR(editor, "File to show file dialog. HRESULT: {:#10x}", result);
            return false;
        }

        IShellItem* shellItem = nullptr;
        result = dialog->GetResult(&shellItem);
        if (!SUCCEEDED(result))
        {
            HE_LOGF_ERROR(editor, "Failed to get shell item from file dialog. HRESULT: {:#10x}", result);
            return false;
        }

        HE_AT_SCOPE_EXIT([&]() { shellItem->Release(); });

        return CopyShellItemName(shellItem, path);
    }

    static PIDLIST_ABSOLUTE ParseDisplayName(const char* path)
    {
        wchar_t* pathW = HE_TO_WSTR(path);

        PIDLIST_ABSOLUTE pidl = nullptr;
        SFGAOF flags = 0;
        HRESULT result = SHParseDisplayName(pathW, nullptr, &pidl, 0, &flags);
        if (!SUCCEEDED(result))
        {
            HE_LOGF_ERROR(editor, "Failed to parse path name for shell usage. HRESULT: {:#10x}, path: {}", result, path);
            return nullptr;
        }

        return pidl;
    }

    static bool ShowInExplorer(PIDLIST_ABSOLUTE pidl, PCUITEMID_CHILD_ARRAY childPidls, uint32_t childPidlCount)
    {
        HRESULT result = SHOpenFolderAndSelectItems(pidl, childPidlCount, childPidls, 0);
        if (!SUCCEEDED(result))
        {
            HE_LOGF_ERROR(editor, "Failed to open file explorer to directory. HRESULT: {:#10x}", result);
            return false;
        }

        return true;
    }

    bool PlatformService::OpenInFileExplorer(const char* directory, const char* selectItem)
    {
        PIDLIST_ABSOLUTE pidl = ParseDisplayName(directory);
        if (!pidl)
            return false;

        if (String::IsEmpty(selectItem))
            return ShowInExplorer(pidl, nullptr, 0);

        String path(Allocator::GetTemp());
        path = directory;
        ConcatPath(path, selectItem);

        PCUITEMID_CHILD childPidl = ParseDisplayName(path.Data());

        return ShowInExplorer(pidl, &childPidl, childPidl ? 1 : 0);
    }
}

#endif
