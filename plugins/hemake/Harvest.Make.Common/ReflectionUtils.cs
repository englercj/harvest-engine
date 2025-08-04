// Copyright Chad Engler

using System.Reflection;

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
                if (Nullable.GetUnderlyingType(type) is not null)
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
                if (Nullable.GetUnderlyingType(type) is not null)
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
                if (Nullable.GetUnderlyingType(type) is not null)
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
