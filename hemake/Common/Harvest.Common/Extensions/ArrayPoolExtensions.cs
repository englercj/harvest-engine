using System.Buffers;

namespace Harvest.Common.Extensions;

public static class ArrayPoolExtensions
{
    public static PooledBuffer<T> RentBuffer<T>(this ArrayPool<T> pool, int minimumLength)
    {
        return new PooledBuffer<T>(pool, minimumLength);
    }
}
