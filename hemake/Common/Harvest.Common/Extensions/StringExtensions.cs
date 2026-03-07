// Copyright Chad Engler

using System.Globalization;
using System.Security.Cryptography;
using System.Text;
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

    public static string ToSHA256HexDigest(this string value)
    {
        byte[] valueBytes = Encoding.UTF8.GetBytes(value);
        byte[] hashBytes = SHA256.HashData(valueBytes);
        return Convert.ToHexString(hashBytes);
    }

    // The pattern handles:
    // 1. (?<=[a-z])(?=[A-Z])       -> Look for Lowercase followed by Uppercase (e.g. "camelCase")
    // 2. (?<=[A-Z])(?=[A-Z][a-z])  -> Look for Uppercase followed by Uppercase then Lowercase (e.g. "HTMLParser")
    // 3. (?<=[0-9])(?=[A-Z])       -> Look for Digit followed by Uppercase (e.g. "PS5Client")
    [GeneratedRegex(@"(?<=[a-z])(?=[A-Z])|(?<=[A-Z])(?=[A-Z][a-z])|(?<=[0-9])(?=[A-Z])")]
    private static partial Regex SplitWordsRegex();

    [GeneratedRegex(@"\s+")]
    private static partial Regex SpacesRegex();
}
