// Copyright Chad Engler

#include "font_import_utils.h"

#include "he/core/file.h"
#include "he/core/log.h"
#include "he/core/path.h"
#include "he/core/result_fmt.h"
#include "he/core/string_ops.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_FONT_FORMATS_H
#include FT_TRUETYPE_TABLES_H

#include <algorithm>
#include <cstring>

namespace he::scribe::editor
{
    namespace
    {
        struct FontTableRecord
        {
            uint32_t tag{ 0 };
            Vector<uint8_t> data{};
            uint32_t checksum{ 0 };
            uint32_t offset{ 0 };
        };

        void WriteU16BE(Vector<uint8_t>& out, uint16_t value)
        {
            out.PushBack(static_cast<uint8_t>((value >> 8) & 0xFFu));
            out.PushBack(static_cast<uint8_t>(value & 0xFFu));
        }

        void WriteU32BE(Vector<uint8_t>& out, uint32_t value)
        {
            out.PushBack(static_cast<uint8_t>((value >> 24) & 0xFFu));
            out.PushBack(static_cast<uint8_t>((value >> 16) & 0xFFu));
            out.PushBack(static_cast<uint8_t>((value >> 8) & 0xFFu));
            out.PushBack(static_cast<uint8_t>(value & 0xFFu));
        }

        void SetU32BE(Span<uint8_t> out, uint32_t value)
        {
            HE_ASSERT(out.Size() >= 4);
            out[0] = static_cast<uint8_t>((value >> 24) & 0xFFu);
            out[1] = static_cast<uint8_t>((value >> 16) & 0xFFu);
            out[2] = static_cast<uint8_t>((value >> 8) & 0xFFu);
            out[3] = static_cast<uint8_t>(value & 0xFFu);
        }

        uint32_t ComputeSfntChecksum(Span<const uint8_t> data)
        {
            uint32_t sum = 0;
            const uint32_t paddedSize = (data.Size() + 3u) & ~3u;
            for (uint32_t offset = 0; offset < paddedSize; offset += 4u)
            {
                uint32_t word = 0;
                for (uint32_t i = 0; i < 4u; ++i)
                {
                    word <<= 8u;
                    if ((offset + i) < data.Size())
                    {
                        word |= data[offset + i];
                    }
                }
                sum += word;
            }

            return sum;
        }

        uint16_t HighestPowerOfTwoLE(uint16_t value)
        {
            uint16_t result = 1;
            while ((result << 1u) <= value)
            {
                result <<= 1u;
            }

            return result;
        }

        FontSourceFormat ResolveFaceSourceFormat(FT_Face face, FontSourceFormat fallback)
        {
            const char* fontFormat = FT_Get_Font_Format(face);
            if (fontFormat == nullptr)
            {
                return fallback;
            }

            if (StrEqual(fontFormat, "CFF"))
            {
                return FontSourceFormat::OpenTypeCff;
            }

            if (StrEqual(fontFormat, "TrueType"))
            {
                return FontSourceFormat::TrueType;
            }

            return fallback;
        }

        bool ExtractFontFaceSourceBytes(Vector<uint8_t>& out, FT_Face ftFace)
        {
            out.Clear();

            FT_ULong tableCount = 0;
            if (FT_Sfnt_Table_Info(ftFace, 0, nullptr, &tableCount) != 0 || tableCount == 0)
            {
                HE_LOG_ERROR(he_scribe,
                    HE_MSG("Failed to enumerate SFNT tables for face extraction."));
                return false;
            }

            Vector<FontTableRecord> tables{};
            tables.Reserve(static_cast<uint32_t>(tableCount));

            for (FT_ULong tableIndex = 0; tableIndex < tableCount; ++tableIndex)
            {
                FT_ULong tag = 0;
                FT_ULong length = 0;
                if (FT_Sfnt_Table_Info(ftFace, static_cast<FT_UInt>(tableIndex), &tag, &length) != 0 || length == 0)
                {
                    continue;
                }

                FontTableRecord& table = tables.EmplaceBack();
                table.tag = static_cast<uint32_t>(tag);
                table.data.Resize(static_cast<uint32_t>(length), DefaultInit);
                FT_ULong loadLength = length;
                if (FT_Load_Sfnt_Table(ftFace, tag, 0, table.data.Data(), &loadLength) != 0 || loadLength != length)
                {
                    return false;
                }

                if (table.tag == FT_MAKE_TAG('h', 'e', 'a', 'd') && table.data.Size() >= 12u)
                {
                    table.data[8] = 0;
                    table.data[9] = 0;
                    table.data[10] = 0;
                    table.data[11] = 0;
                }

                table.checksum = ComputeSfntChecksum(table.data);
            }

            std::sort(tables.Data(), tables.Data() + tables.Size(), [](const FontTableRecord& a, const FontTableRecord& b)
            {
                return a.tag < b.tag;
            });

            const FontSourceFormat faceFormat = ResolveFaceSourceFormat(ftFace, FontSourceFormat::Unknown);
            const uint32_t sfntVersion = faceFormat == FontSourceFormat::OpenTypeCff
                ? FT_MAKE_TAG('O', 'T', 'T', 'O')
                : 0x00010000u;

            const uint16_t numTables = static_cast<uint16_t>(tables.Size());
            const uint16_t maxPowerOfTwo = HighestPowerOfTwoLE(numTables);
            const uint16_t searchRange = static_cast<uint16_t>(maxPowerOfTwo * 16u);
            uint16_t entrySelector = 0;
            for (uint16_t value = maxPowerOfTwo; value > 1u; value >>= 1u)
            {
                ++entrySelector;
            }
            const uint16_t rangeShift = static_cast<uint16_t>((numTables * 16u) - searchRange);

            const uint32_t directorySize = 12u + (static_cast<uint32_t>(numTables) * 16u);
            uint32_t currentOffset = directorySize;
            for (FontTableRecord& table : tables)
            {
                table.offset = currentOffset;
                currentOffset += (table.data.Size() + 3u) & ~3u;
            }

            out.Reserve(currentOffset);
            WriteU32BE(out, sfntVersion);
            WriteU16BE(out, numTables);
            WriteU16BE(out, searchRange);
            WriteU16BE(out, entrySelector);
            WriteU16BE(out, rangeShift);

            uint32_t headTableOffset = 0;
            for (const FontTableRecord& table : tables)
            {
                WriteU32BE(out, table.tag);
                WriteU32BE(out, table.checksum);
                WriteU32BE(out, table.offset);
                WriteU32BE(out, table.data.Size());
                if (table.tag == FT_MAKE_TAG('h', 'e', 'a', 'd'))
                {
                    headTableOffset = table.offset;
                }
            }

            for (const FontTableRecord& table : tables)
            {
                for (uint32_t byteIndex = 0; byteIndex < table.data.Size(); ++byteIndex)
                {
                    out.PushBack(table.data[byteIndex]);
                }
                while ((out.Size() & 3u) != 0u)
                {
                    out.PushBack(0);
                }
            }

            if (headTableOffset != 0 && (headTableOffset + 12u) <= out.Size())
            {
                SetU32BE(Span<uint8_t>(out.Data() + headTableOffset + 8u, 4u), 0);
                const uint32_t checksumAdjustment = 0xB1B0AFBAu - ComputeSfntChecksum(out);
                SetU32BE(Span<uint8_t>(out.Data() + headTableOffset + 8u, 4u), checksumAdjustment);
            }

            return true;
        }

        bool PopulateFontFaceInfo(FontFaceInfo& out, FT_Face ftFace, uint32_t faceIndex)
        {
            out = {};
            out.faceIndex = faceIndex;
            out.faceCount = ftFace->num_faces > 0 ? static_cast<uint32_t>(ftFace->num_faces) : 1;
            out.familyName = ftFace->family_name != nullptr ? String(ftFace->family_name) : String{};
            out.styleName = ftFace->style_name != nullptr ? String(ftFace->style_name) : String{};
            out.unitsPerEm = ftFace->units_per_EM > 0 ? static_cast<uint32_t>(ftFace->units_per_EM) : 0;
            out.ascender = static_cast<int32_t>(ftFace->ascender);
            out.descender = static_cast<int32_t>(ftFace->descender);
            out.lineHeight = static_cast<int32_t>(ftFace->height);
            out.hasColorGlyphs = FT_HAS_COLOR(ftFace);

            const TT_OS2* os2 = static_cast<const TT_OS2*>(FT_Get_Sfnt_Table(ftFace, ft_sfnt_os2));
            if (os2 != nullptr)
            {
                out.capHeight = static_cast<int32_t>(os2->sCapHeight);
            }

            return true;
        }

        class FreeTypeLibrary final
        {
        public:
            FreeTypeLibrary() = default;
            ~FreeTypeLibrary() noexcept
            {
                if (m_library)
                {
                    FT_Done_FreeType(m_library);
                }
            }

            bool Initialize()
            {
                if (m_library)
                {
                    return true;
                }

                const FT_Error err = FT_Init_FreeType(&m_library);
                if (err != 0)
                {
                    HE_LOG_ERROR(he_scribe,
                        HE_MSG("Failed to initialize FreeType."),
                        HE_KV(error, err));
                    return false;
                }

                return true;
            }

            FT_Library Get() const { return m_library; }

        private:
            FT_Library m_library{ nullptr };
        };

        class FreeTypeFace final
        {
        public:
            FreeTypeFace() = default;
            ~FreeTypeFace() noexcept
            {
                if (m_face)
                {
                    FT_Done_Face(m_face);
                }
            }

            bool Load(FT_Library library, const Vector<uint8_t>& sourceBytes, uint32_t faceIndex)
            {
                const FT_Error err = FT_New_Memory_Face(
                    library,
                    reinterpret_cast<const FT_Byte*>(sourceBytes.Data()),
                    static_cast<FT_Long>(sourceBytes.Size()),
                    static_cast<FT_Long>(faceIndex),
                    &m_face);
                if (err != 0)
                {
                    HE_LOG_ERROR(he_scribe,
                        HE_MSG("Failed to load font face from memory."),
                        HE_KV(face_index, faceIndex),
                        HE_KV(error, err));
                    return false;
                }

                return true;
            }

            FT_Face Get() const { return m_face; }

        private:
            FT_Face m_face{ nullptr };
        };
    }

    FontSourceFormat DeduceFontSourceFormat(const char* file)
    {
        const StringView ext = GetExtension(file);
        if (ext.EqualToI(".ttf"))
        {
            return FontSourceFormat::TrueType;
        }

        if (ext.EqualToI(".otf"))
        {
            return FontSourceFormat::OpenTypeCff;
        }

        if (ext.EqualToI(".ttc"))
        {
            return FontSourceFormat::TrueTypeCollection;
        }

        if (ext.EqualToI(".otc"))
        {
            return FontSourceFormat::OpenTypeCollection;
        }

        return FontSourceFormat::Unknown;
    }

    bool ReadFontSourceBytes(Vector<uint8_t>& out, const char* file)
    {
        Result r = File::ReadAll(out, file);
        if (!r)
        {
            HE_LOG_ERROR(he_scribe,
                HE_MSG("Failed to read font source bytes."),
                HE_KV(path, file),
                HE_KV(result, r));
            return false;
        }

        return !out.IsEmpty();
    }

    bool InspectFontFace(const Vector<uint8_t>& sourceBytes, uint32_t faceIndex, FontFaceInfo& out)
    {
        FreeTypeLibrary library;
        if (!library.Initialize())
        {
            return false;
        }

        FreeTypeFace face;
        if (!face.Load(library.Get(), sourceBytes, faceIndex))
        {
            return false;
        }

        return PopulateFontFaceInfo(out, face.Get(), faceIndex);
    }

    bool InspectFontFaces(Vector<FontFaceInfo>& out, const Vector<uint8_t>& sourceBytes)
    {
        out.Clear();

        FreeTypeLibrary library;
        if (!library.Initialize())
        {
            return false;
        }

        FreeTypeFace firstFace;
        if (!firstFace.Load(library.Get(), sourceBytes, 0))
        {
            return false;
        }

        const FT_Face ftFirstFace = firstFace.Get();
        const uint32_t faceCount = ftFirstFace->num_faces > 0 ? static_cast<uint32_t>(ftFirstFace->num_faces) : 1;
        out.Reserve(faceCount);

        FontFaceInfo firstFaceInfo{};
        PopulateFontFaceInfo(firstFaceInfo, ftFirstFace, 0);
        out.PushBack(Move(firstFaceInfo));

        for (uint32_t faceIndex = 1; faceIndex < faceCount; ++faceIndex)
        {
            FreeTypeFace face;
            if (!face.Load(library.Get(), sourceBytes, faceIndex))
            {
                out.Clear();
                return false;
            }

            FontFaceInfo& info = out.EmplaceBack();
            PopulateFontFaceInfo(info, face.Get(), faceIndex);
        }

        return true;
    }

    bool ExtractFontFaceSourceBytes(Vector<uint8_t>& out, const Vector<uint8_t>& sourceBytes, uint32_t faceIndex)
    {
        FreeTypeLibrary library;
        if (!library.Initialize())
        {
            return false;
        }

        FreeTypeFace face;
        if (!face.Load(library.Get(), sourceBytes, faceIndex))
        {
            return false;
        }

        return ExtractFontFaceSourceBytes(out, face.Get());
    }

    void FillFontFaceMetrics(FontFaceMetrics::Builder metrics, const FontFaceInfo& info)
    {
        metrics.SetUnitsPerEm(info.unitsPerEm);
        metrics.SetAscender(info.ascender);
        metrics.SetDescender(info.descender);
        metrics.SetLineHeight(info.lineHeight);
        metrics.SetCapHeight(info.capHeight);
    }

    void FillFontFaceAssetData(ScribeFontFace::Builder assetData, const FontFaceInfo& info)
    {
        assetData.SetFaceIndex(info.faceIndex);
        FillFontFaceMetrics(assetData.InitMetrics(), info);
        assetData.SetHasColorGlyphs(info.hasColorGlyphs);
    }
}
