namespace Harvest.Common.Attributes;

// Attributes in this file are used to mark services and view models for
// the source generator to generate dependency injection code.

[AttributeUsage(AttributeTargets.Class, Inherited = false)]
public class SingletonServiceAttribute : Attribute { }

[AttributeUsage(AttributeTargets.Class, Inherited = false)]
public class SingletonServiceAttribute<T> : Attribute where T : class { }

[AttributeUsage(AttributeTargets.Class, Inherited = false)]
public class TransientServiceAttribute : Attribute { }

[AttributeUsage(AttributeTargets.Class, Inherited = false)]
public class TransientServiceAttribute<T> : Attribute where T : class { }

[AttributeUsage(AttributeTargets.Class, Inherited = false)]
public class ScopedServiceAttribute : Attribute { }

[AttributeUsage(AttributeTargets.Class, Inherited = false)]
public class ScopedServiceAttribute<T> : Attribute where T : class { }
