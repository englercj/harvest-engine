// Copyright Chad Engler

using Harvest.Kdl.Types;
using Harvest.Make.Projects.Attributes;
using Harvest.Make.Projects.Nodes;
using System.Reflection;

namespace Harvest.Make.Projects;

public class NodeKdlEnum<T> : NodeKdlValue<KdlString> where T : struct, Enum
{
    private static List<object> s_validValues = KdlEnumUtils.GetNames<T>().ToList<object>();

    public static new NodeKdlEnum<T> Required => new() { IsRequired = true, ValidValues = s_validValues };
    public static new NodeKdlEnum<T> Optional => new() { IsRequired = false, ValidValues = s_validValues };
}

public static class KdlEnumUtils
{
    public static T Parse<T>(string value) where T : struct, Enum
    {
        return KdlEnumInfo<T>.Values[value];
    }

    public static T Parse<T>(string? value, T defaultValue) where T : struct, Enum
    {
        if (value is not null && KdlEnumInfo<T>.Values.TryGetValue(value, out T result))
        {
            return result;
        }

        return defaultValue;
    }

    public static bool TryParse<T>(string value, out T result) where T : struct, Enum
    {
        return KdlEnumInfo<T>.Values.TryGetValue(value, out result);
    }

    public static string? GetName<T>(T value) where T : struct, Enum
    {
        return KdlEnumInfo<T>.Values.FirstOrDefault(x => x.Value.Equals(value)).Key;
    }

    public static IEnumerable<string> GetNames<T>() where T : struct, Enum
    {
        return KdlEnumInfo<T>.Values.Keys;
    }

    private static class KdlEnumInfo<T> where T : struct, Enum
    {
        private static readonly Dictionary<string, T> s_values = [];

        public static IReadOnlyDictionary<string, T> Values = s_values;

        static KdlEnumInfo()
        {
            FieldInfo[] fields = typeof(T).GetFields();

            foreach (FieldInfo field in fields)
            {
                KdlNameAttribute? attr = field.GetCustomAttribute<KdlNameAttribute>(false);
                string name = attr?.Name ?? field.Name;

                s_values[name] = (T)field.GetValue(null)!;
            }
        }
    }
}
