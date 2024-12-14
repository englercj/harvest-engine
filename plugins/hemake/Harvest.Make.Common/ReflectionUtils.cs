// Copyright Chad Engler

namespace Harvest.Make;

public static class ReflectionUtils
{
    public static bool IsInstanceOfGenericType(Type concreteType, Type genericType)
    {
        Type? type = concreteType;
        while (type is not null)
        {
            if (type.IsGenericType && type.GetGenericTypeDefinition() == genericType)
            {
                return true;
            }
            type = type.BaseType;
        }
        return false;
    }

    public static bool IsInstanceOfGenericType<TConcrete>(Type genericType)
    {
        return IsInstanceOfGenericType(typeof(TConcrete), genericType);
    }

    public static bool IsInstanceOfGenericType<TConcrete, TGeneric>()
    {
        return IsInstanceOfGenericType(typeof(TConcrete), typeof(TGeneric));
    }

    public static bool IsTypeSignedIntegral(Type type)
    {
        switch (Type.GetTypeCode(type))
        {
            case TypeCode.SByte:
            case TypeCode.Int16:
            case TypeCode.Int32:
            case TypeCode.Int64:
                return true;
            case TypeCode.Object:
                if (type.IsGenericType && type.GetGenericTypeDefinition() == typeof(Nullable<>))
                {
                    return IsTypeSignedIntegral(Nullable.GetUnderlyingType(type)!);
                }
                return false;
        }

        return false;
    }

    public static bool IsTypeUnsignedIntegral(Type type)
    {
        switch (Type.GetTypeCode(type))
        {
            case TypeCode.Byte:
            case TypeCode.UInt16:
            case TypeCode.UInt32:
            case TypeCode.UInt64:
                return true;
            case TypeCode.Object:
                if (type.IsGenericType && type.GetGenericTypeDefinition() == typeof(Nullable<>))
                {
                    return IsTypeUnsignedIntegral(Nullable.GetUnderlyingType(type)!);
                }
                return false;
        }

        return false;
    }

    public static bool IsTypeFloatingPoint(Type type)
    {
        switch (Type.GetTypeCode(type))
        {
            case TypeCode.Single:
            case TypeCode.Double:
            case TypeCode.Decimal:
                return true;
            case TypeCode.Object:
                if (type.IsGenericType && type.GetGenericTypeDefinition() == typeof(Nullable<>))
                {
                    return IsTypeFloatingPoint(Nullable.GetUnderlyingType(type)!);
                }
                return false;
        }

        return false;
    }

    public static bool IsTypeIntegral(Type type) => IsTypeSignedIntegral(type) || IsTypeUnsignedIntegral(type);
    public static bool IsTypeNumber(Type type) => IsTypeIntegral(type) || IsTypeFloatingPoint(type);

    public static bool IsTypeSignedIntegral<T>() => IsTypeSignedIntegral(typeof(T));
    public static bool IsTypeUnsignedIntegral<T>() => IsTypeUnsignedIntegral(typeof(T));
    public static bool IsTypeFloatingPoint<T>() => IsTypeFloatingPoint(typeof(T));
    public static bool IsTypeIntegral<T>() => IsTypeIntegral(typeof(T));
    public static bool IsTypeNumber<T>() => IsTypeNumber(typeof(T));
}
