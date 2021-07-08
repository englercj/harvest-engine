// Copyright Chad Engler

#include "he/core/string_view.h"

#include "he/core/memory_ops.h"
#include "he/core/string.h"

namespace he
{
    StringView::StringView(const char* str)
        : m_span(str, String::Length(str))
    {}

    bool StringView::operator==(const String& x)
    {
        return CompareTo(x) == 0;
    }

    bool StringView::operator!=(const String& x)
    {
        return CompareTo(x) != 0;
    }

    bool StringView::operator<(const String& x)
    {
        return CompareTo(x) < 0;
    }

    bool StringView::operator<=(const String& x)
    {
        return CompareTo(x) <= 0;
    }

    bool StringView::operator>(const String& x)
    {
        return CompareTo(x) > 0;
    }

    bool StringView::operator>=(const String& x)
    {
        return CompareTo(x) >= 0;
    }

    int32_t StringView::CompareTo(const StringView& x)
    {
        const uint32_t size = Min(Size(), x.Size());
        const int32_t cmp = MemCmp(Data(), x.Data(), size);

        if (cmp == 0)
            return Size() - x.Size();

        return cmp;
    }

    bool StringView::EqualTo(const StringView& x)
    {
        return CompareTo(x) == 0;
    }

    bool StringView::LessThan(const StringView& x)
    {
        return CompareTo(x) < 0;
    }
}
