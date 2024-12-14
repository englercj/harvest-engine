// Copyright Chad Engler

namespace Harvest.Make.Projects;

public class NodeValidationResult(bool isValid, object? errorContent = null)
{
    public bool IsValid => isValid;
    public object? ErrorContent => errorContent;

    public static NodeValidationResult Valid { get; } = new(true, null);
    public static NodeValidationResult Error(object? errorContent) => new NodeValidationResult(false, errorContent);

    public static bool operator ==(NodeValidationResult left, NodeValidationResult right)
    {
        return Equals(left, right);
    }

    public static bool operator !=(NodeValidationResult left, NodeValidationResult right)
    {
        return !Equals(left, right);
    }

    public override bool Equals(object? obj)
    {
        // A cheaper alternative to Object.ReferenceEquals() is used here for better perf
        if (obj == (object)this)
        {
            return true;
        }

        if (obj is NodeValidationResult vr)
        {
            return IsValid == vr.IsValid && ErrorContent == vr.ErrorContent;
        }

        return false;
    }

    public override int GetHashCode()
    {
        return IsValid.GetHashCode() ^ (ErrorContent is null ? int.MinValue : ErrorContent.GetHashCode());
    }
}
