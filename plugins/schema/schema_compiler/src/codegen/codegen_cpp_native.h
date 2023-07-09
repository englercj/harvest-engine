// Copyright Chad Engler

#pragma once

#include "he/core/rb_tree.h"
#include "he/core/string_builder.h"
#include "he/core/string.h"
#include "he/core/string_view.h"
#include "he/core/types.h"
#include "he/schema/codegen.h"
#include "he/schema/schema.h"

namespace he::schema
{
    class CodeGenCppNative
    {
    public:
        explicit CodeGenCppNative(const CodeGenRequest& request) noexcept;

        bool Generate();

    private:
        void GenHeader();

        void WriteForwardDecl(Declaration::Reader decl, Declaration::Reader scope);
        void WriteDecl(Declaration::Reader decl, Declaration::Reader scope);
        void WriteAttributeDecl(Declaration::Reader decl, Declaration::Reader scope);
        void WriteConstDecl(Declaration::Reader decl, Declaration::Reader scope);
        void WriteEnumDecl(Declaration::Reader decl, Declaration::Reader scope);
        void WriteInterfaceDecl(Declaration::Reader decl, Declaration::Reader scope);
        void WriteStructDecl(Declaration::Reader decl, Declaration::Reader scope);

        void WriteDeclInfo(Declaration::Reader decl);

    private:
        void GenSource();

        void WriteDeclSrc(Declaration::Reader decl);
        void WriteConstDeclSrc(Declaration::Reader decl);
        void WriteStructDeclSrc(Declaration::Reader decl);

        void WriteDeclInfoSrc(Declaration::Reader decl);
        void WriteEnumStrings(Declaration::Reader decl);
        void WriteRawSchemaData();
        void WriteFieldGetValue(Field::Reader field, Declaration::Data::Struct::Reader structDecl, const he::String& className);

    private:
        void WriteName(Declaration::Reader decl, Declaration::Reader scope, Brand::Reader brand);
        void WriteTemplate(Declaration::Reader decl);
        void WriteTemplateBrand(Declaration::Reader decl);
        void WriteType(Type::Reader type, Declaration::Reader scope);
        void WriteDataValue(Type::Reader type, Declaration::Reader scope, Value::Reader value);
        void WriteWithReplace(StringView input, char what, StringView with);

        bool NeedsConstructor(Declaration::Data::Struct::Reader structDecl);

        bool FlushToFile(const char* suffix);
        void FindAllDependencies(Type::Reader type, RBTreeSet<TypeId>& out);
        void FindAllDependencies(Declaration::Reader decl, RBTreeSet<TypeId>& out);

    private:
        const CodeGenRequest& m_request;
        Declaration::Reader m_root{};
        StringBuilder m_writer{};
        he::String m_namespaceName{};
    };
}
