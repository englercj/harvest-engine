// Copyright Chad Engler

#pragma once

#include "he/core/string_view.h"
#include "he/core/types.h"
#include "he/schema/codegen.h"
#include "he/schema/code_writer.h"
#include "he/schema/schema.h"

#include <concepts>

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

    protected:
        void GenHeader();
        void GenSource();

        void GenHdr_Includes();

        void GenHdr_Alias(const AliasDef& def);
        void GenHdr_Attribute(const AttributeDef& def);
        void GenHdr_Const(const ConstDef& def);
        void GenHdr_Enum(const EnumDef& def);
        void GenHdr_Interface(const InterfaceDef& def);
        void GenHdr_Struct(const StructDef& def);
        void GenHdr_Union(const UnionDef& def);

        void GenSrc_Enums();
        void GenSrc_Structs();

        void GenSrc_StructJson(const StructDef& def);

    protected:
        void WriteFieldToJson(const FieldDef& field);
        void WriteFieldFromJson(const FieldDef& field);

    private:
        void WriteBufferType(const Type& t);
        void WriteBufferWrappedType(const Type& t);

        void WriteBufferParamType(const Type& t);

        void WriteBufferStructBuildPrototype(const StructDef& def, bool prefixed);
        void WriteBufferStructBuildImpl(const StructDef& def);

        void WriteBufferUnionBuildPrototype(const UnionDef& def, const FieldDef& field, bool prefixed);
        void WriteBufferUnionBuildImpl(const UnionDef& def, const FieldDef& field);

    protected:
        bool FlushToFile(const char* suffix);

        void WriteArraySize(const Type& t);
        void WriteGenericDecl(const Vector<String>& typeParams);
        void WriteNamespacePrefixed(StringView name);
        void WriteWithReplace(StringView input, char what, StringView with);
        void WriteEscaped(StringView input);
        void WriteNativeType(const Type& t);
        void WriteNativeValue(BaseType t, const Value& v);
        void WriteParamType(const Type& t);
        void WriteScalarType(BaseType t);
        void WriteUnsignedAsHex(BaseType t, const Value& v);

        template <std::integral T>
        void WriteAsHex(T value)
        {
            m_writer.Write("{:#0{}x}", value, 2 + (sizeof(T) * 2));
        }

    protected:
        const SchemaDef& m_schema;
        const CodeGenOptions& m_options;

        CodeWriter m_writer;
    };
}
