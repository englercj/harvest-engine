// Copyright Chad Engler

#pragma once

#include "he/core/string_builder.h"
#include "he/core/string.h"
#include "he/core/string_view.h"
#include "he/core/types.h"
#include "he/schema/codegen.h"
#include "he/schema/schema.h"

#include <set>

namespace he::schema
{
    class CodeGenCpp
    {
    public:
        explicit CodeGenCpp(const CodeGenRequest& request) noexcept;

        bool Generate();

    // Header Generation
    private:
        void GenHeader();

        void WriteDecl(Declaration::Reader decl, Declaration::Reader scope);
        void WriteAttributeDecl(Declaration::Reader decl, Declaration::Reader scope);
        void WriteConstDecl(Declaration::Reader decl, Declaration::Reader scope);
        void WriteEnumDecl(Declaration::Reader decl, Declaration::Reader scope);
        void WriteInterfaceDecl(Declaration::Reader decl, Declaration::Reader scope);
        void WriteStructDecl(Declaration::Reader decl, Declaration::Reader scope);

        void WriteImpl(Declaration::Reader decl, Declaration::Reader scope);
        void WriteInterfaceImpl(Declaration::Reader decl, Declaration::Reader scope);
        void WriteStructImpl(Declaration::Reader decl, Declaration::Reader scope);
        void WriteFieldImpl(Declaration::Reader decl, Declaration::Reader scope);

        void WriteFieldGetDecl(Field::Reader field, Declaration::Reader decl, bool isReader);
        void WriteFieldGetImpl(Field::Reader field, Declaration::Reader decl, Declaration::Reader scope, bool isReader);
        void WriteFieldSetDecl(Field::Reader field, Declaration::Reader decl);
        void WriteFieldSetImpl(Field::Reader field, Declaration::Reader decl, Declaration::Reader scope);
        void WriteGroupFieldClear(Declaration::Reader decl, Declaration::Reader scope);

    // Source Generation
    private:
        void GenSource();

        void WriteRawSchemaData();
        void WriteDeclInfoSrc(Declaration::Reader decl);
        void WriteEnumStrings(Declaration::Reader decl);

    // Utilities
    private:
        bool FlushToFile(const char* suffix);

        void WriteDeclInfo(Declaration::Reader decl);
        void WriteName(Declaration::Reader decl, Declaration::Reader scope, Brand::Reader brand, const char* pointerSuffix);
        void WriteTemplate(Declaration::Reader decl);
        void WriteType(Type::Reader type, Declaration::Reader scope, const char* pointerSuffix);
        void WriteDataValue(Type::Reader type, Declaration::Reader scope, Value::Reader value);
        void WriteWithReplace(StringView input, char what, StringView with);

        void FindAllDependencies(Type::Reader type, std::set<TypeId>& out);
        void FindAllDependencies(Declaration::Reader decl, std::set<TypeId>& out);

    private:
        const CodeGenRequest& m_request;
        Declaration::Reader m_root{};
        StringBuilder m_writer{};
        he::String m_namespaceName{};
    };
}
