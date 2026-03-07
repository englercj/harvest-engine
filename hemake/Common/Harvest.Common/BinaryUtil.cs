using System.Numerics;

namespace Harvest.Common;

public static class BinaryUtil
{
    public static bool IsPowerOfTwo<T>(T value) where T : IBinaryInteger<T>
    {
        if (value <= T.Zero)
        {
            return false;
        }

        return (value & (value - T.One)) == T.Zero;
    }

    public static T AlignUp<T>(T value, T alignment) where T : IBinaryInteger<T>
    {
        if (!IsPowerOfTwo(alignment))
        {
            throw new ArgumentOutOfRangeException(nameof(alignment), "Must be power of two.");
        }

        T mask = alignment - T.One;
        return (value + mask) & ~mask;
    }
}
