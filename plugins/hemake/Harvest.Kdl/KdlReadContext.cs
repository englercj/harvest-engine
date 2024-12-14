// Copyright Chad Engler

using Harvest.Kdl.Exceptions;
using System.Text;

namespace Harvest.Kdl;

public class KdlReadContext(string filePath, TextReader reader)
{
    public string FilePath => filePath;
    public PushBackReader Reader { get; } = new PushBackReader(reader, 2);

    public uint Line { get; private set; } = 1;
    public uint Column { get; private set; } = 1;
    public ulong Offset { get; private set; } = 0;

    private readonly char[] _char = new char[1];

    public int Read()
    {
        int ucc = Reader.Read();

        if (ucc == KdlUtils.EOF)
        {
            throw new KdlException("Unexpected end of file.", this);
        }

        if (KdlUtils.IsDisallowed(ucc))
        {
            throw new KdlException("Disallowed character found.", this);
        }

        _char[0] = (char)ucc;
        uint bytes = (uint)Encoding.UTF8.GetByteCount(_char);

        Offset += bytes;

        if (KdlUtils.IsNewline(ucc))
        {
            if (ucc == 0x000D && Reader.Peek() == 0x000A) // CRLF
            {
                ucc = Reader.Read();
                ++Offset;
            }

            Column = 1;
            ++Line;
        }
        else
        {
            ++Column;
        }

        return ucc;
    }

    public int Peek()
    {
        return Reader.Peek();
    }

    public void Unread(int ucc)
    {
        if (KdlUtils.IsNewline(ucc))
        {
            throw new KdlException("Attempted to Unread() a newline.", null);
        }
        else if (ucc == KdlUtils.EOF)
        {
            throw new KdlException("Attempted to Unread() EOF.", null);
        }
        else
        {
            _char[0] = (char)ucc;
            uint bytes = (uint)Encoding.UTF8.GetByteCount(_char);

            Column -= bytes;
            Offset -= bytes;
        }

        try
        {
            Reader.Unread(ucc);
        }
        catch (IOException)
        {
            throw new KdlException("Attempted to unread more than 2 characters in sequence", null);
        }
    }
}
