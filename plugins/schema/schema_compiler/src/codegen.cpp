// Copyright Chad Engler

#include "he/schema/codegen.h"

namespace he::schema
{
    static void CacheDeclIds(Declaration::Reader decl, DeclIdMap& map);

    static void CacheDeclIds(SchemaFile::Reader schema, DeclIdMap& map)
    {
        CacheDeclIds(schema.Root(), map);

        for (Import::Reader im : schema.Imports())
        {
            CacheDeclIds(im.Schema(), map);
        }
    }

    static void CacheDeclIds(Declaration::Reader decl, DeclIdMap& map)
    {
        HE_ASSERT(decl.Id() != 0);
        const auto result = map.emplace(decl.Id(), decl);
        HE_UNUSED(result);
        HE_ASSERT(result.second);

        for (Declaration::Reader child : decl.Children())
        {
            CacheDeclIds(child, map);
        }
    }

    CodeGenRequest::CodeGenRequest(SchemaFile::Reader schema)
        : schema(schema)
    {
        CacheDeclIds(schema, const_cast<DeclIdMap&>(declsById));
    }

    Declaration::Reader CodeGenRequest::GetDecl(uint64_t id) const
    {
        return declsById.at(id);
    }
}
