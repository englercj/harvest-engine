#include "test_app.h"

#include "he/core/debug.h"
#include "he/window/event.h"

namespace he
{
    TestApp::TestApp(window::Device* device)
        : m_device(device)
    {}

    void TestApp::OnEvent(const window::Event& ev)
    {
        const char* typeName = window::AsString(ev.type);
        PrintToDebugger(typeName);
        PrintToDebugger(": ");

        switch (ev.type)
        {
            case window::EventType::MouseDown:
            {
                const auto& evt = static_cast<const window::MouseDownEvent&>(ev);
                PrintToDebugger("{{ button={} }}", window::AsString(evt.button));
                break;
            }
            case window::EventType::MouseUp:
            {
                const auto& evt = static_cast<const window::MouseUpEvent&>(ev);
                PrintToDebugger("{{ button={} }}", window::AsString(evt.button));
                break;
            }
            case window::EventType::MouseWheel:
            {
                const auto& evt = static_cast<const window::MouseWheelEvent&>(ev);
                PrintToDebugger("{{ delta.x={}, delta.y={} }}", evt.delta.x, evt.delta.y);
                break;
            }
            case window::EventType::MouseMove:
            {
                const auto& evt = static_cast<const window::MouseMoveEvent&>(ev);
                PrintToDebugger("{{ pos.x={}, pos.y={}, absolute={}, raw={} }}", evt.pos.x, evt.pos.y, evt.absolute, evt.raw);
                break;
            }
            case window::EventType::KeyDown:
            {
                const auto& evt = static_cast<const window::KeyDownEvent&>(ev);
                PrintToDebugger("{{ key={} }}", window::AsString(evt.key));
                break;
            }
            case window::EventType::KeyUp:
            {
                const auto& evt = static_cast<const window::KeyDownEvent&>(ev);
                PrintToDebugger("{{ key={} }}", window::AsString(evt.key));
                break;
            }
            case window::EventType::Text:
            {
                const auto& evt = static_cast<const window::TextEvent&>(ev);
                PrintToDebugger("{{ ch={} }}", wchar_t(evt.ch));
                break;
            }
            case window::EventType::GamepadAxis:
            {
                const auto& evt = static_cast<const window::GamepadAxisEvent&>(ev);
                PrintToDebugger("{{ index={}, axis={}, value={} }}", evt.index, window::AsString(evt.axis), evt.value);
                break;
            }
            case window::EventType::GamepadButtonDown:
            {
                const auto& evt = static_cast<const window::GamepadButtonDownEvent&>(ev);
                PrintToDebugger("{{ index={}, button={} }}", evt.index, window::AsString(evt.button));
                break;
            }
            case window::EventType::GamepadButtonUp:
            {
                const auto& evt = static_cast<const window::GamepadButtonUpEvent&>(ev);
                PrintToDebugger("{{ index={}, button={} }}", evt.index, window::AsString(evt.button));
                break;
            }
            case window::EventType::GamepadConnected:
            {
                const auto& evt = static_cast<const window::GamepadConnectedEvent&>(ev);
                PrintToDebugger("{{ index={} }}", evt.index);
                break;
            }
            case window::EventType::GamepadDisconnected:
            {
                const auto& evt = static_cast<const window::GamepadDisconnectedEvent&>(ev);
                PrintToDebugger("{{ index={} }}", evt.index);
                break;
            }
            case window::EventType::ViewRequestClose:
            {
                m_device->Quit(0);
                break;
            }
            case window::EventType::ViewMoved:
            {
                const auto& evt = static_cast<const window::ViewMovedEvent&>(ev);
                PrintToDebugger("{{ pos.x={}, pos.y={} }}", evt.pos.x, evt.pos.y);
                break;
            }
            case window::EventType::ViewResized:
            {
                const auto& evt = static_cast<const window::ViewResizedEvent&>(ev);
                PrintToDebugger("{{ size.x={}, size.y={}, min={}, max={} }}", evt.size.x, evt.size.y, evt.view->IsMinimized(), evt.view->IsMaximized());
                break;
            }
            case window::EventType::ViewActivated:
            {
                const auto& evt = static_cast<const window::ViewActivatedEvent&>(ev);
                PrintToDebugger("{{ active={} }}", evt.active);
                break;
            }
            case window::EventType::ViewDpiScaleChanged:
            {
                const auto& evt = static_cast<const window::ViewDpiScaleChangedEvent&>(ev);
                PrintToDebugger("{{ scale={} }}", evt.scale);
                break;
            }
            case window::EventType::ViewDropFile:
            {
                const auto& evt = static_cast<const window::ViewDropFileEvent&>(ev);
                PrintToDebugger("{{ filePath={} }}", evt.filePath.Data());
                break;
            }
            case window::EventType::Initialized:
            {
                const auto& evt = static_cast<const window::InitializedEvent&>(ev);
                HE_UNUSED(evt);
                break;
            }
            case window::EventType::Terminating:
            {
                const auto& evt = static_cast<const window::TerminatingEvent&>(ev);
                HE_UNUSED(evt);
                break;
            }
            case window::EventType::Suspending:
            {
                const auto& evt = static_cast<const window::SuspendingEvent&>(ev);
                HE_UNUSED(evt);
                break;
            }
            case window::EventType::Resuming:
            {
                const auto& evt = static_cast<const window::ResumingEvent&>(ev);
                HE_UNUSED(evt);
                break;
            }
            case window::EventType::DisplayChanged:
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
