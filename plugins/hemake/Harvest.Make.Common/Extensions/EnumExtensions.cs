// Copyright Chad Engler

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Harvest.Make.Extensions;

public static partial class EnumExtensions
{
    public static bool HasAnyFlag<T>(this T value, T flags) where T : struct, Enum
    {
        return (Convert.ToUInt64(value) & Convert.ToUInt64(flags)) != 0;
    }
}
