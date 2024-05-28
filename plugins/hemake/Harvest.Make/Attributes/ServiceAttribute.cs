// Copyright Chad Engler

using Microsoft.Extensions.DependencyInjection;

namespace Harvest.Make.Attributes;

/// <summary>
/// Marks a class as a service that should be registered with the application's <see cref="IServiceCollection"/>.
/// </summary>
[AttributeUsage(AttributeTargets.Class, AllowMultiple = false, Inherited = false)]
public class ServiceAttribute : Attribute
{
    /// <summary>
    /// The interfaces to register this service for.
    /// </summary>
    public IReadOnlyList<Type> Interfaces { get; set; } = [];

    /// <summary>
    /// Optional key to register the service with.
    /// </summary>
    public object? Key { get; set; } = null;

    /// <summary>
    /// Register as an enumerable service, one implementation of many for the implemented interface(s).
    /// </summary>
    public bool Enumerable { get; set; } = false;

    /// <summary>
    /// The lifetime of the service in the <see cref="IServiceCollection"/>. Default is Singleton.
    /// </summary>
    public ServiceLifetime Lifetime { get; set; } = ServiceLifetime.Singleton;
}

/// <summary>
/// Marks a class as a service that should be registered with the application's <see cref="IServiceCollection"/>.
/// </summary>
/// <typeparam name="T">The interface type to register this service for.</typeparam>
public class ServiceAttribute<T> : ServiceAttribute where T : class
{
    public ServiceAttribute() : base()
    {
        Interfaces = [typeof(T)];
    }
}
