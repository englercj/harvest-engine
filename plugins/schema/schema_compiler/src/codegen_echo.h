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
        explicit CodeGenEcho(const CodeGenRequest& request) noexcept;

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
        void WriteName(Declaration::Reader decl, Brand::Reader brand, Declaration::Reader scope);
        void WriteTuple(Declaration::Reader decl);
        void WriteTypeParams(List<String>::Reader typeParams);
        void WriteType(Type::Reader type, Declaration::Reader scope);
        void WriteValue(Type::Reader type, Value::Reader value, Declaration::Reader scope);
        void WriteValue(Type::Reader fieldType, StructReader value, uint16_t index, uint32_t dataOffset, Declaration::Reader scope);
        void WriteValue(Type::Reader elementType, ListReader value, uint32_t index, Declaration::Reader scope);

        void WriteArrayValue(Type::Reader elementType, StructReader value, uint16_t index, uint32_t dataOffset, uint16_t size, Declaration::Reader scope);
        void WriteListValue(Type::Reader elementType, ListReader list, Declaration::Reader scope);
        void WriteStructValue(TypeId typeId, StructReader value, Declaration::Reader scope);
        void WriteUnionValue(TypeId typeId, StructReader value, Declaration::Reader scope);
        bool TryWriteFieldValue(Field::Reader field, StructReader value, Declaration::Reader scope);

        bool AnyGroupFieldSet(Field::Reader field, StructReader value);
        bool IsUnionFieldSet(Field::Reader field, StructReader value);
        bool IsNormalFieldSet(Field::Reader field, StructReader value);

    private:
        const CodeGenRequest& m_request;
        StringBuilder m_writer;
    };
}
