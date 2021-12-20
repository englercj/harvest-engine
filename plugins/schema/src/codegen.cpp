// Copyright Chad Engler

#include "he/schema/codegen.h"

namespace he::schema
{
    static void CacheDeclIds(const Declaration& decl, DeclIdMap& map);

    static void CacheDeclIds(const SchemaFile& schema, DeclIdMap& map)
    {
        CacheDeclIds(schema.root, map);

        for (const Import& im : schema.imports)
        {
            CacheDeclIds(*im.schema, map);
        }
    }

    static void CacheDeclIds(const Declaration& decl, DeclIdMap& map)
    {
        const auto result = map.try_emplace(decl.id, &decl);
        HE_UNUSED(result);
        HE_ASSERT(result.second);

        for (const Declaration& child : decl.children)
        {
            CacheDeclIds(child, map);
        }
    }

    CodeGenRequest::CodeGenRequest(const SchemaFile& schema)
        : schema(schema)
    {
        CacheDeclIds(schema, const_cast<DeclIdMap&>(declsById));
    }

    const Declaration& CodeGenRequest::GetDecl(uint64_t id) const
    {
        return *declsById.at(id);
    }
}
