// Copyright Chad Engler

namespace Harvest.Make.Projects.Nodes;

public class NodeValidationResult(bool isValid, object? errorContent = null)
{
    private readonly bool _isValid = isValid;
    public bool IsValid => _isValid;

    private readonly object? _errorContent = errorContent;
    public object? ErrorContent => _errorContent;

    private static readonly NodeValidationResult s_valid = new(true, null);
    public static NodeValidationResult Valid => s_valid;

    public static NodeValidationResult Invalid(object? errorContent)
    {
        return new NodeValidationResult(false, errorContent);
    }

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

        NodeValidationResult? vr = obj as NodeValidationResult;
        if (vr is not null)
        {
            return IsValid == vr.IsValid && ErrorContent == vr.ErrorContent;
        }

        return false;
    }

    public override int GetHashCode()
    {
        return IsValid.GetHashCode() ^ (ErrorContent is null ? int.MinValue : ErrorContent).GetHashCode();
    }
}
