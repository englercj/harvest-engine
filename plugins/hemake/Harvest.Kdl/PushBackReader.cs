// Copyright Chad Engler

namespace Harvest.Kdl;

public class PushBackReader
{
    private readonly TextReader _reader;
    private readonly int[] _queue;
    private int _position = 0;

    public PushBackReader(TextReader reader, int pushbackLimit)
    {
        _reader = reader;
        _queue = new int[pushbackLimit];
    }

    public int Peek()
    {
        if (_position > 0)
        {
            return _queue[_position - 1];
        }

        return _reader.Peek();
    }

    public int Read()
    {
        if (_position > 0)
        {
            return _queue[(_position--) - 1];
        }

        return _reader.Read();
    }

    public void Unread(int value)
    {
        if (_position >= _queue.Length)
        {
            throw new IOException("PushBackReader buffer length exceeded");
        }

        _queue[_position++] = value;
    }
}
