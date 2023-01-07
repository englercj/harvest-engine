// Copyright Chad Engler

#pragma once

#include "he/core/types.h"

#if defined(HE_PLATFORM_API_WIN32)

#include <oleidl.h>

namespace he::window::win32
{
    class ViewImpl;

    class DropTarget : public IDropTarget
    {
    public:
        DropTarget(ViewImpl* view) noexcept;

    public: // IUnkown interface
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID* ppvObject) override;
        ULONG STDMETHODCALLTYPE AddRef() override;
        ULONG STDMETHODCALLTYPE Release() override;

    public: // IDropTarget interface
        HRESULT STDMETHODCALLTYPE DragEnter(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override;
        HRESULT STDMETHODCALLTYPE DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override;
        HRESULT STDMETHODCALLTYPE DragLeave() override;
        HRESULT STDMETHODCALLTYPE Drop(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override;

    private:
        LONG m_refCount{ 1 };
        ViewImpl* m_view{ nullptr };
    };
}

#endif
