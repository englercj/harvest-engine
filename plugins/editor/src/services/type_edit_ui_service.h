// Copyright Chad Engler

#pragma once

#include "asset_edit_service.h"

#include "he/core/delegate.h"
#include "he/core/span.h"
#include "he/core/type_info.h"
#include "he/core/types.h"
#include "he/schema/schema.h"
#include "he/schema/layout.h"

#include <unordered_map>

namespace he::editor
{
    class TypeEditUIService
    {
    public:
        template <typename T>
        using Pfn_Editor = void(*)(T, schema::Field::Reader, AssetEditService::Context*);

    public:
        template <typename T>
        void RegisterTypeEditor(Pfn_Editor<T> editor);

        void ShowTypeEditor(const TypeInfo& info, const void* value, schema::Field::Reader path);

        template <typename T>
        void ShowTypeEditor(const void* value, schema::Field::Reader path)
        {
            ShowTypeEditor(TypeInfo::Get<T>(), value, data, path);
        }

    private:
        using EditorDelegate = Delegate<bool(const void* value, schema::StructBuilder data, Span<const schema::Field::Reader> path)>;

        void RegisterTypeEditor(const TypeInfo& info, EditorDelegate func);
        void RegisterFieldEditor(const TypeInfo& info, const char* fieldName, EditorDelegate func);

    private:
        struct TypeEditorKey
        {
            TypeInfo type;
            String fieldName;
        };

    private:
        std::unordered_map<TypeEditorKey, EditorDelegate> m_editors{};
    };

    template <typename T>
    void TypeEditUIService::RegisterTypeEditor(Pfn_Editor<T> editor)
    {
        const auto thunk = [](const void* payload, const void* value, schema::StructBuilder data, Span<const schema::Field::Reader> path) -> bool
        {
            Pfn_Editor<T> editor = static_cast<Pfn_Editor<T>>(payload);
            return editor(*static_cast<const T*>(value), data, path);
        };
        RegisterTypeEditor(TypeInfo::Get<T>(), EditorDelegate::Make(thunk, editor));
    }
}

// Hash overloads
namespace std
{
    template <typename> struct hash;

    template <>
    struct hash<he::editor::TypeEditUIService::TypeEditorKey>
    {
        size_t operator()(const he::editor::TypeEditUIService::TypeEditorKey& value) const
        {
            return std::hash<he::Uuid>()(value.val);
        }
    };
}
