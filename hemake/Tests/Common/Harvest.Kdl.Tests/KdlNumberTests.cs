// Copyright Chad Engler

using Harvest.Kdl.Types;

namespace Harvest.Kdl.Tests;

public sealed class KdlNumberTests
{
    [Fact]
    public void Equality_RespectsTypeAnnotations()
    {
        KdlNumber<int> a = new(1, 10, "a");
        KdlNumber<int> b = new(1, 10, "b");

        Assert.NotEqual(a, b);
    }

    [Fact]
    public void Equality_AcrossNumericTypes_DoesNotThrow()
    {
        KdlNumber<int> a = new(1, 10, "x");
        KdlNumber<long> b = new(1, 10, "x");

        Assert.True(a.Equals(b));
    }

    [Fact]
    public void HashCode_IsConsistentWithEquals()
    {
        KdlNumber<int> a = new(1, 10, "x");
        KdlNumber<int> b = new(1, 10, "x");

        Assert.Equal(a, b);
        Assert.Equal(a.GetHashCode(), b.GetHashCode());
    }

    [Fact]
    public void EqualityOperators_WithClrIntegers_WorkInBothDirections()
    {
        KdlValue value = new KdlNumber<int>(1, 10, "x");

        Assert.True(value == 1);
        Assert.True(1 == value);
        Assert.False(value != 1);
        Assert.False(1 != value);
    }
}
