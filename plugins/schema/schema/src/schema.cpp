// Copyright Chad Engler

#include "he/schema/schema.h"

#include "he/core/assert.h"
#include "he/core/uuid.h"

#include <algorithm>

namespace he::schema
{
    static bool DeclInfoComp(const DeclInfo* info, TypeId id)
    {
        return info->id < id;
    }

    void FillUuidV4(Uuid::Builder builder)
    {
        const he::Uuid uuid = he::Uuid::CreateV4();
        Span<uint8_t> dst = builder.GetValue();

        HE_ASSERT(dst.Size() == sizeof(uuid.m_bytes));
        MemCopy(dst.Data(), uuid.m_bytes, sizeof(uuid.m_bytes));
    }

    const DeclInfo* FindDependency(const DeclInfo& info, TypeId id)
    {
        const DeclInfo* const* begin = info.dependencies;
        const DeclInfo* const* end = info.dependencies + info.dependencyCount;
        const DeclInfo* const* lower = std::lower_bound(begin, end, id, DeclInfoComp);
        return lower && (*lower)->id == id ? *lower : nullptr;
    }

    bool SchemaVisitor::VisitDecl(Declaration::Reader decl, Declaration::Reader scope)
    {
        switch (decl.GetData().GetUnionTag())
        {
            case Declaration::Data::UnionTag::File:
            {
                HE_ASSERT(!scope.IsValid());
                return VisitFile(decl);
            }
            case Declaration::Data::UnionTag::Attribute:
                return VisitAttribute(decl, scope);
            case Declaration::Data::UnionTag::Constant:
                return VisitConstant(decl, scope);
            case Declaration::Data::UnionTag::Enum:
                return VisitEnum(decl, scope);
            case Declaration::Data::UnionTag::Interface:
                return VisitInterface(decl, scope);
            case Declaration::Data::UnionTag::Struct:
                return VisitStruct(decl, scope);
        }

        return true;
    }

    bool SchemaVisitor::VisitFile(Declaration::Reader decl)
    {
        for (const Declaration::Reader child : decl.GetChildren())
        {
            if (!VisitDecl(child, decl))
                return false;
        }

        return true;
    }

    bool SchemaVisitor::VisitEnum(Declaration::Reader decl, Declaration::Reader scope)
    {
        HE_UNUSED(scope);
        const Declaration::Data::Enum::Reader enumDecl = decl.GetData().GetEnum();

        for (const Enumerator::Reader enumerator : enumDecl.GetEnumerators())
        {
            if (!VisitEnumerator(enumerator, decl))
                return false;
        }

        return true;
    }

    bool SchemaVisitor::VisitInterface(Declaration::Reader decl, Declaration::Reader scope)
    {
        HE_UNUSED(scope);
        const Declaration::Data::Interface::Reader interfaceDecl = decl.GetData().GetInterface();

        for (const Declaration::Reader child : decl.GetChildren())
        {
            const Declaration::Data::Reader childData = child.GetData();
            const Declaration::Data::Struct::Reader childSt = childData.IsStruct() ? childData.GetStruct() : Declaration::Data::Struct::Reader{};
            if (!childData.IsStruct() || (!childSt.GetIsMethodParams() && !childSt.GetIsMethodResults()))
            {
                if (!VisitDecl(child, decl))
                    return false;
            }
        }

        for (const Method::Reader method : interfaceDecl.GetMethods())
        {
            if (!VisitMethod(method, decl))
                return false;
        }

        return true;
    }

    bool SchemaVisitor::VisitStruct(Declaration::Reader decl, Declaration::Reader scope)
    {
        HE_UNUSED(scope);
        const Declaration::Data::Struct::Reader structDecl = decl.GetData().GetStruct();

        for (Declaration::Reader child : decl.GetChildren())
        {
            const Declaration::Data::Reader childData = child.GetData();
            const Declaration::Data::Struct::Reader childSt = childData.IsStruct() ? childData.GetStruct() : Declaration::Data::Struct::Reader{};
            if (!childData.IsStruct() || (!childSt.GetIsMethodParams() && !childSt.GetIsMethodResults() && !childSt.GetIsGroup() && !childSt.GetIsUnion()))
            {
                if (!VisitDecl(child, decl))
                    return false;
            }
        }

        for (const Field::Reader field : structDecl.GetFields())
        {
            if (!VisitField(field, decl))
                return false;
        }

        return true;
    }

    bool SchemaVisitor::VisitField(Field::Reader field, Declaration::Reader scope)
    {
        switch (field.GetMeta().GetUnionTag())
        {
            case Field::Meta::UnionTag::Normal:
                if (!VisitNormalField(field, scope))
                    return false;
                break;
            case Field::Meta::UnionTag::Group:
                if (!VisitGroupField(field, scope))
                    return false;
                break;
            case Field::Meta::UnionTag::Union:
                if (!VisitUnionField(field, scope))
                    return false;
                break;
        }

        return true;
    }

    void StructVisitor::VisitStruct(StructReader data, const DeclInfo& info)
    {
        const Declaration::Reader decl = GetSchema(info);
        const Declaration::Data::Struct::Reader structDecl = decl.GetData().GetStruct();

        // Visit normal fields, that are not structures, first
        for (Field::Reader field : structDecl.GetFields())
        {
            const Field::Meta::UnionTag tag = field.GetMeta().GetUnionTag();

            if (tag == Field::Meta::UnionTag::Normal)
            {
                const Field::Meta::Normal::Reader norm = field.GetMeta().GetNormal();
                if (!norm.GetType().GetData().IsStruct())
                {
                    VisitField(data, field, info);
                }
            }
        }

        // Visit structural fields second
        for (Field::Reader field : structDecl.GetFields())
        {
            const Field::Meta::UnionTag tag = field.GetMeta().GetUnionTag();

            if (tag == Field::Meta::UnionTag::Normal)
            {
                const Field::Meta::Normal::Reader norm = field.GetMeta().GetNormal();
                if (norm.GetType().GetData().IsStruct())
                {
                    VisitField(data, field, info);
                }
            }
            else
            {
                VisitField(data, field, info);
            }
        }
    }

    void StructVisitor::VisitField(StructReader data, Field::Reader field, const DeclInfo& scope)
    {
        switch (field.GetMeta().GetUnionTag())
        {
            case Field::Meta::UnionTag::Normal:
                if (ShouldVisitNormalField(data, field, scope))
                    VisitNormalField(data, field, scope);
                break;
            case Field::Meta::UnionTag::Group:
                if (ShouldVisitGroupField(data, field, scope))
                    VisitGroupField(data, field, scope);
                break;
            case Field::Meta::UnionTag::Union:
                if (ShouldVisitUnionField(data, field, scope))
                    VisitUnionField(data, field, scope);
                break;
        }
    }

    void StructVisitor::VisitNormalField(StructReader data, Field::Reader field, const DeclInfo& scope)
    {
        const Field::Meta::Normal::Reader norm = field.GetMeta().GetNormal();
        const Type::Reader fieldType = norm.GetType();
        const uint16_t index = norm.GetIndex();
        const uint32_t dataOffset = norm.GetDataOffset();

        VisitValue(data, fieldType, index, dataOffset, scope);
    }

    void StructVisitor::VisitGroupField(StructReader data, Field::Reader field, const DeclInfo& scope)
    {
        const DeclInfo* info = FindGroupOrUnionInfo(field, scope);
        if (!info)
            return;

        VisitStruct(data, *info);
    }

    void StructVisitor::VisitUnionField(StructReader data, Field::Reader field, const DeclInfo& scope)
    {
        const DeclInfo* info = FindGroupOrUnionInfo(field, scope);
        if (!info)
            return;

        const Declaration::Reader decl = GetSchema(*info);
        const Declaration::Data::Struct::Reader structDecl = decl.GetData().GetStruct();
        const List<Field>::Reader fields = structDecl.GetFields();

        const uint16_t activeFieldTag = data.GetDataField<uint16_t>(structDecl.GetUnionTagOffset());

        for (const Field::Reader unionField : fields)
        {
            if (unionField.GetUnionTag() == activeFieldTag)
            {
                VisitField(data, unionField, scope);
                break;
            }
        }
    }

    bool StructVisitor::ShouldVisitNormalField(StructReader data, Field::Reader field, const DeclInfo& scope)
    {
        HE_UNUSED(scope);

        const Field::Meta::Normal::Reader norm = field.GetMeta().GetNormal();
        const Type::Reader fieldType = norm.GetType();

        if (fieldType.GetData().IsVoid())
            return false;

        if (IsPointer(fieldType))
        {
            return data.HasPointerField(norm.GetIndex());
        }

        return data.HasDataField(norm.GetIndex());
    }

    bool StructVisitor::ShouldVisitGroupField(StructReader data, Field::Reader field, const DeclInfo& scope)
    {
        return AnyGroupFieldSet(data, field, scope);
    }

    bool StructVisitor::ShouldVisitUnionField(StructReader data, Field::Reader field, const DeclInfo& scope)
    {
        return IsUnionFieldSet(data, field, scope);
    }

    void StructVisitor::VisitValue(Value::Reader value, Type::Reader type, const DeclInfo& scope)
    {
        const Value::Data::Reader valueData = value.GetData();

        switch (valueData.GetUnionTag())
        {
            case Value::Data::UnionTag::Void: break;
            case Value::Data::UnionTag::Bool: VisitValue(valueData.GetBool(), type, scope); break;
            case Value::Data::UnionTag::Int8: VisitValue(valueData.GetInt8(), type, scope); break;
            case Value::Data::UnionTag::Int16: VisitValue(valueData.GetInt16(), type, scope); break;
            case Value::Data::UnionTag::Int32: VisitValue(valueData.GetInt32(), type, scope); break;
            case Value::Data::UnionTag::Int64: VisitValue(valueData.GetInt64(), type, scope); break;
            case Value::Data::UnionTag::Uint8: VisitValue(valueData.GetUint8(), type, scope); break;
            case Value::Data::UnionTag::Uint16: VisitValue(valueData.GetUint16(), type, scope); break;
            case Value::Data::UnionTag::Uint32: VisitValue(valueData.GetUint32(), type, scope); break;
            case Value::Data::UnionTag::Uint64: VisitValue(valueData.GetUint64(), type, scope); break;
            case Value::Data::UnionTag::Float32: VisitValue(valueData.GetFloat32(), type, scope); break;
            case Value::Data::UnionTag::Float64: VisitValue(valueData.GetFloat64(), type, scope); break;
            case Value::Data::UnionTag::Blob: VisitValue(valueData.GetBlob(), type, scope); break;
            case Value::Data::UnionTag::String: VisitValue(valueData.GetString(), type, scope); break;
            case Value::Data::UnionTag::Enum: VisitValue(EnumValueTag{}, valueData.GetEnum(), type, scope); break;
            case Value::Data::UnionTag::List:
            {
                const Type::Reader elementType = type.GetData().GetList().GetElementType();
                const ElementSize elementSize = GetTypeElementSize(elementType);
                const ListReader listValues = valueData.GetList().TryGetList(elementSize);
                VisitListValue(listValues, type, scope);
                break;
            }
            case Value::Data::UnionTag::Struct:
            {
                const Type::Data::Struct::Reader structType = type.GetData().GetStruct();
                const StructReader structValue = valueData.GetStruct().TryGetStruct();
                const DeclInfo* info = FindDependency(scope, structType.GetId());
                VisitStruct(structValue, *info);
                break;
            }
        }
    }

    void StructVisitor::VisitValue(StructReader data, Type::Reader type, uint16_t index, uint32_t dataOffset, const DeclInfo& scope)
    {
        const Type::Data::Reader typeData = type.GetData();

        switch (typeData.GetUnionTag())
        {
            case Type::Data::UnionTag::Void: break;
            case Type::Data::UnionTag::Bool: VisitValue(data.GetDataField<bool>(dataOffset), type, scope); break;
            case Type::Data::UnionTag::Int8: VisitValue(data.GetDataField<int8_t>(dataOffset), type, scope); break;
            case Type::Data::UnionTag::Int16: VisitValue(data.GetDataField<int16_t>(dataOffset), type, scope); break;
            case Type::Data::UnionTag::Int32: VisitValue(data.GetDataField<int32_t>(dataOffset), type, scope); break;
            case Type::Data::UnionTag::Int64: VisitValue(data.GetDataField<int64_t>(dataOffset), type, scope); break;
            case Type::Data::UnionTag::Uint8: VisitValue(data.GetDataField<uint8_t>(dataOffset), type, scope); break;
            case Type::Data::UnionTag::Uint16: VisitValue(data.GetDataField<uint16_t>(dataOffset), type, scope); break;
            case Type::Data::UnionTag::Uint32: VisitValue(data.GetDataField<uint32_t>(dataOffset), type, scope); break;
            case Type::Data::UnionTag::Uint64: VisitValue(data.GetDataField<uint64_t>(dataOffset), type, scope); break;
            case Type::Data::UnionTag::Float32: VisitValue(data.GetDataField<float>(dataOffset), type, scope); break;
            case Type::Data::UnionTag::Float64: VisitValue(data.GetDataField<double>(dataOffset), type, scope); break;
            case Type::Data::UnionTag::Blob: VisitValue(data.GetPointerField(index).TryGetBlob(), type, scope); break;
            case Type::Data::UnionTag::String: VisitValue(data.GetPointerField(index).TryGetString(), type, scope); break;
            case Type::Data::UnionTag::Enum: VisitValue(EnumValueTag{}, data.GetDataField<uint16_t>(dataOffset), type, scope); break;
            case Type::Data::UnionTag::Array:
            {
                const Type::Data::Array::Reader arrayType = typeData.GetArray();
                const Type::Reader elementType = arrayType.GetElementType();

                if (!HE_VERIFY(!elementType.GetData().IsArray(),
                    HE_MSG("Invalid schema. Arrays of arrays are not supported."),
                    HE_KV(scope_id, scope.id),
                    HE_KV(scope_name, GetSchema(scope).GetName())))
                {
                    return;
                }

                const uint16_t size = arrayType.GetSize();
                VisitArrayValue(data, elementType, index, dataOffset, size, scope);
                break;
            }
            case Type::Data::UnionTag::List:
            {
                const Type::Data::List::Reader listType = typeData.GetList();
                const Type::Reader elementType = listType.GetElementType();
                const ElementSize elementSize = GetTypeElementSize(elementType);
                const ListReader list = data.GetPointerField(index).TryGetList(elementSize);
                VisitListValue(list, elementType, scope);
                break;
            }
            case Type::Data::UnionTag::Struct:
            {
                const Type::Data::Struct::Reader structType = typeData.GetStruct();
                const DeclInfo* info = FindDependency(scope, structType.GetId());

                if (!HE_VERIFY(info,
                    HE_MSG("Invalid schema. No dependency of the parent scope matches the struct's type id."),
                    HE_KV(scope_id, scope.id),
                    HE_KV(scope_name, GetSchema(scope).GetName())))
                {
                    return;
                }

                const StructReader st = data.GetPointerField(index).TryGetStruct();
                VisitStruct(st, *info);
                break;
            }
            case Type::Data::UnionTag::AnyPointer:
            {
                const PointerReader ptr = data.GetPointerField(index);
                VisitAnyPointer(ptr, type, scope);
                break;
            }
            case Type::Data::UnionTag::AnyStruct:
            {
                const PointerReader ptr = data.GetPointerField(index);
                VisitAnyStruct(ptr, type, scope);
                break;
            }
            case Type::Data::UnionTag::AnyList:
            {
                const PointerReader ptr = data.GetPointerField(index);
                VisitAnyList(ptr, type, scope);
                break;
            }
            case Type::Data::UnionTag::Interface:
            case Type::Data::UnionTag::Parameter:
                HE_VERIFY(false, HE_MSG("{} types cannot have values.", typeData.GetUnionTag()));
                break;
        }
    }

    void StructVisitor::VisitValue(ListReader data, Type::Reader elementType, uint32_t index, const DeclInfo& scope)
    {
        const Type::Data::Reader typeData = elementType.GetData();

        switch (typeData.GetUnionTag())
        {
            case Type::Data::UnionTag::Void: break;
            case Type::Data::UnionTag::Bool: VisitValue(data.GetDataElement<bool>(index), elementType, scope); break;
            case Type::Data::UnionTag::Int8: VisitValue(data.GetDataElement<int8_t>(index), elementType, scope); break;
            case Type::Data::UnionTag::Int16: VisitValue(data.GetDataElement<int16_t>(index), elementType, scope); break;
            case Type::Data::UnionTag::Int32: VisitValue(data.GetDataElement<int32_t>(index), elementType, scope); break;
            case Type::Data::UnionTag::Int64: VisitValue(data.GetDataElement<int64_t>(index), elementType, scope); break;
            case Type::Data::UnionTag::Uint8: VisitValue(data.GetDataElement<uint8_t>(index), elementType, scope); break;
            case Type::Data::UnionTag::Uint16: VisitValue(data.GetDataElement<uint16_t>(index), elementType, scope); break;
            case Type::Data::UnionTag::Uint32: VisitValue(data.GetDataElement<uint32_t>(index), elementType, scope); break;
            case Type::Data::UnionTag::Uint64: VisitValue(data.GetDataElement<uint64_t>(index), elementType, scope); break;
            case Type::Data::UnionTag::Float32: VisitValue(data.GetDataElement<float>(index), elementType, scope); break;
            case Type::Data::UnionTag::Float64: VisitValue(data.GetDataElement<double>(index), elementType, scope); break;
            case Type::Data::UnionTag::Blob: VisitValue(data.GetPointerElement(index).TryGetBlob(), elementType, scope); break;
            case Type::Data::UnionTag::String: VisitValue(data.GetPointerElement(index).TryGetString(), elementType, scope); break;
            case Type::Data::UnionTag::Enum: VisitValue(EnumValueTag{}, data.GetDataElement<uint16_t>(index), elementType, scope); break;
            case Type::Data::UnionTag::Array:
            {
                if (!HE_VERIFY(!typeData.IsArray(),
                    HE_MSG("Invalid schema. Lists of arrays are not supported."),
                    HE_KV(scope_id, scope.id),
                    HE_KV(scope_name, GetSchema(scope).GetName())))
                {
                    return;
                }
                break;
            }
            case Type::Data::UnionTag::List:
            {
                const Type::Data::List::Reader listType = typeData.GetList();
                const Type::Reader subElementType = listType.GetElementType();
                const ElementSize subElementSize = GetTypeElementSize(subElementType);
                const ListReader list = data.GetPointerElement(index).TryGetList(subElementSize);
                VisitListValue(list, subElementType, scope);
                break;
            }
            case Type::Data::UnionTag::Struct:
            {
                const Type::Data::Struct::Reader structType = typeData.GetStruct();
                const DeclInfo* info = FindDependency(scope, structType.GetId());

                if (!HE_VERIFY(info,
                    HE_MSG("Invalid schema. No dependency of the parent scope matches the struct's type id."),
                    HE_KV(scope_id, scope.id),
                    HE_KV(scope_name, GetSchema(scope).GetName())))
                {
                    return;
                }

                const StructReader st = data.GetCompositeElement(index);
                VisitStruct(st, *info);
                break;
            }
            case Type::Data::UnionTag::AnyPointer:
            {
                const PointerReader ptr = data.GetPointerElement(index);
                VisitAnyPointer(ptr, elementType, scope);
                break;
            }
            case Type::Data::UnionTag::AnyStruct:
            {
                const PointerReader ptr = data.GetPointerElement(index);
                VisitAnyStruct(ptr, elementType, scope);
                break;
            }
            case Type::Data::UnionTag::AnyList:
            {
                const PointerReader ptr = data.GetPointerElement(index);
                VisitAnyList(ptr, elementType, scope);
                break;
            }
            case Type::Data::UnionTag::Interface:
            case Type::Data::UnionTag::Parameter:
                HE_VERIFY(false, HE_MSG("{} types cannot have values.", typeData.GetUnionTag()));
                break;
        }
    }

    void StructVisitor::VisitArrayValue(StructReader data, Type::Reader elementType, uint16_t index, uint32_t dataOffset, uint16_t size, const DeclInfo& scope)
    {
        const bool isPointer = IsPointer(elementType);

        for (uint16_t i = 0; i < size; ++i)
        {
            if (isPointer)
            {
                VisitValue(data, elementType, index + i, dataOffset, scope);
            }
            else
            {
                VisitValue(data, elementType, index, dataOffset + i, scope);
            }
        }
    }

    void StructVisitor::VisitListValue(ListReader data, Type::Reader elementType, const DeclInfo& scope)
    {
        const uint32_t size = data.Size();

        for (uint32_t i = 0; i < size; ++i)
        {
            VisitValue(data, elementType, i, scope);
        }
    }

    bool StructVisitor::AnyGroupFieldSet(StructReader data, Field::Reader field, const DeclInfo& scope)
    {
        const DeclInfo* info = FindGroupOrUnionInfo(field, scope);
        if (!info)
            return false;

        const Declaration::Reader decl = GetSchema(*info);
        const Declaration::Data::Struct::Reader structDecl = decl.GetData().GetStruct();
        const List<Field>::Reader fields = structDecl.GetFields();

        for (const Field::Reader f : fields)
        {
            if (f.GetMeta().IsGroup())
            {
                if (AnyGroupFieldSet(data, f, scope))
                    return true;
            }
            else if (f.GetMeta().IsUnion())
            {
                if (IsUnionFieldSet(data, f, scope))
                    return true;
            }
            else
            {
                if (IsNormalFieldSet(data, f, scope))
                    return true;
            }
        }

        return false;
    }

    bool StructVisitor::IsUnionFieldSet(StructReader data, Field::Reader field, const DeclInfo& scope)
    {
        // TODO: This doesn't actually work. We need to allocate a field index for the union tag to make this work.
        //const Field::Meta::Group::Reader group = field.GetMeta().GetGroup();
        //const Declaration::Reader decl = m_request.GetDecl(group.GetTypeId());
        //const Declaration::Data::Struct::Reader structDecl = decl.GetData().GetStruct();

        //return data.HasDataField(structDecl.GetUnionTagIndex());

        HE_UNUSED(data, field, scope);
        return true;
    }

    bool StructVisitor::IsNormalFieldSet(StructReader data, Field::Reader field, const DeclInfo& scope)
    {
        HE_UNUSED(scope);

        const Field::Meta::Normal::Reader norm = field.GetMeta().GetNormal();
        const Type::Reader fieldType = norm.GetType();
        const Type::Data::Reader fieldTypeData = fieldType.GetData();

        if (fieldTypeData.IsVoid())
            return false;

        if (IsPointer(fieldType))
            return data.HasPointerField(norm.GetIndex());

        return data.HasDataField(norm.GetIndex());
    }

    const DeclInfo* StructVisitor::FindGroupOrUnionInfo(Field::Reader field, const DeclInfo& scope)
    {
        HE_ASSERT(field.GetMeta().IsGroup() || field.GetMeta().IsUnion());

        const Declaration::Reader decl = GetSchema(scope);

        const Field::Meta::Reader meta = field.GetMeta();
        const TypeId id = meta.IsGroup() ? meta.GetGroup().GetTypeId() : meta.GetUnion().GetTypeId();
        const char* kindName = meta.IsGroup() ? "group" : "union";
        HE_UNUSED(kindName);

        Declaration::Reader groupChild;
        for (const Declaration::Reader child : decl.GetChildren())
        {
            if (child.GetId() == id)
            {
                if (!HE_VERIFY(child.GetData().IsStruct(),
                    HE_MSG("Invalid schema. Child declaration that matches {} type id is not a struct.", kindName),
                    HE_KV(scope_type_id, decl.GetId()),
                    HE_KV(scope_name, decl.GetName()),
                    HE_KV(child_type_id, id),
                    HE_KV(child_name, field.GetName())))
                {
                    return nullptr;
                }

                const Declaration::Data::Struct::Reader childStruct = child.GetData().GetStruct();

                if (!HE_VERIFY(childStruct.GetIsGroup() == meta.IsGroup() && childStruct.GetIsUnion() == meta.IsUnion(),
                    HE_MSG("Invalid schema. Child declaration that matches {0} type id is not marked as a {0}.", kindName),
                    HE_KV(scope_type_id, decl.GetId()),
                    HE_KV(scope_name, decl.GetName()),
                    HE_KV(child_type_id, id),
                    HE_KV(child_name, field.GetName())))
                {
                    return nullptr;
                }

                groupChild = child;
                break;
            }
        }

        if (!HE_VERIFY(groupChild.IsValid(),
            HE_MSG("Invalid schema. No child declaration matches the {}'s type id.", kindName),
            HE_KV(scope_type_id, decl.GetId()),
            HE_KV(scope_name, decl.GetName()),
            HE_KV(child_type_id, id),
            HE_KV(child_name, field.GetName())))
        {
            return nullptr;
        }

        const DeclInfo* info = FindDependency(scope, groupChild.GetId());
        if (!HE_VERIFY(info,
            HE_MSG("Invalid schema. No dependency of the parent scope matches the {}'s type id.", kindName),
            HE_KV(scope_type_id, decl.GetId()),
            HE_KV(scope_name, decl.GetName()),
            HE_KV(child_type_id, id),
            HE_KV(child_name, field.GetName())))
        {
            return nullptr;
        }

        return info;
    }
}
