// Copyright Chad Engler

#pragma once

#include "he/core/types.h"
#include "he/schema/codegen.h"
#include "he/schema/code_writer.h"
#include "he/schema/schema.h"

namespace he::schema
{
    class CodeGenEcho
    {
    public:
        CodeGenEcho(const CodeGenRequest& request);

        bool Generate();

    private:
        void WriteDecl(Declaration::Reader decl, Declaration::Reader scope);
        void WriteAttributeDecl(Declaration::Reader decl, Declaration::Reader scope);
        void WriteConstDecl(Declaration::Reader decl, Declaration::Reader scope);
        void WriteEnumDecl(Declaration::Reader decl, Declaration::Reader scope);
        void WriteInterfaceDecl(Declaration::Reader decl, Declaration::Reader scope);
        void WriteStructDecl(Declaration::Reader decl, Declaration::Reader scope);

        void WriteAttribute(Attribute::Reader attribute, Declaration::Reader scope);
        void WriteAttributes(List<Attribute>::Reader attributes, Declaration::Reader scope);
        void WriteField(Field::Reader field, Declaration::Reader scope);
        void WriteName(Declaration::Reader decl, Declaration::Reader scope, Brand::Reader brand);
        void WriteTuple(Declaration::Reader decl);
        void WriteTypeParams(List<String>::Reader typeParams);
        void WriteType(Type::Reader type, Declaration::Reader scope);
        void WriteValue(Type::Reader type, Declaration::Reader scope, Value::Reader value);

    private:
        const CodeGenRequest& m_request;
        CodeWriter m_writer;
    };
}
