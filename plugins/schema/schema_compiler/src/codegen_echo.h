// Copyright Chad Engler

#pragma once

#include "he/core/string_builder.h"
#include "he/core/types.h"
#include "he/schema/codegen.h"
#include "he/schema/schema.h"

namespace he::schema
{
    class CodeGenEcho : private SchemaVisitor
    {
    public:
        CodeGenEcho(const CodeGenRequest& request);

        bool Generate();

    private:
        bool VisitFile(Declaration::Reader decl) override;
        bool VisitAttribute(Declaration::Reader decl, Declaration::Reader scope) override;
        bool VisitConstant(Declaration::Reader decl, Declaration::Reader scope) override;

        bool VisitEnum(Declaration::Reader decl, Declaration::Reader scope) override;
        bool VisitEnumerator(Enumerator::Reader enumerator, Declaration::Reader scope) override;

        bool VisitInterface(Declaration::Reader decl, Declaration::Reader scope) override;
        bool VisitMethod(Method::Reader method, Declaration::Reader scope) override;

        bool VisitStruct(Declaration::Reader decl, Declaration::Reader scope) override;
        bool VisitNormalField(Field::Reader field, Declaration::Reader scope) override;
        bool VisitGroupField(Field::Reader field, Declaration::Reader scope) override;
        bool VisitUnionField(Field::Reader field, Declaration::Reader scope) override;

    private:
        void WriteAttribute(Attribute::Reader attribute, Declaration::Reader scope);
        void WriteAttributes(List<Attribute>::Reader attributes, Declaration::Reader scope);
        void WriteField(Field::Reader field, Declaration::Reader scope);
        bool WriteGroupOrUnionField(Field::Reader field, Declaration::Reader scope);
        void WriteName(Declaration::Reader decl, Declaration::Reader scope, Brand::Reader brand);
        void WriteTuple(Declaration::Reader decl);
        void WriteTypeParams(List<String>::Reader typeParams);
        void WriteType(Type::Reader type, Declaration::Reader scope);
        void WriteValue(Type::Reader type, Declaration::Reader scope, Value::Reader value);

        void WriteEnumValue(Type::Data::Enum::Reader enumType, uint16_t value, Declaration::Reader scope);

        template <typename F>
        void WriteListValue(uint32_t size, F&& iterator);
        void WriteListElementValue(uint32_t index, Type::Reader elementType, Declaration::Reader scope, ListReader list);

        void WriteStructValue(Type::Data::Struct::Reader structType, Declaration::Reader scope, StructReader structValue);
        void WriteStructFieldValue(uint16_t index, uint32_t dataOffset, Type::Reader type, Declaration::Reader scope, StructReader structValue);
        bool TryWriteStructFieldValue(Field::Reader field, Declaration::Reader scope, StructReader structValue);

    private:
        const CodeGenRequest& m_request;
        StringBuilder m_writer;
    };
}
