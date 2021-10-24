// Copyright Chad Engler

#pragma once

#include "he/core/string.h"
#include "he/core/string_view.h"
#include "he/core/types.h"
#include "he/schema/codegen.h"
#include "he/schema/code_writer.h"
#include "he/schema/schema.h"

namespace he::schema
{
    class CodeGenCpp
    {
    public:
        CodeGenCpp(const SchemaDef& schema, const CodeGenOptions& options)
            : m_schema(schema)
            , m_options(options)
            , m_writer(schema.namespaceName.GetAllocator())
        {}

        bool Generate();

    private:
        void GenHeader();

        void GenHdr_Includes();

        void GenHdr_Alias(const AliasDef& def);
        void GenHdr_Attribute(const AttributeDef& def);
        void GenHdr_Const(const ConstDef& def);
        void GenHdr_Enum(const EnumDef& def);
        void GenHdr_Interface(const InterfaceDef& def);
        void GenHdr_Struct(const StructDef& def);
        void GenHdr_Union(const UnionDef& def);

    private:
        void GenSource();

        void GenSrc_BufferImpl();
        void GenSrc_BufferImplRecursive(
            StringView prefix,
            Span<const ObjectDef> objects,
            Span<const EnumDef> enums,
            Span<const InterfaceDef> interfaces,
            Span<const StructDef> structs,
            Span<const UnionDef> unions);
        void GenSrc_StructBuffer(StringView prefix, const StructDef& def);
        void GenSrc_UnionBuffer(StringView prefix, const UnionDef& def);

        void GenSrc_JsonImpl();
        void GenSrc_JsonImplRecursive(
            StringView prefix,
            Span<const ObjectDef> objects,
            Span<const EnumDef> enums,
            Span<const InterfaceDef> interfaces,
            Span<const StructDef> structs,
            Span<const UnionDef> unions);
        void GenSrc_EnumJson(StringView prefix, const EnumDef& def);
        void GenSrc_StructJson(StringView prefix, const StructDef& def);
        void GenSrc_UnionJson(StringView prefix, const UnionDef& def);

        void GenSrc_ReflectionImpl();
        void GenSrc_ReflectionImplRecursive(
            StringView prefix,
            Span<const ObjectDef> objects,
            Span<const EnumDef> enums,
            Span<const InterfaceDef> interfaces,
            Span<const StructDef> structs,
            Span<const UnionDef> unions);
        void GenSrc_StructReflection(StringView prefix, const StructDef& def);
        void GenSrc_UnionReflection(StringView prefix, const UnionDef& def);

        void GenSrc_StringImpl();
        void GenSrc_StringImplRecursive(
            StringView prefix,
            Span<const ObjectDef> objects,
            Span<const EnumDef> enums,
            Span<const InterfaceDef> interfaces,
            Span<const StructDef> structs,
            Span<const UnionDef> unions);
        void GenSrc_EnumAsString(StringView prefix, const EnumDef& def);

    private:
        bool FlushToFile(const char* suffix);

        void WriteArraySize(const Type& t);
        void WriteBufferType(const Type& t);
        void WriteBufferWrappedType(const Type& t);
        void WriteEscaped(StringView input);
        void WriteGenericDecl(const Vector<String>& typeParams);
        void WriteNativeType(const Type& t);
        void WriteNativeValue(BaseType t, const Value& v);
        void WriteParamType(const Type& t);
        void WriteUnsignedAsHex(BaseType t, const Value& v);
        void WriteWithReplace(StringView input, char what, StringView with);

        const char* GetScalarType(BaseType t);

    protected:
        const SchemaDef& m_schema;
        const CodeGenOptions& m_options;

        CodeWriter m_writer;
        String m_namespacePrefix;
    };
}
