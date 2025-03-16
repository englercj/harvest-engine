// Copyright Chad Engler

namespace Harvest.Make.Extensions;

public static partial class EnumExtensions
{
    public static bool HasAnyFlag<T>(this T value, T flags) where T : struct, Enum
    {
        return (Convert.ToUInt64(value) & Convert.ToUInt64(flags)) != 0;
    }
}
