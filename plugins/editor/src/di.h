// Copyright Chad Engler

#pragma once

#include "editor_data.h"

#include "he/core/allocator.h"
#include "he/core/compiler.h"
#include "he/core/utils.h"

HE_PUSH_WARNINGS()
HE_DISABLE_MSVC_WARNING(4668)

#include "boost/di.hpp"
#include "boost/di/extension/injector.hpp"

HE_POP_WARNINGS()

namespace di = boost::di;

namespace he::editor
{
    inline auto MakeAppInjector()
    {
        static he::editor::EditorData s_editorData{};

        return di::make_injector(
            di::bind<he::Allocator>().to(he::CrtAllocator::Get()),
            di::bind<he::editor::EditorData>().to(s_editorData)
        );
    }

    using AppInjectorType = decltype(MakeAppInjector());

    extern const AppInjectorType* g_appInjector;

    template <typename T>
    auto DICreate() -> decltype(g_appInjector->template create<T>())
    {
        return g_appInjector->template create<T>();
    }

    template <typename T>
    std::unique_ptr<T> DICreateUnique()
    {
        return DICreate<std::unique_ptr<T>>();
    }
}
