// Copyright Chad Engler

#pragma once

#include "he/core/string_view.h"
#include "he/core/types.h"
#include "he/schema/codegen.h"
#include "he/schema/code_writer.h"
#include "he/schema/schema.h"

#include <unordered_map>

namespace he::schema
{
    class CodeGenEcho
    {
    public:
        CodeGenEcho(const CodeGenRequest& request);

        bool Generate();

    private:
        void WriteDecl(const Declaration& decl, const Declaration& scope);
        void WriteAttributeDecl(const Declaration& decl, const Declaration& scope);
        void WriteConstDecl(const Declaration& decl, const Declaration& scope);
        void WriteEnumDecl(const Declaration& decl, const Declaration& scope);
        void WriteInterfaceDecl(const Declaration& decl, const Declaration& scope);
        void WriteStructDecl(const Declaration& decl, const Declaration& scope);

        void WriteAttribute(const Attribute& attribute, const Declaration& scope);
        void WriteAttributes(Span<const Attribute> attributes, const Declaration& scope);
        void WriteField(const Field& field, const Declaration& scope);
        void WriteName(const Declaration& decl, const Declaration& scope, const Brand& brand);
        void WriteTuple(const Declaration& decl);
        void WriteTypeParams(Span<const String> typeParams);
        void WriteType(const Type& type, const Declaration& scope);
        void WriteValue(const Type& type, const Declaration& scope, const Value& value);

    private:
        const CodeGenRequest& m_request;
        CodeWriter m_writer;
    };
}
