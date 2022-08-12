// Copyright Chad Engler

#pragma once

#include "di.h"
#include "dialogs/dialog.h"

#include "he/core/types.h"
#include "he/core/utils.h"
#include "he/core/unique_ptr.h"
#include "he/core/vector.h"

#include <type_traits>

namespace he::editor
{
    class DialogService
    {
    public:
        void DestroyClosedDialogs();

        void ShowDialogs();

        template <typename T> requires(std::is_base_of_v<Dialog, T>)
        T& Open()
        {
            m_dialogs.PushBack(DICreateUnique<T>());
            return *static_cast<T*>(m_dialogs.Back().Get());
        }

    private:
        Vector<UniquePtr<Dialog>> m_dialogs{};
    };
}
