using System.Buffers;

namespace Harvest.Common;

public struct PooledBuffer<T>(ArrayPool<T> pool, int length) : IDisposable
{
    private T[]? _array = pool.Rent(length);

    public readonly int Length => length;
    public readonly T[] Array => _array ?? [];

    public readonly Span<T> AsSpan() => _array.AsSpan(0, Length);
    public readonly Span<T> AsSpan(Index start)
    {
        int startOffset = start.GetOffset(Length);
        return _array.AsSpan(startOffset, Length - startOffset);
    }

    public readonly Span<T> AsSpan(Range range)
    {
        (int startOffset, int rangeLength) = range.GetOffsetAndLength(Length);
        return _array.AsSpan(startOffset, rangeLength);
    }

    public readonly Span<T> AsSpan(int start, int length)
    {
        if ((uint)start > (uint)Length || (uint)length > (uint)(Length - start))
        {
            throw new ArgumentOutOfRangeException();
        }

        return _array.AsSpan(start, length);
    }

    public readonly Memory<T> AsMemory() => _array.AsMemory(0, Length);
    public readonly Memory<T> AsMemory(Index start)
    {
        int startOffset = start.GetOffset(Length);
        return _array.AsMemory(startOffset, Length - startOffset);
    }

    public readonly Memory<T> AsMemory(Range range)
    {
        (int startOffset, int rangeLength) = range.GetOffsetAndLength(Length);
        return _array.AsMemory(startOffset, rangeLength);
    }

    public readonly Memory<T> AsMemory(int start, int length)
    {
        if ((uint)start > (uint)Length || (uint)length > (uint)(Length - start))
        {
            throw new ArgumentOutOfRangeException();
        }

        return _array.AsMemory(start, length);
    }

    public static implicit operator Span<T>(PooledBuffer<T> buffer) => buffer.AsSpan();
    public static implicit operator ReadOnlySpan<T>(PooledBuffer<T> buffer) => buffer.AsSpan();

    public static implicit operator Memory<T>(PooledBuffer<T> buffer) => buffer.AsMemory();
    public static implicit operator ReadOnlyMemory<T>(PooledBuffer<T> buffer) => buffer.AsMemory();

    public void Dispose()
    {
        if (_array is not null)
        {
            pool.Return(_array);
            _array = null;
        }
        GC.SuppressFinalize(this);
    }
}
