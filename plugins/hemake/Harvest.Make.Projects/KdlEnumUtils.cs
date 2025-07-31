// Copyright Chad Engler

using Harvest.Make.Projects.Attributes;
using System.Diagnostics.CodeAnalysis;
using System.Reflection;

namespace Harvest.Make.Projects;

public static class KdlEnumUtils
{
    public static T Parse<T>(string? value, T defaultValue) where T : struct, Enum
    {
        if (TryParse(value, out T result))
        {
            return result;
        }

        return defaultValue;
    }

    public static bool TryParse<T>(string? value, [MaybeNullWhen(false)] out T result) where T : struct, Enum
    {
        result = default;
        return value is not null && KdlEnumInfo<T>.Values.TryGetValue(value, out result);
    }

    public static string GetName<T>(T value) where T : struct, Enum
    {
        foreach ((string name, T enumerator) in KdlEnumInfo<T>.Values)
        {
            if (enumerator.Equals(value))
            {
                return name;
            }
        }

        return value.ToString();
    }

    public static IEnumerable<string> GetNames<T>() where T : struct, Enum
    {
        return KdlEnumInfo<T>.Values.Keys;
    }

    private static class KdlEnumInfo<T> where T : struct, Enum
    {
        private static readonly SortedDictionary<string, T> s_values = [];

        public static IReadOnlyDictionary<string, T> Values => s_values;

        static KdlEnumInfo()
        {
            FieldInfo[] fields = typeof(T).GetFields();

            foreach (FieldInfo field in fields)
            {
                if (!field.IsLiteral || field.IsSpecialName)
                {
                    continue;
                }

                if (field.GetValue(null) is T value)
                {
                    KdlNameAttribute? attr = field.GetCustomAttribute<KdlNameAttribute>(false);
                    string name = attr?.Name ?? field.Name;

                    s_values.Add(name, value);
                }
            }
        }
    }
}
