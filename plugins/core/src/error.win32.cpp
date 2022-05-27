// Copyright Chad Engler

#include "he/core/error.h"

#include "he/core/alloca.h"
#include "he/core/appender.h"
#include "he/core/buffer_writer.h"
#include "he/core/debugger.h"
#include "he/core/key_value_fmt.h"
#include "he/core/span.h"
#include "he/core/string.h"
#include "he/core/string_view.h"
#include "he/core/types.h"
#include "he/core/utils.h"
#include "he/core/vector.h"
#include "he/core/wstr.h"

#include "fmt/format.h"

#if defined(HE_PLATFORM_API_WIN32)

#if !defined(NOMINMAX)
    #define NOMINMAX
#endif

#include <Windows.h>

#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

namespace he
{
    class DialogTemplate
    {
    private:
        static constexpr uint32_t ItemsCountOffset = 16;

    public:
        DialogTemplate(LPCWSTR caption, DWORD style, short x, short y, short w, short h)
        {
            m_buf.Reserve(1024);

            // Collect font information from the system with some reasonable default if it fails
            WORD fontPoint = 8;
            WORD fontWeight = 8;
            BYTE fontItalic = 0;
            BYTE fontCharset = DEFAULT_CHARSET;
            LPCWSTR fontName = L"Tahoma";

            NONCLIENTMETRICSW ncm = { sizeof(NONCLIENTMETRICSW) };
            if (::SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0))
            {
                fontWeight = static_cast<WORD>(ncm.lfMessageFont.lfWeight);
                fontItalic = ncm.lfMessageFont.lfItalic;
                fontCharset = ncm.lfMessageFont.lfCharSet;
                fontName = ncm.lfMessageFont.lfFaceName;

                if (ncm.lfMessageFont.lfHeight < 0)
                {
                    HDC hdc = ::GetDC(NULL);
                    if (hdc)
                    {
                        int dpi = ::GetDeviceCaps(hdc, LOGPIXELSY);
                        fontPoint = static_cast<WORD>(-::MulDiv(ncm.lfMessageFont.lfHeight, 72, dpi));
                        ::ReleaseDC(NULL, hdc);
                    }
                    else
                    {
                        // don't set fontPoint so it uses the default value
                    }
                }
                else
                {
                    fontPoint = static_cast<WORD>(ncm.lfMessageFont.lfHeight);
                }
            }

            // Write the extended dialog header
            // https://docs.microsoft.com/en-us/windows/win32/dlgbox/dlgtemplateex
            m_buf.Write<WORD>(1); // dlgVer
            m_buf.Write<WORD>(0xFFFF); // signature
            m_buf.Write<DWORD>(0); // helpID
            m_buf.Write<DWORD>(0); // exStyle
            m_buf.Write<DWORD>(style); // style
            m_buf.Write<WORD>(0); // cDlgItems
            m_buf.Write<short>(y); // x
            m_buf.Write<short>(x); // y
            m_buf.Write<short>(w); // cx
            m_buf.Write<short>(h); // cy
            m_buf.Write<WCHAR>(0); // menu
            m_buf.Write<WCHAR>(0); // windowClass
            WriteString(caption);

            // Write the font information
            m_buf.Write<WORD>(fontPoint); // pointsize
            m_buf.Write<WORD>(fontWeight); // weight
            m_buf.Write<BYTE>(fontItalic); // italic
            m_buf.Write<BYTE>(fontCharset); // charset
            WriteString(fontName); // typeface
        }

        void AddButton(LPCWSTR caption, DWORD style, DWORD exStyle, short x, short y, short w, short h, WORD id)
        {
            AddStandardItem(0x0080, style, exStyle, x, y, w, h, id);
            WriteString(caption); // title
            m_buf.Write<WORD>(0); // extraCount
        }

        void AddEditBox(LPCWSTR caption, DWORD style, DWORD exStyle, short x, short y, short w, short h, WORD id)
        {
            AddStandardItem(0x0081, style, exStyle, x, y, w, h, id);
            WriteString(caption); // title
            m_buf.Write<WORD>(0); // extraCount
        }

        void AddStaticText(LPCWSTR caption, DWORD style, DWORD exStyle, short x, short y, short w, short h, WORD id)
        {
            AddStandardItem(0x0082, style, exStyle, x, y, w, h, id);
            WriteString(caption); // title
            m_buf.Write<WORD>(0); // extraCount
        }

        void AddStaticIcon(LPCWSTR icon, DWORD style, DWORD exStyle, short x, short y, short w, short h, WORD id)
        {
            const WORD iconValue = (WORD)((LONG_PTR)icon);
            AddStandardItem(0x0082, style, exStyle, x, y, w, h, id);
            m_buf.Write<WORD>(0xFFFF); // title, predefined resource sentinel
            m_buf.Write<WORD>(iconValue); // title, the icon to show
            m_buf.Write<WORD>(0); // extraCount
        }

        void AddListBox(LPCWSTR caption, DWORD style, DWORD exStyle, short x, short y, short w, short h, WORD id)
        {
            AddStandardItem(0x0083, style, exStyle, x, y, w, h, id);
            WriteString(caption); // title
            m_buf.Write<WORD>(0); // extraCount
        }

        void AddScrollBar(LPCWSTR caption, DWORD style, DWORD exStyle, short x, short y, short w, short h, WORD id)
        {
            AddStandardItem(0x0084, style, exStyle, x, y, w, h, id);
            WriteString(caption); // title
            m_buf.Write<WORD>(0); // extraCount
        }

        void AddComboBox(LPCWSTR caption, DWORD style, DWORD exStyle, short x, short y, short w, short h, WORD id)
        {
            AddStandardItem(0x0085, style, exStyle, x, y, w, h, id);
            WriteString(caption); // title
            m_buf.Write<WORD>(0); // extraCount
        }

        LPCDLGTEMPLATE AsTemplate() const
        {
            return reinterpret_cast<LPCDLGTEMPLATE>(m_buf.Data());
        }

    private:
        void AlignToDword()
        {
            const uint32_t mod = m_buf.Size() % sizeof(DWORD);

            if (mod != 0)
            {
                m_buf.WriteRepeat(0, sizeof(DWORD) - mod);
            }
        }

        void WriteString(LPCWSTR str)
        {
            HE_ASSERT((m_buf.Size() % sizeof(WORD)) == 0);
            m_buf.Write(str, (lstrlenW(str) + 1) * sizeof(WCHAR));
        }

        void AddItemHeader(DWORD style, DWORD exStyle, short x, short y, short w, short h, WORD id)
        {
            // Increment item count
            reinterpret_cast<LPWORD>(m_buf.Data())[8] += 1;

            AlignToDword();
            m_buf.Write<DWORD>(0); // helpID
            m_buf.Write<DWORD>(exStyle); // exStyle
            m_buf.Write<DWORD>(style); // style
            m_buf.Write<short>(x); // x
            m_buf.Write<short>(y); // y
            m_buf.Write<short>(w); // cx
            m_buf.Write<short>(h); // cy
            m_buf.Write<DWORD>(id); // id
        }

        void AddStandardItem(WORD type, DWORD style, DWORD exStyle, short x, short y, short w, short h, WORD id)
        {
            AddItemHeader(style, exStyle, x, y, w, h, id);

            m_buf.Write<WORD>(0xFFFF); // windowClass, predefined item sentinel
            m_buf.Write<WORD>(type); // windowClass
        }

    private:
        BufferWriter m_buf{};
        uint32_t m_ctrlCountOffset{ 0 };
    };

    INT_PTR CALLBACK DialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM)
    {
        switch (msg)
        {
            case WM_COMMAND:
            {
                const WORD id = LOWORD(wParam);
                switch (id)
                {
                    case IDABORT:
                    case IDRETRY:
                    case IDIGNORE:
                        EndDialog(hWnd, id);
                        return TRUE;
                }
                break;
            }
        }
        return FALSE;
    }

    static const wchar_t* GetErrorDialogTitle(ErrorKind kind)
    {
        switch (kind)
        {
            case ErrorKind::Assert: return L"Assertion Failed";
            case ErrorKind::Except: return L"Exception Thrown";
            case ErrorKind::Expect: return L"Expectation Failed";
            case ErrorKind::Verify: return L"Verification Failed";
        }
        return L"Application Error";
    }

    bool _PlatformErrorHandler(const ErrorSource& source, const KeyValue* kvs, uint32_t count)
    {
        HE_UNUSED(source);

        const short margin = 5;
        const short btnWidth = 50;
        const short itemHeight = 14;
        const short dlgHeight = 150;
        const short dlgWidth = 400;

        const ErrorKind errorKind = kvs[0].GetEnum<ErrorKind>();
        const wchar_t* title = GetErrorDialogTitle(errorKind);
        const DWORD style = WS_CAPTION | DS_FIXEDSYS | DS_SETFONT | DS_MODALFRAME | DS_CENTER;

        DialogTemplate temp(title, style, 32, 32, dlgWidth, dlgHeight);

        const LPCWSTR icon = errorKind == ErrorKind::Assert ? IDI_ERROR : IDI_WARNING;
        temp.AddStaticIcon(
            icon,
            WS_CHILD | WS_VISIBLE | SS_ICON | SS_REALSIZECONTROL | SS_CENTERIMAGE,
            0,
            margin,
            margin,
            itemHeight,
            itemHeight,
            0xffff);

        String errorTitle = AsString(errorKind);
        errorTitle += " Details:";
        temp.AddStaticText(
            HE_TO_WSTR(errorTitle.Data()),
            WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX | SS_CENTERIMAGE,
            0,
            itemHeight + (margin * 2),
            margin,
            dlgWidth - (margin * 3) - itemHeight,
            itemHeight,
            0xffff);

        String errorMsg;
        fmt::format_to(Appender(errorMsg), "{}", fmt::join(kvs, kvs + count, "\r\n"));
        fmt::format_to(Appender(errorMsg), "\r\nsource.file = {}\r\nsource.line = {}\r\nsource.funcName = {}", source.file, source.line, source.funcName);
        temp.AddEditBox(
            HE_TO_WSTR(errorMsg.Data()),
            WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | WS_HSCROLL | ES_READONLY | ES_MULTILINE,
            0,
            margin,
            margin + itemHeight,
            dlgWidth - (margin * 2),
            dlgHeight - (margin * 4) - (itemHeight * 2),
            0xffff);

        temp.AddButton(
            L"Close App",
            WS_CHILD | WS_VISIBLE | WS_GROUP | WS_TABSTOP | BS_DEFPUSHBUTTON,
            0,
            dlgWidth - (margin * 3) - (btnWidth * 3),
            dlgHeight - margin - itemHeight,
            btnWidth,
            itemHeight,
            IDABORT);

        temp.AddButton(
            L"Debug",
            WS_CHILD | WS_VISIBLE | WS_GROUP | WS_TABSTOP | BS_PUSHBUTTON,
            0,
            dlgWidth - (margin * 2) - (btnWidth * 2),
            dlgHeight - margin - itemHeight,
            btnWidth,
            itemHeight,
            IDRETRY);

        temp.AddButton(
            L"Ignore",
            WS_CHILD | WS_VISIBLE | WS_GROUP | WS_TABSTOP | BS_PUSHBUTTON,
            0,
            dlgWidth - margin - btnWidth,
            dlgHeight - margin - itemHeight,
            btnWidth,
            itemHeight,
            IDIGNORE);

        DPI_AWARENESS_CONTEXT oldDpiAwareness = ::SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        INT_PTR result = ::DialogBoxIndirectW(NULL, temp.AsTemplate(), NULL, DialogProc);
        ::SetThreadDpiAwarenessContext(oldDpiAwareness);

        switch (result)
        {
            case IDABORT: ::ExitProcess(255); return true; // exit immediately
            case IDRETRY: return true; // debug break
            case IDIGNORE: return false; // continue
        }

        // Unknown result type...what
        HE_DEBUG_BREAK();
        return true;
    }
}

#endif
