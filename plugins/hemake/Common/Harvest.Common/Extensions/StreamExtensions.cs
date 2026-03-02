// Copyright Chad Engler

using CommunityToolkit.HighPerformance;
using System.Buffers;

namespace Harvest.Common.Extensions;

public static class StreamExtensions
{
    public static async Task CopyToFileAsync(this Stream stream, string filePath)
    {
        await using FileStream fileStream = new(filePath, FileMode.Create, FileAccess.Write, FileShare.None);
        await CopyToFileAsync(stream, fileStream);
    }

    public static async Task<bool> CopyToFileIfChangedAsync(this Stream stream, string filePath)
    {
        bool fileHasChanged;
        try
        {
            await using FileStream existingFile = new(filePath, FileMode.Open, FileAccess.ReadWrite, FileShare.None);
            fileHasChanged = !stream.EqualsStream(existingFile);

            if (fileHasChanged)
            {
                await CopyToFileAsync(stream, existingFile);
            }
        }
        catch (FileNotFoundException)
        {
            fileHasChanged = true;
            await stream.CopyToFileAsync(filePath);
        }

        return fileHasChanged;
    }

    public static bool EqualsBytes(this Stream stream, ReadOnlySpan<byte> bytes)
    {
        // Fast path for memory streams that can expose their buffer directly.
        if (stream is MemoryStream mem && mem.TryGetBuffer(out ArraySegment<byte> bufferSegment))
        {
            if (bufferSegment.Count != bytes.Length || bufferSegment.Array is null)
            {
                return false;
            }

            ReadOnlySpan<byte> span = bufferSegment.AsSpan();
            return span.SequenceEqual(bytes);
        }

        // If we don't have a MemoryStream or TryGetBuffer fails, we need to read from the stream to compare.
        long originalPosition = 0;
        if (stream.CanSeek)
        {
            originalPosition = stream.Position;
            stream.Position = 0;
        }

        try
        {
            return CompareStreamToBytesByReading(stream, bytes);
        }
        finally
        {
            if (stream.CanSeek)
            {
                stream.Position = originalPosition;
            }
        }
    }

    public static bool EqualsStream(this Stream stream, Stream other)
    {
        if (ReferenceEquals(stream, other))
        {
            return true;
        }

        // Fast path for comparing memory streams that can expose their buffers directly.
        if (stream is MemoryStream memA && memA.TryGetBuffer(out ArraySegment<byte> bytesA)
            && other is MemoryStream memB && memB.TryGetBuffer(out ArraySegment<byte> bytesB))
        {
            if (bytesA.Count != bytesB.Count || bytesA.Array is null || bytesB.Array is null)
            {
                return false;
            }

            ReadOnlySpan<byte> spanA = bytesA.AsSpan();
            ReadOnlySpan<byte> spanB = bytesB.AsSpan();
            return spanA.SequenceEqual(spanB);
        }

        long originalPositionA = 0;
        if (stream.CanSeek)
        {
            originalPositionA = stream.Position;
            stream.Position = 0;
        }

        long originalPositionB = 0;
        if (other.CanSeek)
        {
            originalPositionB = other.Position;
            other.Position = 0;
        }

        try
        {
            return CompareStreamsByReading(stream, other);
        }
        finally
        {
            if (stream.CanSeek)
            {
                stream.Position = originalPositionA;
            }

            if (other.CanSeek)
            {
                other.Position = originalPositionB;
            }
        }
    }

    private static async Task CopyToFileAsync(Stream source, FileStream fileStream)
    {
        long originalPosition = 0;
        if (source.CanSeek)
        {
            originalPosition = source.Position;
            source.Position = 0;
        }

        if (fileStream.CanSeek)
        {
            fileStream.Position = 0;
            fileStream.SetLength(0);
        }

        await source.CopyToAsync(fileStream);
        await fileStream.FlushAsync();

        if (source.CanSeek)
        {
            source.Position = originalPosition;
        }
    }

    private static bool CompareStreamsByReading(Stream a, Stream b)
    {
        if (a.CanSeek && b.CanSeek)
        {
            if (a.Length != b.Length)
            {
                return false;
            }
        }

        const int bufferSize = 4096;
        using PooledBuffer<byte> bufferA = ArrayPool<byte>.Shared.RentBuffer(bufferSize);
        using PooledBuffer<byte> bufferB = ArrayPool<byte>.Shared.RentBuffer(bufferSize);

        while (true)
        {
            int countA = a.Read(bufferA);
            int countB = b.Read(bufferB);

            // If the amount of bytes read differs, they are different
            // (Provided we assume reliable reads, typical for FileStreams)
            if (countA != countB)
            {
                return false;
            }

            // If we reached the end of both streams, they are equal
            if (countA == 0)
            {
                return true;
            }

            ReadOnlySpan<byte> spanA = bufferA.AsSpan(..countA);
            ReadOnlySpan<byte> spanB = bufferB.AsSpan(..countB);
            if (!spanA.SequenceEqual(spanB))
            {
                return false;
            }
        }
    }

    private static bool CompareStreamToBytesByReading(Stream stream, ReadOnlySpan<byte> bytes)
    {
        if (stream.CanSeek)
        {
            if (stream.Length != bytes.Length)
            {
                return false;
            }
        }

        const int bufferSize = 4096;
        using PooledBuffer<byte> buffer = ArrayPool<byte>.Shared.RentBuffer(bufferSize);

        int offset = 0;
        while (true)
        {
            int count = stream.Read(buffer);
            if (count == 0)
            {
                return true;
            }

            ReadOnlySpan<byte> spanA = buffer.AsSpan(..count);
            ReadOnlySpan<byte> spanB = bytes.Slice(offset, count);
            if (!spanA.SequenceEqual(spanB))
            {
                return false;
            }

            offset += count;
        }
    }
}
