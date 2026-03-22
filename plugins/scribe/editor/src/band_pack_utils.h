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

    template <typename TBandRef>
    PackedBandStats AppendPackedBands(
        Vector<PackedBandTexel>& outBandTexels,
        uint32_t glyphBandStart,
        const Vector<Vector<TBandRef>>& horizontalBands,
        const Vector<Vector<TBandRef>>& verticalBands)
    {
        struct ExistingBandPayload
        {
            uint32_t offset{ 0 };
            uint32_t size{ 0 };
        };

        PackedBandStats stats{};
        stats.headerCount = horizontalBands.Size() + verticalBands.Size();

        const uint32_t payloadStart = glyphBandStart + stats.headerCount;
        outBandTexels.Resize(payloadStart);

        uint32_t currentOffset = stats.headerCount;
        Vector<ExistingBandPayload> existingBands{};
        auto appendBand = [&](uint32_t headerIndex, const Vector<TBandRef>& band)
        {
            PackedBandTexel header{};
            header.x = static_cast<uint16_t>(band.Size());

            if (!band.IsEmpty())
            {
                constexpr uint32_t InvalidOffset = 0xFFFFFFFFu;
                uint32_t reuseOffset = InvalidOffset;
                for (uint32_t existingIndex = 0; existingIndex < existingBands.Size(); ++existingIndex)
                {
                    const ExistingBandPayload& existing = existingBands[existingIndex];
                    if (existing.size != band.Size())
                    {
                        continue;
                    }

                    bool matches = true;
                    for (uint32_t curveIndex = 0; curveIndex < band.Size(); ++curveIndex)
                    {
                        const PackedBandTexel& existingTexel = outBandTexels[payloadStart + existing.offset + curveIndex];
                        if ((existingTexel.x != band[curveIndex].x) || (existingTexel.y != band[curveIndex].y))
                        {
                            matches = false;
                            break;
                        }
                    }

                    if (matches)
                    {
                        reuseOffset = existing.offset;
                        break;
                    }
                }

                if (reuseOffset != InvalidOffset)
                {
                    header.y = static_cast<uint16_t>(stats.headerCount + reuseOffset);
                    stats.reusedBandCount += 1;
                    stats.reusedPayloadTexelCount += band.Size();
                }
                else
                {
                    header.y = static_cast<uint16_t>(currentOffset);
                    ExistingBandPayload& existing = existingBands.EmplaceBack();
                    existing.offset = currentOffset - stats.headerCount;
                    existing.size = band.Size();

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
