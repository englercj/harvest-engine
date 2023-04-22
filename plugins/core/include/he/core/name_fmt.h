// Copyright Chad Engler

#pragma once

#include "he/core/fmt.h"
#include "he/core/name.h"

namespace he
{
    template <>
    struct Formatter<Name> : Formatter<const char*>
    {
        using Type = Name;

        void Format(String& out, const Name& name) const
        {
            if (name)
            {
                Formatter<const char*>::Format(out, name.String());
            }
        }
    };
}
