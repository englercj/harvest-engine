// Copyright Chad Engler

#include "he/schema/ast.h"

#include "he/core/enum_ops.h"

namespace he
{
    const char* AsString(schema::AstExpression::Kind x)
    {
        switch (x)
        {
            case schema::AstExpression::Kind::Unknown: return "Unknown";
            case schema::AstExpression::Kind::Array: return "Array";
            case schema::AstExpression::Kind::Blob: return "Blob";
            case schema::AstExpression::Kind::Float: return "Float";
            case schema::AstExpression::Kind::Generic: return "Generic";
            case schema::AstExpression::Kind::List: return "List";
            case schema::AstExpression::Kind::Identifier: return "Identifier";
            case schema::AstExpression::Kind::Namespace: return "Namespace";
            case schema::AstExpression::Kind::Sequence: return "Sequence";
            case schema::AstExpression::Kind::SignedInt: return "SignedInt";
            case schema::AstExpression::Kind::String: return "String";
            case schema::AstExpression::Kind::Tuple: return "Tuple";
            case schema::AstExpression::Kind::UnsignedInt: return "UnsignedInt";
            case schema::AstExpression::Kind::QualifiedName: return "QualifiedName";
        }
        return "<unknown>";
    }

    const char* AsString(schema::AstMethodParams::Kind x)
    {
        switch (x)
        {
            case schema::AstMethodParams::Kind::Type: return "Type";
            case schema::AstMethodParams::Kind::Fields: return "Fields";
        }
        return "<unknown>";
    }

    const char* AsString(schema::AstNode::Kind x)
    {
        switch (x)
        {
            case schema::AstNode::Kind::Alias: return "Alias";
            case schema::AstNode::Kind::Attribute: return "Attribute";
            case schema::AstNode::Kind::Constant: return "Constant";
            case schema::AstNode::Kind::Enum: return "Enum";
            case schema::AstNode::Kind::Enumerator: return "Enumerator";
            case schema::AstNode::Kind::Field: return "Field";
            case schema::AstNode::Kind::File: return "File";
            case schema::AstNode::Kind::Group: return "Group";
            case schema::AstNode::Kind::Interface: return "Interface";
            case schema::AstNode::Kind::Method: return "Method";
            case schema::AstNode::Kind::Struct: return "Struct";
            case schema::AstNode::Kind::Union: return "Union";
        }
        return "<unknown>";
    }
}
