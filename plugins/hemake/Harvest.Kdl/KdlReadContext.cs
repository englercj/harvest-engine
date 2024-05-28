// Copyright Chad Engler

using Harvest.Kdl.Exceptions;
using System.Text;

namespace Harvest.Kdl;

public class KdlReadContext
{
    public PushBackReader Reader { get; }

    public uint Line { get; private set; }
    public uint Column { get; private set; }
    public ulong Offset { get; private set; }

    private char[] _char;

    public KdlReadContext(TextReader reader)
    {
        Reader = new PushBackReader(reader, 2);

        Line = 1;
        Column = 1;
        Offset = 0;

        _char = new char[1];
    }

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
