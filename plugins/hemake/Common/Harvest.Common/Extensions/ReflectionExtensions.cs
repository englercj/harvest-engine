// Copyright Chad Engler

using System.Reflection;

namespace Harvest.Common.Extensions;

public static class ReflectionExtensions
{
    public static bool IsInstanceOfGenericType(this Type concreteType, Type genericType)
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

    public static bool IsTypeSignedIntegral(this Type type)
    {
        switch (Type.GetTypeCode(type))
        {
            case TypeCode.SByte:
            case TypeCode.Int16:
            case TypeCode.Int32:
            case TypeCode.Int64:
                return true;
            case TypeCode.Object:
                if (Nullable.GetUnderlyingType(type) is not null)
                {
                    return IsTypeSignedIntegral(Nullable.GetUnderlyingType(type)!);
                }
                return false;
        }

        return false;
    }

    public static bool IsTypeUnsignedIntegral(this Type type)
    {
        switch (Type.GetTypeCode(type))
        {
            case TypeCode.Byte:
            case TypeCode.UInt16:
            case TypeCode.UInt32:
            case TypeCode.UInt64:
                return true;
            case TypeCode.Object:
                if (Nullable.GetUnderlyingType(type) is not null)
                {
                    return IsTypeUnsignedIntegral(Nullable.GetUnderlyingType(type)!);
                }
                return false;
        }

        return false;
    }

    public static bool IsTypeFloatingPoint(this Type type)
    {
        switch (Type.GetTypeCode(type))
        {
            case TypeCode.Single:
            case TypeCode.Double:
            case TypeCode.Decimal:
                return true;
            case TypeCode.Object:
                if (Nullable.GetUnderlyingType(type) is not null)
                {
                    return IsTypeFloatingPoint(Nullable.GetUnderlyingType(type)!);
                }
                return false;
        }

        return false;
    }

    public static bool IsTypeIntegral(this Type type) => type.IsTypeSignedIntegral() || type.IsTypeUnsignedIntegral();
    public static bool IsTypeNumber(this Type type) => type.IsTypeIntegral() || type.IsTypeFloatingPoint();

    public static bool IsTypeSignedIntegral<T>() => typeof(T).IsTypeSignedIntegral();
    public static bool IsTypeUnsignedIntegral<T>() => typeof(T).IsTypeUnsignedIntegral();
    public static bool IsTypeFloatingPoint<T>() => typeof(T).IsTypeFloatingPoint();
    public static bool IsTypeIntegral<T>() => typeof(T).IsTypeIntegral();
    public static bool IsTypeNumber<T>() => typeof(T).IsTypeNumber();

    public static object? Invoke(Type type, string methodName, object? instance, params object[] parameters)
    {
        MethodInfo? method = type.GetMethod(methodName, BindingFlags.Instance | BindingFlags.Public);
        return method?.Invoke(instance, parameters);
    }

    public static object? InvokeStatic(Type type, string methodName, params object[] parameters)
    {
        return Invoke(type, methodName, null, parameters);
    }

    public static object? InvokeStatic<T>(string methodName, params object[] parameters)
    {
        return Invoke(typeof(T), methodName, null, parameters);
    }

    public static object? InvokeMember(string methodName, object instance, params object[] parameters)
    {
        return Invoke(instance.GetType(), methodName, instance, parameters);
    }
}
