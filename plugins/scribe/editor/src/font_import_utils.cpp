// Copyright Chad Engler

#include "font_import_utils.h"

#include "he/core/file.h"
#include "he/core/log.h"
#include "he/core/path.h"
#include "he/core/result_fmt.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_TRUETYPE_TABLES_H

namespace he::scribe::editor
{
    namespace
    {
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

        String CopyFtString(const char* str)
        {
            return str != nullptr ? String(str) : String{};
        }
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

    bool InspectFontFace(const Vector<uint8_t>& sourceBytes, uint32_t faceIndex, FontSourceFormat sourceFormat, FontFaceInfo& out)
    {
        out = {};

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

        FT_Face ftFace = face.Get();
        out.faceIndex = faceIndex;
        out.faceCount = ftFace->num_faces > 0 ? static_cast<uint32_t>(ftFace->num_faces) : 1;
        out.sourceFormat = sourceFormat;
        out.familyName = CopyFtString(ftFace->family_name);
        out.styleName = CopyFtString(ftFace->style_name);
        out.postscriptName = CopyFtString(FT_Get_Postscript_Name(ftFace));
        out.glyphCount = ftFace->num_glyphs > 0 ? static_cast<uint32_t>(ftFace->num_glyphs) : 0;
        out.unitsPerEm = ftFace->units_per_EM > 0 ? static_cast<uint32_t>(ftFace->units_per_EM) : 0;
        out.maxAdvanceWidth = ftFace->max_advance_width > 0 ? static_cast<uint32_t>(ftFace->max_advance_width) : 0;
        out.maxAdvanceHeight = ftFace->max_advance_height > 0 ? static_cast<uint32_t>(ftFace->max_advance_height) : 0;
        out.ascender = static_cast<int32_t>(ftFace->ascender);
        out.descender = static_cast<int32_t>(ftFace->descender);
        out.lineHeight = static_cast<int32_t>(ftFace->height);
        out.isScalable = FT_IS_SCALABLE(ftFace);
        out.hasColorGlyphs = FT_HAS_COLOR(ftFace);
        out.hasKerning = FT_HAS_KERNING(ftFace);
        out.hasHorizontalLayout = FT_HAS_HORIZONTAL(ftFace);
        out.hasVerticalLayout = FT_HAS_VERTICAL(ftFace);

        const TT_OS2* os2 = static_cast<const TT_OS2*>(FT_Get_Sfnt_Table(ftFace, ft_sfnt_os2));
        if (os2 != nullptr)
        {
            out.capHeight = static_cast<int32_t>(os2->sCapHeight);
        }

        return true;
    }

    void FillFontFaceMetrics(FontFaceMetrics::Builder metrics, const FontFaceInfo& info)
    {
        metrics.SetUnitsPerEm(info.unitsPerEm);
        metrics.SetAscender(info.ascender);
        metrics.SetDescender(info.descender);
        metrics.SetLineHeight(info.lineHeight);
        metrics.SetMaxAdvanceWidth(info.maxAdvanceWidth);
        metrics.SetMaxAdvanceHeight(info.maxAdvanceHeight);
        metrics.SetCapHeight(info.capHeight);
    }

    void FillFontFaceImportMetadata(FontFaceImportMetadata::Builder metadata, const FontFaceInfo& info)
    {
        metadata.SetFaceIndex(info.faceIndex);
        metadata.SetSourceFormat(info.sourceFormat);
        metadata.InitFamilyName(info.familyName);
        metadata.InitStyleName(info.styleName);
        metadata.InitPostscriptName(info.postscriptName);
        metadata.SetGlyphCount(info.glyphCount);
        FillFontFaceMetrics(metadata.InitMetrics(), info);
        metadata.SetIsScalable(info.isScalable);
        metadata.SetHasColorGlyphs(info.hasColorGlyphs);
        metadata.SetHasKerning(info.hasKerning);
        metadata.SetHasHorizontalLayout(info.hasHorizontalLayout);
        metadata.SetHasVerticalLayout(info.hasVerticalLayout);
    }

    void FillFontFaceAssetData(ScribeFontFace::Builder assetData, const FontFaceInfo& info, bool preserveSourceBytesForShaping)
    {
        assetData.SetFaceIndex(info.faceIndex);
        assetData.SetPreserveSourceBytesForShaping(preserveSourceBytesForShaping);
        assetData.InitFamilyName(info.familyName);
        assetData.InitStyleName(info.styleName);
        assetData.InitPostscriptName(info.postscriptName);
        assetData.SetSourceFormat(info.sourceFormat);
        assetData.SetGlyphCount(info.glyphCount);
        FillFontFaceMetrics(assetData.InitMetrics(), info);
        assetData.SetIsScalable(info.isScalable);
        assetData.SetHasColorGlyphs(info.hasColorGlyphs);
        assetData.SetHasKerning(info.hasKerning);
        assetData.SetHasHorizontalLayout(info.hasHorizontalLayout);
        assetData.SetHasVerticalLayout(info.hasVerticalLayout);
    }
}
