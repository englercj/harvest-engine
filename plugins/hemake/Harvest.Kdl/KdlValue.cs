// Copyright Chad Engler

using Harvest.Kdl.Types;
using System.Numerics;

namespace Harvest.Kdl;

public abstract class KdlValue(string? type) : IKdlObject
{
    public static KdlValue From(object? o, string? type = null)
    {
        return o switch
        {
            null => new KdlNull(type),
            bool v => new KdlBool(v, type),
            sbyte v => new KdlNumber<sbyte>(v, radix: 10, type),
            short v => new KdlNumber<short>(v, radix: 10, type),
            int v => new KdlNumber<int>(v, radix: 10, type),
            long v => new KdlNumber<long>(v, radix: 10, type),
            byte v => new KdlNumber<byte>(v, radix: 10, type),
            ushort v => new KdlNumber<ushort>(v, radix: 10, type),
            uint v => new KdlNumber<uint>(v, radix: 10, type),
            ulong v => new KdlNumber<ulong>(v, radix: 10, type),
            float v => new KdlNumber<float>(v, radix: 10, type),
            double v => new KdlNumber<double>(v, radix: 10, type),
            decimal v => new KdlNumber<decimal>(v, radix: 10, type),
            BigInteger v => new KdlNumber<BigInteger>(v, radix: 10, type: type),
            string s => new KdlString(s, type),
            _ => throw new ArgumentException($"No KdlValue for object {o}"),
        };
    }

    public string? Type => type;

    public void WriteKdl(TextWriter writer, KdlWriteOptions options)
    {
        if (Type != null)
        {
            writer.Write('(');
            KdlUtils.WriteString(writer, Type, options);
            writer.Write(')');
        }
        WriteKdlValue(writer, options);
    }

    protected abstract void WriteKdlValue(TextWriter writer, KdlWriteOptions options);

    public abstract override bool Equals(object? obj);
    public abstract override int GetHashCode();
    public abstract string GetValueString();

    public static bool operator==(KdlValue? a, KdlValue? b) => (a is null) ? b is null : a.Equals(b);
    public static bool operator!=(KdlValue? a, KdlValue? b) => !(a == b);
}

public abstract class KdlValue<T>(T value, string? type) : KdlValue(type)
{
    public T Value => value;

    public override string ToString() => $"KdlValue{{ Value={Value}, Type={Type ?? "null"} }}";
    public override string GetValueString() => Value?.ToString() ?? "<null>";

    public override bool Equals(object? obj)
    {
        if (obj is KdlValue<T> v)
        {
            if (Type != v.Type)
            {
                return false;
            }

            return Value?.Equals(v.Value) ?? v.Value is null;
        }

        if (obj is T t)
        {
            return Value?.Equals(t) ?? false;
        }

        return false;
    }

    public override int GetHashCode() => HashCode.Combine(Value, Type);

    public static implicit operator T(KdlValue<T> v) => v.Value;
}
