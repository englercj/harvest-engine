// Copyright Chad Engler

namespace Harvest.Make;

public class StreamUtils
{
    public static async Task CopyStreamToFileAsync(Stream source, string filePath)
    {
        long originalPosition = 0;
        if (source.CanSeek)
        {
            originalPosition = source.Position;
            source.Position = 0;
        }

        await using FileStream fileStream = new(filePath, FileMode.Create, FileAccess.Write, FileShare.None);
        await source.CopyToAsync(fileStream);

        if (source.CanSeek)
        {
            source.Position = originalPosition;
        }
    }

    public static async Task<bool> CopyStreamToFileIfChangedAsync(MemoryStream source, string filePath)
    {
        bool fileHasChanged;
        try
        {
            FileStream existingFile = File.OpenRead(filePath);
            fileHasChanged = AreStreamsEqual(source, existingFile);
        }
        catch (FileNotFoundException)
        {
            fileHasChanged = true;
        }

        if (fileHasChanged)
        {
            await CopyStreamToFileAsync(source, filePath);
        }

        return fileHasChanged;
    }

    public static bool IsStreamEqualToBytes(MemoryStream stream, byte[] bytes)
    {
        if (stream.TryGetBuffer(out ArraySegment<byte> bufferSegment))
        {
            if (bufferSegment.Count != bytes.Length || bufferSegment.Array is null)
            {
                return false;
            }

            for (int i = 0; i < bufferSegment.Count; ++i)
            {
                if (bufferSegment.Array[bufferSegment.Offset + i] != bytes[i])
                {
                    return false;
                }
            }

            return true;
        }
        else
        {
            long originalPosition = 0;
            if (stream.CanSeek)
            {
                originalPosition = stream.Position;
                stream.Position = 0;
            }

            // If TryGetBuffer fails (e.g., stream was resized or not constructed from an array),
            // we need to read from the stream.
            bool isEqual = CompareMemoryStreamToByteArrayByReading(stream, bytes);

            if (stream.CanSeek)
            {
                stream.Position = originalPosition;
            }

            return isEqual;
        }
    }

    public static bool AreStreamsEqual(Stream a, Stream b)
    {
        long originalPositionA = 0;
        if (a.CanSeek)
        {
            originalPositionA = a.Position;
            a.Position = 0;
        }

        long originalPositionB = 0;
        if (b.CanSeek)
        {
            originalPositionB = b.Position;
            b.Position = 0;
        }

        try
        {
            return CompareStreamsByReading(a, b);
        }
        finally
        {
            if (a.CanSeek)
            {
                a.Position = originalPositionA;
            }

            if (b.CanSeek)
            {
                b.Position = originalPositionB;
            }
        }
    }

    private static bool CompareStreamsByReading(Stream a, Stream b)
    {
        if (a == b)
        {
            return true;
        }

        if (a.CanSeek && b.CanSeek)
        {
            if (a.Length != b.Length)
            {
                return false;
            }
        }

        const int bufferSize = 4096;
        byte[] bufferA = new byte[bufferSize];
        byte[] bufferB = new byte[bufferSize];

        while (true)
        {
            int countA = a.Read(bufferA, 0, bufferSize);
            int countB = b.Read(bufferB, 0, bufferSize);

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

            Span<byte> spanA = bufferA.AsSpan(0, countA);
            Span<byte> spanB = bufferB.AsSpan(0, countB);
            if (!spanA.SequenceEqual(spanB))
            {
                return false;
            }
        }
    }

    private static bool CompareMemoryStreamToByteArrayByReading(MemoryStream stream, byte[] bytes)
    {
        if (stream.Length != bytes.Length)
        {
            return false;
        }

        for (int i = 0; i < bytes.Length; ++i)
        {
            int readByte = stream.ReadByte();
            if (readByte == -1 || (byte)readByte != bytes[i])
            {
                return false;
            }
        }

        return true;
    }
}
