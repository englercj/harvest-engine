// Copyright Chad Engler

#pragma once

#include "he/core/appender.h"
#include "he/core/enum_ops.h"
#include "he/core/enum_ops.h"
#include "he/core/string.h"
#include "he/core/type_traits.h"
#include "he/core/vector.h"

#include "fmt/format.h"
#include "rapidjson/document.h"

#include <concepts>
#include <list>
#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace he::schema
{
    // --------------------------------------------------------------------------------------------
    // Serialization to a JSON value

    template <typename T>
    rapidjson::Value ToJsonValue(rapidjson::Document& doc, const T& value);

    // Serialization of boolean types.
    template <>
    inline rapidjson::Value ToJsonValue(rapidjson::Document&, const bool& value)
    {
        return rapidjson::Value(value ? rapidjson::kTrueType : rapidjson::kFalseType);
    }

    // Serialization of numeric types.
    template <typename T> requires(std::is_integral_v<T> || std::is_floating_point_v<T>)
    rapidjson::Value ToJsonValue(rapidjson::Document&, const T& value)
    {
        return rapidjson::Value(value);
    }

    // Serialization of enum types.
    template <Enum T>
    rapidjson::Value ToJsonValue(rapidjson::Document&, const T& value)
    {
        return rapidjson::Value(rapidjson::StringRef(AsString(value)));
    }

    // Serialization of vectors.
    template <typename T>
    rapidjson::Value ToJsonValue(rapidjson::Document& doc, const Vector<T>& value)
    {
        rapidjson::Value v(rapidjson::kArrayType);
        v.Reserve(value.Size(), doc.GetAllocator());
        for (const T& item : value)
        {
            v.PushBack(ToJsonValue(doc, item).Move(), doc.GetAllocator());
        }
        return v;
    }

    // Serialization of fixed size arrays.
    template <typename T> requires(std::is_array_v<T>)
    rapidjson::Value ToJsonValue(rapidjson::Document& doc, const T& value)
    {
        constexpr uint32_t N = HE_LENGTH_OF(value);

        rapidjson::Value v(rapidjson::kArrayType);
        v.Reserve(N, doc.GetAllocator());
        for (uint32_t i = 0; i < N; ++i)
        {
            v.PushBack(ToJsonValue(doc, value[i]).Move(), doc.GetAllocator());
        }
        return v;
    }

    // Serialization of lists.
    template <typename T>
    rapidjson::Value ToJsonValue(rapidjson::Document& doc, const std::list<T>& value)
    {
        rapidjson::Value v(rapidjson::kArrayType);
        v.Reserve(value.size(), doc.GetAllocator());
        for (const T& item : value)
        {
            v.PushBack(ToJsonValue(doc, item).Move(), doc.GetAllocator());
        }
        return v;
    }

    // Serialization of maps.
    template <typename T, typename U>
    rapidjson::Value ToJsonValue(rapidjson::Document& doc, const std::unordered_map<T, U>& value)
    {
        String buf;
        rapidjson::Value obj(rapidjson::kObjectType);
        obj.MemberReserve(value.size(), doc.GetAllocator());
        for (auto&& it : value)
        {
            buf.Clear();
            fmt::format_to(Appender(buf), "{}", it->first);

            rapidjson::Value key(buf.Data(), buf.Size(), doc.GetAllocator());
            rapidjson::Value v = ToJsonValue(doc, it->second);

            obj.AddMember(key.Move(), v.Move(), doc.GetAllocator());
        }
        return obj;
    }

    // Serialization of sets.
    template <typename T>
    rapidjson::Value ToJsonValue(rapidjson::Document& doc, const std::unordered_set<T>& value)
    {
        rapidjson::Value v(rapidjson::kArrayType);
        v.Reserve(value.size(), doc.GetAllocator());
        for (const T& item : value)
        {
            v.PushBack(ToJsonValue(doc, item).Move(), doc.GetAllocator());
        }
        return v;
    }

    // Serialization of pointers.
    template <typename T>
    rapidjson::Value ToJsonValue(rapidjson::Document& doc, const std::unique_ptr<T>& value)
    {
        if (!value)
            return rapidjson::Value(rapidjson::kNullType);

        return ToJsonValue(doc, *value);
    }

    // Serialization of strings.
    template <>
    inline rapidjson::Value ToJsonValue(rapidjson::Document&, const String& value)
    {
        return rapidjson::Value(rapidjson::StringRef(value.Data(), value.Size()));
    }

    // --------------------------------------------------------------------------------------------
    // Deserialization from a JSON value

    template <typename T>
    bool FromJsonValue(const rapidjson::Value& value, T& out);

    // Deserialization of boolean types.
    template <>
    inline bool FromJsonValue(const rapidjson::Value& value, bool& out)
    {
        if (!value.IsFalse() && !value.IsTrue())
            return false;

        out = value.IsTrue();
        return true;
    }

    // Deserialization of integral types.
    template <typename T> requires(std::is_integral_v<T>)
    bool FromJsonValue(const rapidjson::Value& value, T& out)
    {
        if (value.IsInt())
            out = static_cast<T>(value.GetInt());
        else if (value.IsInt64())
            out = static_cast<T>(value.GetInt64());
        else if (value.IsUint())
            out = static_cast<T>(value.IsUint());
        else if (value.IsUint64())
            out = static_cast<T>(value.IsUint64());
        else
            return false;

        return true;
    }

    // Deserialization of floating point types.
    template <typename T> requires(std::is_floating_point_v<T>)
    bool FromJsonValue(const rapidjson::Value& value, T& out)
    {
        if (value.IsFloat())
            out = static_cast<T>(value.GetFloat());
        else if (value.IsDouble())
            out = static_cast<T>(value.GetDouble());
        else
            return false;

        return true;
    }

    // Deserialization of enum types.
    template <Enum T>
    bool FromJsonValue(const rapidjson::Value& value, T& out)
    {
        if (value.IsNumber())
        {
            out = static_cast<T>(value.GetInt64());
            return true;
        }

        if (value.IsString())
        {
            // TODO
            return true;
        }

        return false;
    }

    // Deserialization of vectors.
    template <typename T>
    bool FromJsonValue(const rapidjson::Value& value, Vector<T>& out)
    {
        if (!value.IsArray())
            return false;

        out.Reserve(static_cast<uint32_t>(value.Size()));
        for (const rapidjson::Value& item : value.GetArray())
        {
            T& v = out.EmplaceBack();
            if (!FromJsonValue(item, v))
                return false;
        }

        return true;
    }

    // Deserialization of fixed size arrays.
    template <typename T> requires(std::is_array_v<T>)
    bool FromJsonValue(const rapidjson::Value& value, T& out)
    {
        constexpr uint32_t N = HE_LENGTH_OF(out);

        if (!value.IsArray())
            return false;

        rapidjson::Value::ConstArray arr = value.GetArray();
        const uint32_t size = Min(N, static_cast<uint32_t>(arr.Size()));
        for (uint32_t i = 0; i < size; ++i)
        {
            if (!FromJsonValue(arr[i], out[i]))
                return false;
        }

        return true;
    }

    // Deserialization of lists.
    template <typename T>
    bool FromJsonValue(const rapidjson::Value& value, std::list<T>& out)
    {
        if (!value.IsArray())
            return false;

        for (const rapidjson::Value& item : value.GetArray())
        {
            T& v = out.emplace_back();
            if (!FromJsonValue(item, v))
                return false;
        }

        return true;
    }

    // Deserialization of maps.
    template <typename T, typename U>
    bool FromJsonValue(const rapidjson::Value& value, std::unordered_map<T, U>& out)
    {
        if (!value.IsObject())
            return false;

        out.reserve(value.MemberCount());
        for (auto&& item : value.GetObject())
        {
            T key{};
            if (!FromJsonValue(item.name, key))
                return false;

            U v{};
            if (!FromJsonValue(item.value, v))
                return false;

            out.emplace(Move(key), Move(value));
        }

        return true;
    }

    // Deserialization of sets.
    template <typename T>
    bool FromJsonValue(const rapidjson::Value& value, std::unordered_set<T>& out)
    {
        if (!value.IsArray())
            return false;

        out.reserve(value.Size());
        for (const rapidjson::Value& item : value.GetArray())
        {
            T v{};
            if (!FromJsonValue(item, v))
                return false;

            out.insert(Move(v));
        }

        return true;
    }

    // Deserialization of pointers.
    template <typename T>
    bool FromJsonValue(const rapidjson::Value& value, std::unique_ptr<T>& out)
    {
        if (value.IsNull())
        {
            out.reset();
            return true;
        }

        out = std::make_unique<T>();
        return FromJsonValue(value, *out);
    }

    // Deserialization of strings.
    template <>
    inline bool FromJsonValue(const rapidjson::Value& value, String& out)
    {
        if (!value.IsString())
            return false;

        out.Assign(value.GetString(), value.GetStringLength());
        return true;
    }

    // --------------------------------------------------------------------------------------------
    // Transform a structure into and from a json document

    template <typename T>
    rapidjson::Document ToJson(const T& value)
    {
        rapidjson::Document doc;
        rapidjson::Value v = ToJsonValue(doc, value);
        doc.Swap(v);
        return doc;
    }

    template <typename T>
    bool FromJson(const rapidjson::Document& doc, T& out)
    {
        return FromJsonValue(doc, out);
    }

    template <typename T, uint32_t N>
    inline bool FromJsonMember(const rapidjson::Value& value, const char (&name)[N], T& out)
    {
        if (!value.IsObject())
            return false;

        rapidjson::Value::ConstObject obj = value.GetObject();
        rapidjson::Value key(rapidjson::StringRef(name, N));
        rapidjson::Value::ConstMemberIterator it = obj.FindMember(key);

        if (it != obj.MemberEnd())
        {
            if (!FromJsonValue(it->value, out))
                return false;
        }

        return true;
    }
}
