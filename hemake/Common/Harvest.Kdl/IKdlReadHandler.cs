// Copyright Chad Engler

namespace Harvest.Kdl;

public interface IKdlReadHandler
{
    /// Called at the start of reading the document.
    void StartDocument(KdlSourceInfo source);

    /// Called at the end of reading the document.
    void EndDocument();

    /// Called if a document version marker is specified.
    ///
    /// \param[in] value The version value.
    void Version(string value, KdlSourceInfo source);

    /// Called when a single-line (`//`) or multi-line (`/**/`) comment is encountered.
    ///
    /// \param[in] value The comment value.
    void Comment(string value, KdlSourceInfo source);

    /// Called when a slashdash comment starts. This may be followed by any number of
    /// calls to \ref StartNode, \ref EndNode, \ref Argument, or \ref Property which
    /// should all be treated as "commented out" until \ref EndComment is called.
    void StartComment(KdlSourceInfo source);

    /// Called when the components of a slashdash comment have been read.
    void EndComment();

    /// Called when a node is encountered. Additional calls to \ref StartNode before
    /// \ref EndNode are children of this node.
    ///
    /// \param[in] name The name of the node.
    /// \param[in] type The type annotation, if any, on the node.
    void StartNode(string name, string? type, KdlSourceInfo source);

    /// Called when the end of a node is encountered.
    void EndNode();

    /// Called when an argument to a node is encountered.
    ///
    /// \param[in] value The argument value.
    void Argument(KdlValue value, KdlSourceInfo source);

    /// Called when a property is encountered.
    ///
    /// \param[in] name The name of the property.
    /// \param[in] value The value of the property.
    void Property(string name, KdlValue value, KdlSourceInfo source);
}
