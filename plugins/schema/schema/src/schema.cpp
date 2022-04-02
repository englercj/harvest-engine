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
        switch (decl.Data().Tag())
        {
            case Declaration::Data::Tag::File:
            {
                HE_ASSERT(!scope.IsValid());
                return VisitFile(decl);
            }
            case Declaration::Data::Tag::Attribute:
                return VisitAttribute(decl, scope);
            case Declaration::Data::Tag::Constant:
                return VisitConstant(decl, scope);
            case Declaration::Data::Tag::Enum:
                return VisitEnum(decl, scope);
            case Declaration::Data::Tag::Interface:
                return VisitInterface(decl, scope);
            case Declaration::Data::Tag::Struct:
                return VisitStruct(decl, scope);
        }

        return true;
    }

    bool SchemaVisitor::VisitFile(Declaration::Reader decl)
    {
        for (Declaration::Reader child : decl.Children())
        {
            if (!VisitDecl(child, decl))
                return false;
        }

        return true;
    }

    bool SchemaVisitor::VisitEnum(Declaration::Reader decl, Declaration::Reader scope)
    {
        HE_UNUSED(scope);
        Declaration::Data::Enum::Reader enumDecl = decl.Data().Enum();

        for (Enumerator::Reader enumerator : enumDecl.Enumerators())
        {
            if (!VisitEnumerator(enumerator, decl))
                return false;
        }

        return true;
    }

    bool SchemaVisitor::VisitInterface(Declaration::Reader decl, Declaration::Reader scope)
    {
        HE_UNUSED(scope);
        Declaration::Data::Interface::Reader interfaceDecl = decl.Data().Interface();

        for (Declaration::Reader child : decl.Children())
        {
            Declaration::Data::Reader childData = child.Data();
            Declaration::Data::Struct::Reader childSt = childData.IsStruct() ? childData.Struct() : Declaration::Data::Struct::Reader{};
            if (!childData.IsStruct() || (!childSt.IsMethodParams() && !childSt.IsMethodResults()))
            {
                if (!VisitDecl(child, decl))
                    return false;
            }
        }

        for (Method::Reader method : interfaceDecl.Methods())
        {
            if (!VisitMethod(method, decl))
                return false;
        }

        return true;
    }

    bool SchemaVisitor::VisitStruct(Declaration::Reader decl, Declaration::Reader scope)
    {
        HE_UNUSED(scope);
        Declaration::Data::Struct::Reader structDecl = decl.Data().Struct();

        for (Declaration::Reader child : decl.Children())
        {
            Declaration::Data::Reader childData = child.Data();
            Declaration::Data::Struct::Reader childSt = childData.IsStruct() ? childData.Struct() : Declaration::Data::Struct::Reader{};
            if (!childData.IsStruct() || (!childSt.IsMethodParams() && !childSt.IsMethodResults() && !childSt.IsGroup() && !childSt.IsUnion()))
            {
                if (!VisitDecl(child, decl))
                    return false;
            }
        }

        for (Field::Reader field : structDecl.Fields())
        {
            if (!VisitField(field, decl))
                return false;
        }

        return true;
    }

    bool SchemaVisitor::VisitField(Field::Reader field, Declaration::Reader scope)
    {
        switch (field.Meta().Tag())
        {
            case Field::Meta::Tag::Normal:
                if (!VisitNormalField(field, scope))
                    return false;
                break;
            case Field::Meta::Tag::Group:
                if (!VisitGroupField(field, scope))
                    return false;
                break;
            case Field::Meta::Tag::Union:
                if (!VisitUnionField(field, scope))
                    return false;
                break;
        }

        return true;
    }
}
