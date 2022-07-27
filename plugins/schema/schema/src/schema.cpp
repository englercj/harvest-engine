// Copyright Chad Engler

#include "he/schema/schema.h"

#include "he/core/assert.h"

#include <algorithm>

namespace he::schema
{
    static bool DeclInfoComp(const DeclInfo* info, TypeId id)
    {
        return info->id < id;
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
        for (Declaration::Reader child : decl.GetChildren())
        {
            if (!VisitDecl(child, decl))
                return false;
        }

        return true;
    }

    bool SchemaVisitor::VisitEnum(Declaration::Reader decl, Declaration::Reader scope)
    {
        HE_UNUSED(scope);
        Declaration::Data::Enum::Reader enumDecl = decl.GetData().GetEnum();

        for (Enumerator::Reader enumerator : enumDecl.GetEnumerators())
        {
            if (!VisitEnumerator(enumerator, decl))
                return false;
        }

        return true;
    }

    bool SchemaVisitor::VisitInterface(Declaration::Reader decl, Declaration::Reader scope)
    {
        HE_UNUSED(scope);
        Declaration::Data::Interface::Reader interfaceDecl = decl.GetData().GetInterface();

        for (Declaration::Reader child : decl.GetChildren())
        {
            Declaration::Data::Reader childData = child.GetData();
            Declaration::Data::Struct::Reader childSt = childData.IsStruct() ? childData.GetStruct() : Declaration::Data::Struct::Reader{};
            if (!childData.IsStruct() || (!childSt.GetIsMethodParams() && !childSt.GetIsMethodResults()))
            {
                if (!VisitDecl(child, decl))
                    return false;
            }
        }

        for (Method::Reader method : interfaceDecl.GetMethods())
        {
            if (!VisitMethod(method, decl))
                return false;
        }

        return true;
    }

    bool SchemaVisitor::VisitStruct(Declaration::Reader decl, Declaration::Reader scope)
    {
        HE_UNUSED(scope);
        Declaration::Data::Struct::Reader structDecl = decl.GetData().GetStruct();

        for (Declaration::Reader child : decl.GetChildren())
        {
            Declaration::Data::Reader childData = child.GetData();
            Declaration::Data::Struct::Reader childSt = childData.IsStruct() ? childData.GetStruct() : Declaration::Data::Struct::Reader{};
            if (!childData.IsStruct() || (!childSt.GetIsMethodParams() && !childSt.GetIsMethodResults() && !childSt.GetIsGroup() && !childSt.GetIsUnion()))
            {
                if (!VisitDecl(child, decl))
                    return false;
            }
        }

        for (Field::Reader field : structDecl.GetFields())
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
}
