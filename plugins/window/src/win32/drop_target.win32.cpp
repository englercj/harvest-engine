// Copyright Chad Engler

#include "drop_target.win32.h"

#include "device.win32.h"
#include "view.win32.h"

#include "he/window/application.h"
#include "he/window/event.h"
#include "he/core/string.h"
#include "he/core/vector.h"
#include "he/core/wstr.h"

#if defined(HE_PLATFORM_API_WIN32)

#include <oleidl.h>

namespace he::window::win32
{
    DropTarget::DropTarget(ViewImpl* view) noexcept
        : m_view(view)
    {}

    HRESULT DropTarget::QueryInterface(REFIID riid, LPVOID* ppvObject)
    {
        if (!ppvObject)
            return E_INVALIDARG;

        if (riid == IID_IUnknown || riid == IID_IDropTarget)
        {
            *ppvObject = this;
            AddRef();
            return S_OK;
        }

        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    ULONG DropTarget::AddRef()
    {
        return ::InterlockedIncrement(&m_refCount);
    }

    ULONG DropTarget::Release()
    {
        const ULONG count = ::InterlockedDecrement(&m_refCount);
        if (count == 0)
        {
            delete this;
        }
        return count;
    }

    HRESULT DropTarget::DragEnter(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
    {
        HE_UNUSED(pDataObj, grfKeyState, pt);

        if (!pdwEffect)
            return E_INVALIDARG;

        ViewDndStartEvent ev(m_view);
        m_view->m_device->m_app->OnEvent(ev);

        *pdwEffect &= DROPEFFECT_COPY;
        return S_OK;
    }

    HRESULT DropTarget::DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
    {
        HE_UNUSED(grfKeyState);

        ViewDndMoveEvent ev(m_view);
        ev.pos = { static_cast<float>(pt.x), static_cast<float>(pt.y) };
        m_view->m_device->m_app->OnEvent(ev);

        *pdwEffect &= DROPEFFECT_COPY;
        return S_OK;
    }

    HRESULT DropTarget::DragLeave()
    {
        ViewDndEndEvent ev(m_view);
        m_view->m_device->m_app->OnEvent(ev);
        return S_OK;
    }

    HRESULT DropTarget::Drop(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
    {
        HE_UNUSED(grfKeyState, pt);

        FORMATETC fmte{ CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
        STGMEDIUM stgm;

        if (SUCCEEDED(pDataObj->GetData(&fmte, &stgm)))
        {
            const HDROP drop = reinterpret_cast<HDROP>(stgm.hGlobal);
            const UINT count = ::DragQueryFileW(drop, 0xffffffff, nullptr, 0);

            Vector<wchar_t> buf{};
            String path{};

            for (UINT i = 0; i < count; i++)
            {
                const UINT length = ::DragQueryFileW(drop, i, NULL, 0);
                buf.Resize(length + 1);

                if (::DragQueryFileW(drop, i, buf.Data(), buf.Size()))
                {
                    WCToMBStr(path, buf.Data());

                    ViewDndDropEvent ev(m_view);
                    ev.path = path;
                    m_view->m_device->m_app->OnEvent(ev);
                }
            }

            ::ReleaseStgMedium(&stgm);

            ViewDndEndEvent ev(m_view);
            m_view->m_device->m_app->OnEvent(ev);
        }

        *pdwEffect &= DROPEFFECT_COPY;
        return S_OK;
    }
}

#endif
