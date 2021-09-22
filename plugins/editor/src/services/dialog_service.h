// Copyright Chad Engler

#pragma once

#include "di.h"
#include "dialogs/dialog.h"

#include "he/core/type_traits.h"
#include "he/core/utils.h"
#include "he/core/vector.h"

#include <memory>

namespace he::editor
{
    class DialogService
    {
    public:
        DialogService(Allocator& allocator);

        void DestroyClosedDialogs();

        void ShowDialogs();

        template <typename T>
        T& Open()
        {
            static_assert(std::is_base_of_v<Dialog, T>, "Dialog Service can only open Dialog objects.");
            m_dialogs.PushBack(DICreateUnique<T>());
            return *static_cast<T*>(m_dialogs.Back().get());
        }

    private:
        Vector<std::unique_ptr<Dialog>> m_dialogs;
    };
}
