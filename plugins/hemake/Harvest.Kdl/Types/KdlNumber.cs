// Copyright Chad Engler

using System.Diagnostics;
using System.Globalization;
using System.Numerics;

namespace Harvest.Kdl.Types;

/// <summary>
/// A <see cref="KdlValue"/> wrapper around a number.
/// </summary>
/// <typeparam name="T">The number type to wrap.</typeparam>
[DebuggerDisplay("{Value}")]
public class KdlNumber<T> : KdlValue<T> where T : struct, INumber<T>
{
    public int Radix { get; } = 10;

    public KdlNumber(T value, int radix, string? type = null)
        : base(value, type)
    {
        Radix = radix;
    }

    public KdlNumber(T value, string? type = null)
        : base(value, type)
    {
        Radix = 10;
    }

    protected override void WriteKdlValue(TextWriter writer, KdlWriteOptions options)
    {
        // Handle special values (nan, inf, -inf)
        string? specialValue = Value switch
        {
            float v => float.IsNaN(v) ? "#nan" : (float.IsPositiveInfinity(v) ? "#inf" : (float.IsNegativeInfinity(v) ? "#-inf" : null)),
            double v => double.IsNaN(v) ? "#nan" : (double.IsPositiveInfinity(v) ? "#inf" : (double.IsNegativeInfinity(v) ? "#-inf" : null)),
            _ => null
        };
        if (specialValue != null)
        {
            writer.Write(specialValue);
            return;
        }

        // Write the number values, which we know are finite
        int radix = options.KeepRadix ? Radix : 10;
        string result = Value switch
        {
            sbyte v => Convert.ToString((long)v, radix), // no sbyte overload
            short v => Convert.ToString(v, radix),
            int v => Convert.ToString(v, radix),
            long v => Convert.ToString(v, radix),
            byte v => Convert.ToString(v, radix),
            ushort v => Convert.ToString((long)v, radix), // no ushort overload
            uint v => Convert.ToString((long)v, radix), // no uint overload
            ulong v => Convert.ToString((long)v, radix), // no ulong overload
            float v => Math.Truncate(v) != v ? v.ToString("G", CultureInfo.GetCultureInfo("en-US")) : v.ToString("0.0", CultureInfo.GetCultureInfo("en-US")),
            double v => Math.Truncate(v) != v ? v.ToString("G", CultureInfo.GetCultureInfo("en-US")) : v.ToString("0.0", CultureInfo.GetCultureInfo("en-US")),
            decimal v => Math.Truncate(v) != v ? v.ToString("G", CultureInfo.GetCultureInfo("en-US")) : v.ToString("0.0", CultureInfo.GetCultureInfo("en-US")),
            BigInteger v => Convert.ToString(v) ?? "",
            _ => throw new InvalidOperationException("Invalid number type.")
        };

        switch (radix)
        {
            case 2:
                writer.Write("0b");
                writer.Write(result);
                break;
            case 8:
                writer.Write("0o");
                writer.Write(result);
                break;
            case 10:
                writer.Write(result
                    .Replace("E-0", "E-")
                    .Replace("E+0", "E+")
                    .Replace('E', options.ExponentChar));
                break;
            case 16:
                writer.Write("0x");
                writer.Write(result);
                break;
        }
    }
}
