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
    class CodeGenCpp
    {
    public:
        CodeGenCpp(const CodeGenRequest& request);

        bool Generate();

    // Header Generation
    private:
        void GenHeader();

        void WriteDecl(const Declaration& decl, const Declaration& scope);
        void WriteAttributeDecl(const Declaration& decl, const Declaration& scope);
        void WriteConstDecl(const Declaration& decl, const Declaration& scope);
        void WriteEnumDecl(const Declaration& decl, const Declaration& scope);
        void WriteInterfaceDecl(const Declaration& decl, const Declaration& scope);
        void WriteStructDecl(const Declaration& decl, const Declaration& scope);

        void WriteImpl(const Declaration& decl, const Declaration& scope);
        void WriteInterfaceImpl(const Declaration& decl, const Declaration& scope);
        void WriteStructImpl(const Declaration& decl, const Declaration& scope);
        void WriteFieldImpl(const Declaration& decl, const Declaration& scope);

        void WriteFieldGetDecl(const Field& field, const Declaration& decl, bool isReader);
        void WriteFieldGetImpl(const Field& field, const Declaration& decl, const Declaration& scope, bool isReader);
        void WriteFieldSetDecl(const Field& field, const Declaration& decl);
        void WriteFieldSetImpl(const Field& field, const Declaration& decl, const Declaration& scope);

    // Source Generation
    private:
        void GenSource();

    // Utilities
    private:
        bool FlushToFile(const char* suffix);

        void WriteDeclInfo(const Declaration& decl);
        void WriteName(const Declaration& decl, const Declaration& scope, const Brand& brand, const char* pointerSuffix);
        void WriteTemplate(const Declaration& decl);
        void WriteType(const Type& type, const Declaration& scope, const char* pointerSuffix);
        void WriteValue(const Type& type, const Declaration& scope, const Value& value);
        void WriteWithReplace(StringView input, char what, StringView with);

    private:
        const CodeGenRequest& m_request;
        CodeWriter m_writer;
        String m_namespaceName;
    };
}
