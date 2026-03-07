// Copyright Chad Engler

namespace Harvest.Kdl.Tests;

public sealed class KdlReadOptionsTests
{
    private sealed class CommentTrackingHandler : IKdlReadHandler
    {
        public int Comments { get; private set; }
        public List<string> CommentValues { get; } = [];

        public void StartDocument(KdlSourceInfo source) { }
        public void EndDocument() { }
        public void Version(string value, KdlSourceInfo source) { }

        public void Comment(string value, KdlSourceInfo source)
        {
            ++Comments;
            CommentValues.Add(value);
        }

        public void StartComment(KdlSourceInfo source) { }
        public void EndComment() { }
        public void StartNode(string name, string? type, KdlSourceInfo source) { }
        public void EndNode() { }
        public void Argument(KdlValue value, KdlSourceInfo source) { }
        public void Property(string name, KdlValue value, KdlSourceInfo source) { }
    }

    [Fact]
    public void ReadString_DoesNotEmitComments_ByDefault()
    {
        CommentTrackingHandler handler = new();

        KdlReader.ReadString(
            """
            // header
            node 1 /* inline */
            """,
            handler,
            filePath: "comments.kdl");

        Assert.Equal(0, handler.Comments);
    }

    [Fact]
    public void ReadString_EmitsComments_WhenCaptureCommentsEnabled()
    {
        CommentTrackingHandler handler = new();

        KdlReader.ReadString(
            """
            // header
            node 1 /* inline */
            """,
            handler,
            filePath: "comments.kdl",
            options: new KdlReadOptions { CaptureComments = true });

        Assert.Equal(2, handler.Comments);
    }

    [Fact]
    public void ReadString_CapturesEmptyMultilineComment_WithoutNulCharacters()
    {
        CommentTrackingHandler handler = new();

        KdlReader.ReadString(
            "node /**/ 1\n",
            handler,
            filePath: "comments.kdl",
            options: new KdlReadOptions { CaptureComments = true });

        Assert.Single(handler.CommentValues);
        Assert.Equal(string.Empty, handler.CommentValues[0]);
        Assert.DoesNotContain('\0', handler.CommentValues[0]);
    }

    [Fact]
    public void ReadString_CapturesNestedMultilineComment_Text()
    {
        CommentTrackingHandler handler = new();

        KdlReader.ReadString(
            "node /* a /* b */ c */ 1\n",
            handler,
            filePath: "comments.kdl",
            options: new KdlReadOptions { CaptureComments = true });

        Assert.Single(handler.CommentValues);
        Assert.Equal("a /* b */ c", handler.CommentValues[0]);
    }
}
