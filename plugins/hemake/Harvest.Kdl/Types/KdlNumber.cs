// Copyright Chad Engler

using System.Diagnostics;
using System.Globalization;
using System.Numerics;
using System.Reflection;

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

    public override int GetHashCode() => HashCode.Combine(Value, Type);

    public override bool Equals(object? obj)
    {
        if (obj is null)
        {
            return false;
        }

        if (ReferenceEquals(this, obj))
        {
            return true;
        }

        if (obj is KdlNumber<T> kdlNum)
        {
            return Value.Equals(kdlNum.Value);
        }

        if (obj is T num)
        {
            return Value.Equals(num);
        }

        return EqualsObject(obj);
    }

    public bool EqualsNumber<U>(KdlNumber<U>? other) where U : struct, INumber<U>
    {
        if (other is null)
        {
            return false;
        }

        return EqualsNumber(other.Value);
    }

    public bool EqualsNumber<U>(U other) where U : struct, INumber<U>
    {
        try
        {
            T otherValueAsT = T.CreateChecked(other);
            return Value.Equals(otherValueAsT);
        }
        catch
        {
            return false;
        }
    }

    protected bool EqualsObject(object obj)
    {
        Type type = obj.GetType();

        switch (System.Type.GetTypeCode(type))
        {
            case TypeCode.SByte: return EqualsNumber((sbyte)obj);
            case TypeCode.Int16: return EqualsNumber((short)obj);
            case TypeCode.Int32: return EqualsNumber((int)obj);
            case TypeCode.Int64: return EqualsNumber((long)obj);
            case TypeCode.Byte: return EqualsNumber((byte)obj);
            case TypeCode.UInt16: return EqualsNumber((ushort)obj);
            case TypeCode.UInt32: return EqualsNumber((uint)obj);
            case TypeCode.UInt64: return EqualsNumber((ulong)obj);
            case TypeCode.Single: return EqualsNumber((float)obj);
            case TypeCode.Double: return EqualsNumber((double)obj);
            case TypeCode.Decimal: return EqualsNumber((decimal)obj);
            case TypeCode.Object:
            {
                if (obj is BigInteger bigInt)
                {
                    return EqualsNumber(bigInt);
                }
                else if (Nullable.GetUnderlyingType(type) is Type nullableType)
                {
                    if (!nullableType.IsValueType)
                    {
                        return false; // Nullable reference type, can't compare
                    }

                    if (type.GetProperty("Value")?.GetValue(obj) is object value)
                    {
                        return EqualsObject(value);
                    }
                }
                else if (type.IsGenericType && type.GetGenericTypeDefinition() == typeof(KdlNumber<>))
                {
                    // Invoke the Equals<U>(KdlNumber<U>) method via reflection
                    MethodInfo? method = typeof(KdlNumber<T>).GetMethod(nameof(Equals), BindingFlags.Public | BindingFlags.Instance);
                    if (method != null)
                    {
                        Type otherType = type.GetGenericArguments()[0];
                        MethodInfo genericMethod = method.MakeGenericMethod(otherType);
                        return (bool)genericMethod.Invoke(this, [obj])!;
                    }
                }
                return false;
            }
        }

        return false;
    }

    public override KdlValue Clone()
    {
        return new KdlNumber<T>(Value, Radix, Type) { SourceInfo = SourceInfo };
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
