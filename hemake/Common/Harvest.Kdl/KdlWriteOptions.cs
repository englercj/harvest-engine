// Copyright Chad Engler

namespace Harvest.Kdl;

public class KdlWriteOptions
{
    public static KdlWriteOptions PrettyDefault => new KdlWriteOptions();
    public static KdlWriteOptions RawDefault => new KdlWriteOptions() { IndentSize = 0, EscapeNonAscii = false, PrintEmptyChildren = false };

    /// <value>Defines whether CR, LF, backspaces, tabs and form feed must be escaped.</value>
    public bool EscapeCommon { get; set; } = true;

    /// <value>Defines whether non-ASCII characters (Unicode value > 127) must be escaped.</value>
    public bool EscapeNonAscii { get; set; } = false;

    /// <value>Defines whether non-printable ASCII characters (Unicode value < 32) must be escaped.</value>
    public bool EscapeNonPrintableAscii { get; set; } = true;

    /// <value>Defines the exponent character used when printing floating point numbers.</value>
    public char ExponentChar { get; set; } = 'E';

    /// <value>Defines the character used for indentation.</value>
    public char IndentChar { get; set; } = ' ';

    /// <value>Defines the amount of indentation to use.</value>
    public int IndentSize { get; set; } = 4;

    /// <value>Defines whether hexadecimal, octal, and binary numbers should be written using their prefix format (0x, 0o, 0b).</value>
    public bool KeepRadix { get; set; } = false;

    /// <value>Defines the newline string.</value>
    public string Newline { get; set; } = "\n";

    /// <value>Defines whether arguments with a null value should be ignored.</value>
    public bool PrintNullArguments { get; set; } = true;

    /// <value>Defines whether properties with a null value should be ignored.</value>
    public bool PrintNullProperties { get; set; } = true;

    /// <value>Defines whether {} should be printed for nodes with empty children.</value>
    public bool PrintEmptyChildren { get; set; } = true;

    /// <value>Defines whether nodes are terminated with a semicolon.</value>
    public bool TerminateNodesWithSemicolon { get; set; } = false;
}
