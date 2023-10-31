#include "test_app.h"

#include "he/core/debugger.h"
#include "he/core/string_fmt.h"
#include "he/core/wstr.h"
#include "he/math/types_fmt.h"
#include "he/window/event.h"

namespace he
{
    TestApp::TestApp(window::Device* device)
        : m_device(device)
    {}

    void TestApp::OnEvent(const window::Event& ev)
    {
        const char* typeName = AsString(ev.kind);
        PrintToDebugger(typeName);
        PrintToDebugger(": ");

        switch (ev.kind)
        {
            case window::EventKind::PointerDown:
            {
                const auto& evt = static_cast<const window::PointerDownEvent&>(ev);
                PrintToDebugger("{{ pointerId={}, pointerKind={}, size={}, tilt={}, rotation={}, pressure={}, isPrimary={}, button={} }}",
                    evt.pointerId, evt.pointerKind, evt.size, evt.tilt, evt.rotation, evt.pressure, evt.isPrimary, evt.button);
                break;
            }
            case window::EventKind::PointerUp:
            {
                const auto& evt = static_cast<const window::PointerUpEvent&>(ev);
                PrintToDebugger("{{ pointerId={}, pointerKind={}, size={}, tilt={}, rotation={}, pressure={}, isPrimary={}, button={} }}",
                    evt.pointerId, evt.pointerKind, evt.size, evt.tilt, evt.rotation, evt.pressure, evt.isPrimary, evt.button);
                break;
            }
            case window::EventKind::PointerWheel:
            {
                const auto& evt = static_cast<const window::PointerWheelEvent&>(ev);
                PrintToDebugger("{{ pointerId={}, pointerKind={}, size={}, tilt={}, rotation={}, pressure={}, isPrimary={}, delta={} }}",
                    evt.pointerId, evt.pointerKind, evt.size, evt.tilt, evt.rotation, evt.pressure, evt.isPrimary, evt.delta);
                break;
            }
            case window::EventKind::PointerMove:
            {
                const auto& evt = static_cast<const window::PointerMoveEvent&>(ev);
                PrintToDebugger("{{ pointerId={}, pointerKind={}, size={}, tilt={}, rotation={}, pressure={}, isPrimary={}, pos={}, absolute={} }}",
                    evt.pointerId, evt.pointerKind, evt.size, evt.tilt, evt.rotation, evt.pressure, evt.isPrimary, evt.pos, evt.absolute);
                break;
            }
            case window::EventKind::KeyDown:
            {
                const auto& evt = static_cast<const window::KeyDownEvent&>(ev);
                PrintToDebugger("{{ key={} }}", evt.key);
                break;
            }
            case window::EventKind::KeyUp:
            {
                const auto& evt = static_cast<const window::KeyDownEvent&>(ev);
                PrintToDebugger("{{ key={} }}", evt.key);
                break;
            }
            case window::EventKind::Text:
            {
                const auto& evt = static_cast<const window::TextEvent&>(ev);
                const wchar_t ch = static_cast<wchar_t>(evt.ch);
                String str;
                WCToMBStr(str, &ch, 1);
                PrintToDebugger("{{ ch={} }}", str);
                break;
            }
            case window::EventKind::GamepadAxis:
            {
                const auto& evt = static_cast<const window::GamepadAxisEvent&>(ev);
                PrintToDebugger("{{ index={}, axis={}, value={} }}", evt.index, evt.axis, evt.value);
                break;
            }
            case window::EventKind::GamepadButtonDown:
            {
                const auto& evt = static_cast<const window::GamepadButtonDownEvent&>(ev);
                PrintToDebugger("{{ index={}, button={} }}", evt.index, evt.button);
                break;
            }
            case window::EventKind::GamepadButtonUp:
            {
                const auto& evt = static_cast<const window::GamepadButtonUpEvent&>(ev);
                PrintToDebugger("{{ index={}, button={} }}", evt.index, evt.button);
                break;
            }
            case window::EventKind::GamepadConnected:
            {
                const auto& evt = static_cast<const window::GamepadConnectedEvent&>(ev);
                PrintToDebugger("{{ index={} }}", evt.index);
                break;
            }
            case window::EventKind::GamepadDisconnected:
            {
                const auto& evt = static_cast<const window::GamepadDisconnectedEvent&>(ev);
                PrintToDebugger("{{ index={} }}", evt.index);
                break;
            }
            case window::EventKind::ViewRequestClose:
            {
                m_device->Quit(0);
                break;
            }
            case window::EventKind::ViewMoved:
            {
                const auto& evt = static_cast<const window::ViewMovedEvent&>(ev);
                PrintToDebugger("{{ pos.x={}, pos.y={} }}", evt.pos.x, evt.pos.y);
                break;
            }
            case window::EventKind::ViewResized:
            {
                const auto& evt = static_cast<const window::ViewResizedEvent&>(ev);
                PrintToDebugger("{{ size.x={}, size.y={}, min={}, max={} }}", evt.size.x, evt.size.y, evt.view->IsMinimized(), evt.view->IsMaximized());
                break;
            }
            case window::EventKind::ViewActivated:
            {
                const auto& evt = static_cast<const window::ViewActivatedEvent&>(ev);
                PrintToDebugger("{{ active={} }}", evt.active);
                break;
            }
            case window::EventKind::ViewDpiScaleChanged:
            {
                const auto& evt = static_cast<const window::ViewDpiScaleChangedEvent&>(ev);
                PrintToDebugger("{{ scale={} }}", evt.scale);
                break;
            }
            case window::EventKind::ViewDndStart:
            {
                const auto& evt = static_cast<const window::ViewDndStartEvent&>(ev);
                HE_UNUSED(evt);
                break;
            }
            case window::EventKind::ViewDndMove:
            {
                const auto& evt = static_cast<const window::ViewDndMoveEvent&>(ev);
                PrintToDebugger("{{ pos={} }}", evt.pos);
                break;
            }
            case window::EventKind::ViewDndDrop:
            {
                const auto& evt = static_cast<const window::ViewDndDropEvent&>(ev);
                PrintToDebugger("{{ path={} }}", evt.path);
                break;
            }
            case window::EventKind::ViewDndEnd:
            {
                const auto& evt = static_cast<const window::ViewDndEndEvent&>(ev);
                HE_UNUSED(evt);
                break;
            }
            case window::EventKind::Initialized:
            {
                const auto& evt = static_cast<const window::InitializedEvent&>(ev);
                HE_UNUSED(evt);

                window::ViewDesc desc;
                desc.title = "HE Window Test App";

                m_view = m_device->CreateView(desc);
                m_view->SetVisible(true, true);
                break;
            }
            case window::EventKind::Terminating:
            {
                const auto& evt = static_cast<const window::TerminatingEvent&>(ev);
                HE_UNUSED(evt);
                break;
            }
            case window::EventKind::Suspending:
            {
                const auto& evt = static_cast<const window::SuspendingEvent&>(ev);
                HE_UNUSED(evt);
                break;
            }
            case window::EventKind::Resuming:
            {
                const auto& evt = static_cast<const window::ResumingEvent&>(ev);
                HE_UNUSED(evt);
                break;
            }
            case window::EventKind::DisplayChanged:
            {
                const auto& evt = static_cast<const window::DisplayChangedEvent&>(ev);
                HE_UNUSED(evt);
                break;
            }
        }
        PrintToDebugger("\n");
    }

    void TestApp::OnTick()
    {

    }
}
