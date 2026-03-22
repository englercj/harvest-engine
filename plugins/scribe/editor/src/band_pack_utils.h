// Copyright Chad Engler

#pragma once

#include "he/scribe/packed_data.h"

#include "he/core/span.h"
#include "he/core/vector.h"

namespace he::scribe::editor
{
    struct PackedBandStats
    {
        uint32_t headerCount{ 0 };
        uint32_t emittedPayloadTexelCount{ 0 };
        uint32_t reusedBandCount{ 0 };
        uint32_t reusedPayloadTexelCount{ 0 };
    };

    namespace detail
    {
        template <typename TBandRef>
        uint32_t FindReusableBandPayloadOffset(Span<const PackedBandTexel> existingPayload, const Vector<TBandRef>& band)
        {
            constexpr uint32_t InvalidOffset = 0xFFFFFFFFu;
            if (band.IsEmpty() || (existingPayload.Size() < band.Size()))
            {
                return InvalidOffset;
            }

            const uint32_t maxStart = existingPayload.Size() - band.Size();
            for (uint32_t start = 0; start <= maxStart; ++start)
            {
                bool matches = true;
                for (uint32_t index = 0; index < band.Size(); ++index)
                {
                    if ((existingPayload[start + index].x != band[index].x)
                        || (existingPayload[start + index].y != band[index].y))
                    {
                        matches = false;
                        break;
                    }
                }

                if (matches)
                {
                    return start;
                }
            }

            return InvalidOffset;
        }
    }

    template <typename TBandRef>
    PackedBandStats AppendPackedBands(
        Vector<PackedBandTexel>& outBandTexels,
        uint32_t glyphBandStart,
        const Vector<Vector<TBandRef>>& horizontalBands,
        const Vector<Vector<TBandRef>>& verticalBands)
    {
        PackedBandStats stats{};
        stats.headerCount = horizontalBands.Size() + verticalBands.Size();

        const uint32_t payloadStart = glyphBandStart + stats.headerCount;
        outBandTexels.Resize(payloadStart);

        uint32_t currentOffset = stats.headerCount;
        auto appendBand = [&](uint32_t headerIndex, const Vector<TBandRef>& band)
        {
            PackedBandTexel header{};
            header.x = static_cast<uint16_t>(band.Size());

            if (!band.IsEmpty())
            {
                const Span<const PackedBandTexel> existingPayload{
                    outBandTexels.Data() + payloadStart,
                    outBandTexels.Size() - payloadStart
                };

                const uint32_t reuseOffset = detail::FindReusableBandPayloadOffset(existingPayload, band);
                if (reuseOffset != 0xFFFFFFFFu)
                {
                    header.y = static_cast<uint16_t>(stats.headerCount + reuseOffset);
                    stats.reusedBandCount += 1;
                    stats.reusedPayloadTexelCount += band.Size();
                }
                else
                {
                    header.y = static_cast<uint16_t>(currentOffset);
                    for (uint32_t curveIndex = 0; curveIndex < band.Size(); ++curveIndex)
                    {
                        PackedBandTexel texel{};
                        texel.x = band[curveIndex].x;
                        texel.y = band[curveIndex].y;
                        outBandTexels.PushBack(texel);
                    }

                    currentOffset += band.Size();
                    stats.emittedPayloadTexelCount += band.Size();
                }
            }
            else
            {
                header.y = static_cast<uint16_t>(currentOffset);
            }

            outBandTexels[glyphBandStart + headerIndex] = header;
        };

        for (uint32_t bandIndex = 0; bandIndex < horizontalBands.Size(); ++bandIndex)
        {
            appendBand(bandIndex, horizontalBands[bandIndex]);
        }

        for (uint32_t bandIndex = 0; bandIndex < verticalBands.Size(); ++bandIndex)
        {
            appendBand(horizontalBands.Size() + bandIndex, verticalBands[bandIndex]);
        }

        return stats;
    }
}
