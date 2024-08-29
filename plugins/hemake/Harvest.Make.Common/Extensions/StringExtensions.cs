// Copyright Chad Engler

using System.Globalization;
using System.Text.RegularExpressions;

namespace Harvest.Make.Utils;

public enum StringCasing
{
    LowerCase,
    UpperCase,
    TitleCase,
    PascalCase,
    CamelCase,
    KebabCase,
    SnakeCase,
}

public static partial class StringExtensions
{
    public static string ToCase(this string value, StringCasing casing)
    {
        if (string.IsNullOrWhiteSpace(value))
            return value;

        switch (casing)
        {
            case StringCasing.LowerCase: return value.ToLowerInvariant();
            case StringCasing.UpperCase: return value.ToUpperInvariant();
            case StringCasing.TitleCase: return value.ToTitleCase();
            case StringCasing.PascalCase: return value.ToPascalCase();
            case StringCasing.CamelCase: return value.ToCamelCase();
            case StringCasing.KebabCase: return value.ToKebabCase();
            case StringCasing.SnakeCase: return value.ToSnakeCase();
            default: return value;
        }
    }

    public static string ToCamelCase(this string value)
    {
        string[] words = SplitWordsRegex().Split(value.Trim());
        IEnumerable<string> transformed = words.Select((word, index) =>
        {
            word = (index == 0) ? word.ToLowerInvariant() : word.ToLowerInvariant().ToTitleCase();
            return SpacesRegex().Replace(word, "");
        });
        return string.Concat(transformed);
    }

    public static string ToKebabCase(this string value)
    {
        string[] words = SplitWordsRegex().Split(value.Trim());
        IEnumerable<string> transformed = words.Select(word => SpacesRegex().Replace(word.ToLowerInvariant(), "-"));
        return string.Join("-", transformed);
    }

    public static string ToPascalCase(this string value)
    {
        string[] words = SplitWordsRegex().Split(value.Trim());
        IEnumerable<string> transformed = words.Select(word => SpacesRegex().Replace(word.ToLowerInvariant().ToTitleCase(), ""));
        return string.Concat(transformed);
    }

    public static string ToSnakeCase(this string value)
    {
        string[] words = SplitWordsRegex().Split(value.Trim());
        IEnumerable<string> transformed = words.Select(word => SpacesRegex().Replace(word.ToLowerInvariant(), "_"));
        return string.Join("_", transformed);
    }

    public static string ToTitleCase(this string value)
    {
        string[] words = SplitWordsRegex().Split(value.Trim());
        IEnumerable<string> transformed = words.Select(word => CultureInfo.InvariantCulture.TextInfo.ToTitleCase(word.ToLowerInvariant()));
        return string.Concat(transformed);
    }

    [GeneratedRegex("(?<=[a-z])(?=[A-Z])|(?<=[0-9])(?=[A-Za-z])|(?<=[A-Za-z])(?=[0-9])")]
    private static partial Regex SplitWordsRegex();

    [GeneratedRegex(@"\s+")]
    private static partial Regex SpacesRegex();
}
