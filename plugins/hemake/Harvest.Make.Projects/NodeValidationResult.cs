// Copyright Chad Engler

namespace Harvest.Make.Projects;

public class NodeValidationResult(bool isValid, object? errorContent = null)
{
    public bool IsValid => isValid;
    public object? ErrorContent => errorContent;

    public static NodeValidationResult Valid { get; } = new(true, null);
    public static NodeValidationResult Error(object? errorContent) => new NodeValidationResult(false, errorContent);

    public override int GetHashCode() => HashCode.Combine(IsValid, ErrorContent);

    public override bool Equals(object? other) => Equals(other as NodeValidationResult);

    public bool Equals(NodeValidationResult? other)
    {
        if (other is null)
        {
            return false;
        }

        if (ReferenceEquals(this, other))
        {
            return true;
        }

        if (GetType() != other.GetType())
        {
            return false;
        }

        return IsValid == other.IsValid && ErrorContent == other.ErrorContent;
    }

    public static bool operator ==(NodeValidationResult? lhs, NodeValidationResult? rhs)
    {
        if (lhs is null)
        {
            if (rhs is null)
            {
                return true;
            }

            return false;
        }

        return lhs.Equals(rhs);
    }

    public static bool operator !=(NodeValidationResult? lhs, NodeValidationResult? rhs) => !(lhs == rhs);
}
