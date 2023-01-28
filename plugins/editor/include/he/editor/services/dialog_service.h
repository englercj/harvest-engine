// Copyright Chad Engler

#pragma once

#include "he/core/type_traits.h"
#include "he/core/types.h"
#include "he/core/utils.h"
#include "he/core/unique_ptr.h"
#include "he/core/vector.h"
#include "he/editor/di.h"
#include "he/editor/dialogs/dialog.h"

namespace he::editor
{
    class DialogService
    {
    public:
        void DestroyClosedDialogs();

        void ShowDialogs();

        template <typename T> requires(IsBaseOf<Dialog, T>)
        T& Open()
        {
            m_dialogs.PushBack(DICreateUnique<T>());
            return *static_cast<T*>(m_dialogs.Back().Get());
        }

    private:
        Vector<UniquePtr<Dialog>> m_dialogs{};
    };
}
