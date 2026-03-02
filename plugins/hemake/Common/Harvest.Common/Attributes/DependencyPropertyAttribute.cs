namespace Luna.Common.Attributes;

/// <summary>
/// An Attribute that indicates that a given partial property should be implemented by the source generator.
/// In ortder to use this attribute, the containing type has to inherit from <see cref="DependencyObject"/>.
/// <para>
/// This attribute can be used as follows:
/// <code>
/// partial class MyControl : DependencyObject
/// {
///     [DependencyProperty]
///     public partial MyViewModel ViewModel { get; set; }
/// }
/// </code>
/// </para>
/// And with this, code analogous to this will be generated:
/// <code>
/// partial class MyControl
/// {
///     public static readonly DependencyProperty ViewModelProperty =
///         DependencyProperty.Register(
///             nameof(ViewModel),
///             typeof(MyViewModel),
///             typeof(MyControl),
///             new PropertyMetadata(default(MyViewModel)));
///
///     public partial MyViewModel ViewModel
///     {
///         get => (MyViewModel)GetValue(ViewModelProperty);
///         set => SetValue(ViewModelProperty, value);
///     }
/// }
/// </code>
/// </summary>
/// <remarks>
/// In order to use this attribute on partial properties, the .NET 9 SDK is required, and C# preview must
/// be used.
/// </remarks>
[AttributeUsage(AttributeTargets.Property, Inherited = false)]
public class DependencyPropertyAttribute : Attribute
{
    public DependencyPropertyAttribute() { }
    public DependencyPropertyAttribute(string propertyChangedCallbackName) { }
}
