// Copyright Chad Engler

using Harvest.Kdl.Types;
using Harvest.Kdl.Exceptions;
using System.Numerics;

namespace Harvest.Kdl.Tests;

public sealed class KdlParserRegressionTests
{
    [Fact]
    public void ParsesOversizedHexIntegerAsBigInteger()
    {
        KdlDocument doc = KdlDocument.FromString("hex_int.kdl", "node 0xABCDEF0123456789abcdef\n");

        KdlNumber<BigInteger> value = Assert.IsType<KdlNumber<BigInteger>>(doc.Nodes[0].Arguments[0]);
        Assert.Equal(BigInteger.Parse("207698809136909011942886895"), value.Value);
    }

    [Fact]
    public void ParsesExponentWithUnderscoreWithoutUnderflowingToZero()
    {
        KdlDocument doc = KdlDocument.FromString("underscore_exp.kdl", "node 1.0e-10_0\n");

        KdlNumber<double> value = Assert.IsType<KdlNumber<double>>(doc.Nodes[0].Arguments[0]);
        Assert.True(value.Value > 0.0);
        Assert.Equal(1e-100, value.Value);
    }

    [Fact]
    public void ParsesNegativeHexBeyondInt64AsBigInteger()
    {
        KdlDocument doc = KdlDocument.FromString("neg_hex_big.kdl", "node -0xffffffffffffffff\n");

        KdlNumber<BigInteger> value = Assert.IsType<KdlNumber<BigInteger>>(doc.Nodes[0].Arguments[0]);
        Assert.Equal(BigInteger.Parse("-18446744073709551615"), value.Value);
    }

    [Fact]
    public void ParsesDecimalNegativeLongMinValue()
    {
        KdlDocument doc = KdlDocument.FromString("long_min.kdl", "node -9223372036854775808\n");

        KdlNumber<long> value = Assert.IsType<KdlNumber<long>>(doc.Nodes[0].Arguments[0]);
        Assert.Equal(long.MinValue, value.Value);
    }

    [Fact]
    public void RejectsBareBackslashBetweenArguments()
    {
        Assert.Throws<KdlParseException>(() =>
            KdlDocument.FromString("bare_backslash.kdl", "node 1 \\ 2\n"));
    }

    [Fact]
    public void AcceptsEscapedNewlineBetweenArguments()
    {
        KdlDocument doc = KdlDocument.FromString("escaped_newline_args.kdl", "node 1 \\\n 2\n");

        Assert.Equal(2, doc.Nodes[0].Arguments.Count);
        Assert.Equal(1, Assert.IsType<KdlNumber<int>>(doc.Nodes[0].Arguments[0]).Value);
        Assert.Equal(2, Assert.IsType<KdlNumber<int>>(doc.Nodes[0].Arguments[1]).Value);
    }

    [Fact]
    public void RejectsMissingWhitespaceBeforeChildBlockAfterArgument()
    {
        Assert.Throws<KdlParseException>(() =>
            KdlDocument.FromString("missing_ws_before_child.kdl", "node 1{ child }\n"));
    }

    [Fact]
    public void ParsesUntypedFloatAsDouble()
    {
        KdlDocument doc = KdlDocument.FromString("double_default.kdl", "node 0.1\n");
        _ = Assert.IsType<KdlNumber<double>>(doc.Nodes[0].Arguments[0]);
    }

    [Fact]
    public void ParsesUsizeWithoutTruncation()
    {
        const string input = "node (usize)4294967296\n";
        if (nuint.Size == 4)
        {
            Assert.Throws<OverflowException>(() => KdlDocument.FromString("usize_overflow.kdl", input));
            return;
        }

        KdlDocument doc = KdlDocument.FromString("usize_no_overflow.kdl", input);
        KdlNumber<nuint> value = Assert.IsType<KdlNumber<nuint>>(doc.Nodes[0].Arguments[0]);
        ulong expected = 4294967296UL;
        Assert.Equal((nuint)expected, value.Value);
    }
}
