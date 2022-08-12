// Copyright Chad Engler

#pragma once

#include "he/core/types.h"

namespace he::editor
{
    class Command
    {
    public:
        virtual ~Command() = default;

        virtual bool CanRun() const = 0;
        virtual void Run() = 0;

        virtual const char* Label() const { return nullptr; };
        virtual const char* Icon() const { return nullptr; }
        virtual const char* Shortcut() const { return nullptr; }
    };
}
