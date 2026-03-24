// Copyright Chad Engler

#pragma once

#include "he/scribe/context.h"

#include "he/core/assert.h"
#include "he/core/hash.h"
#include "he/core/memory_ops.h"
#include "he/core/span.h"

namespace he::scribe
{
    template <typename TReader>
    inline Span<const uint8_t> GetResourceBytes(const TReader& source)
    {
        const schema::StructReader& root = source;
        return {
            reinterpret_cast<const uint8_t*>(root.Data()),
            static_cast<uint32_t>((root.DataWordSize() + root.PointerCount()) * schema::BytesPerWord)
        };
    }

    inline uint64_t HashResourceBytes(Span<const uint8_t> bytes)
    {
        Hash<WyHash> hash{};
        hash.Update(bytes);
        return hash.Final();
    }

    inline uint64_t HashResourceStorage(Span<const schema::Word> storage)
    {
        return HashResourceBytes(storage.AsBytes());
    }

    template <typename TResource, typename TReader>
    inline TReader CopyOwnedResource(Vector<schema::Word>& storage, const TReader& source)
    {
        const Span<const uint8_t> bytes = GetResourceBytes(source);
        HE_ASSERT(IsAligned(bytes.Size(), schema::BytesPerWord));

        storage.Resize(bytes.Size() / schema::BytesPerWord, DefaultInit);
        MemCopy(storage.Data(), bytes.Data(), bytes.Size());
        return schema::ReadRoot<TResource>(storage.Data());
    }
}
