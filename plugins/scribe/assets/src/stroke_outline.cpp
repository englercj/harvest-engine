// Copyright Chad Engler

#include "he/scribe/stroke_outline.h"

#include "he/core/math.h"
#include "he/core/utils.h"
#include "he/core/log.h"
#include "he/core/scope_guard.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include FT_STROKER_H

namespace he::scribe
{
    namespace
    {
        constexpr float StrokeOutlineCoordScale = 1024.0f;

        FT_Stroker_LineJoin ToFreeTypeJoin(StrokeJoinKind join)
        {
            switch (join)
            {
                case StrokeJoinKind::Bevel:
                    return FT_STROKER_LINEJOIN_BEVEL;
                case StrokeJoinKind::Round:
                    return FT_STROKER_LINEJOIN_ROUND;
                case StrokeJoinKind::Miter:
                default:
                    return FT_STROKER_LINEJOIN_MITER_FIXED;
            }
        }

        FT_Stroker_LineCap ToFreeTypeCap(StrokeCapKind cap)
        {
            switch (cap)
            {
                case StrokeCapKind::Square:
                    return FT_STROKER_LINECAP_SQUARE;
                case StrokeCapKind::Round:
                    return FT_STROKER_LINECAP_ROUND;
                case StrokeCapKind::Butt:
                default:
                    return FT_STROKER_LINECAP_BUTT;
            }
        }

        FT_Vector ToFreeTypeVector(const StrokeOutlineSourcePoint& point)
        {
            FT_Vector result{};
            result.x = static_cast<FT_Pos>(Round(point.x * StrokeOutlineCoordScale));
            result.y = static_cast<FT_Pos>(Round(point.y * StrokeOutlineCoordScale));
            return result;
        }

        float FromFreeTypeCoord(FT_Pos value)
        {
            return static_cast<float>(value) / StrokeOutlineCoordScale;
        }

        class FreeTypeLibrary final
        {
        public:
            ~FreeTypeLibrary() noexcept
            {
                if (m_library != nullptr)
                {
                    FT_Done_FreeType(m_library);
                }
            }

            bool Initialize()
            {
                return FT_Init_FreeType(&m_library) == 0;
            }

            FT_Library Get() const { return m_library; }

        private:
            FT_Library m_library{ nullptr };
        };

        bool BuildStrokeInputPath(
            FT_Stroker stroker,
            Span<const StrokeOutlineSourcePoint> points,
            Span<const StrokeOutlineSourceCommand> commands)
        {
            auto isSubPathOpen = [&](uint32_t moveCommandIndex) -> bool
            {
                for (uint32_t commandIndex = moveCommandIndex + 1; commandIndex < commands.Size(); ++commandIndex)
                {
                    if (commands[commandIndex].type == StrokeCommandType::MoveTo)
                    {
                        break;
                    }

                    if (commands[commandIndex].type == StrokeCommandType::Close)
                    {
                        return false;
                    }
                }

                return true;
            };

            bool hasOpenSubPath = false;
            for (uint32_t commandIndex = 0; commandIndex < commands.Size(); ++commandIndex)
            {
                const StrokeOutlineSourceCommand& command = commands[commandIndex];
                switch (command.type)
                {
                    case StrokeCommandType::MoveTo:
                    {
                        if (command.firstPoint >= points.Size())
                        {
                            return false;
                        }

                        if (hasOpenSubPath && (FT_Stroker_EndSubPath(stroker) != 0))
                        {
                            return false;
                        }

                        FT_Vector point = ToFreeTypeVector(points[command.firstPoint]);
                        if (FT_Stroker_BeginSubPath(stroker, &point, isSubPathOpen(commandIndex)) != 0)
                        {
                            return false;
                        }

                        hasOpenSubPath = true;
                        break;
                    }

                    case StrokeCommandType::LineTo:
                    {
                        if (!hasOpenSubPath || (command.firstPoint >= points.Size()))
                        {
                            return false;
                        }

                        FT_Vector point = ToFreeTypeVector(points[command.firstPoint]);
                        if (FT_Stroker_LineTo(stroker, &point) != 0)
                        {
                            return false;
                        }
                        break;
                    }

                    case StrokeCommandType::QuadraticTo:
                    {
                        if (!hasOpenSubPath || ((command.firstPoint + 1) >= points.Size()))
                        {
                            return false;
                        }

                        FT_Vector control = ToFreeTypeVector(points[command.firstPoint]);
                        FT_Vector point = ToFreeTypeVector(points[command.firstPoint + 1]);
                        if (FT_Stroker_ConicTo(stroker, &control, &point) != 0)
                        {
                            return false;
                        }
                        break;
                    }

                    case StrokeCommandType::CubicTo:
                    {
                        if (!hasOpenSubPath || ((command.firstPoint + 2) >= points.Size()))
                        {
                            return false;
                        }

                        FT_Vector control1 = ToFreeTypeVector(points[command.firstPoint]);
                        FT_Vector control2 = ToFreeTypeVector(points[command.firstPoint + 1]);
                        FT_Vector point = ToFreeTypeVector(points[command.firstPoint + 2]);
                        if (FT_Stroker_CubicTo(stroker, &control1, &control2, &point) != 0)
                        {
                            return false;
                        }
                        break;
                    }

                    case StrokeCommandType::Close:
                    {
                        if (!hasOpenSubPath)
                        {
                            return false;
                        }

                        if (FT_Stroker_EndSubPath(stroker) != 0)
                        {
                            return false;
                        }

                        hasOpenSubPath = false;
                        break;
                    }
                }
            }

            if (hasOpenSubPath && (FT_Stroker_EndSubPath(stroker) != 0))
            {
                return false;
            }

            return true;
        }

        struct CurveExportState
        {
            Vector<StrokeOutlineCurve>* out{ nullptr };
            StrokeOutlineSourcePoint current{};
            bool hasCurrent{ false };
        };

        int ExportMoveTo(const FT_Vector* to, void* user)
        {
            CurveExportState& state = *static_cast<CurveExportState*>(user);
            state.current.x = FromFreeTypeCoord(to->x);
            state.current.y = FromFreeTypeCoord(to->y);
            state.hasCurrent = true;
            return 0;
        }

        int ExportLineTo(const FT_Vector* to, void* user)
        {
            CurveExportState& state = *static_cast<CurveExportState*>(user);
            if (!state.hasCurrent)
            {
                return 1;
            }

            StrokeOutlineCurve& curve = state.out->EmplaceBack();
            curve.kind = StrokeOutlineCurveKind::Line;
            curve.x0 = state.current.x;
            curve.y0 = state.current.y;
            curve.x1 = FromFreeTypeCoord(to->x);
            curve.y1 = FromFreeTypeCoord(to->y);
            state.current.x = curve.x1;
            state.current.y = curve.y1;
            return 0;
        }

        int ExportConicTo(const FT_Vector* control, const FT_Vector* to, void* user)
        {
            CurveExportState& state = *static_cast<CurveExportState*>(user);
            if (!state.hasCurrent)
            {
                return 1;
            }

            StrokeOutlineCurve& curve = state.out->EmplaceBack();
            curve.kind = StrokeOutlineCurveKind::Quadratic;
            curve.x0 = state.current.x;
            curve.y0 = state.current.y;
            curve.x1 = FromFreeTypeCoord(control->x);
            curve.y1 = FromFreeTypeCoord(control->y);
            curve.x2 = FromFreeTypeCoord(to->x);
            curve.y2 = FromFreeTypeCoord(to->y);
            state.current.x = curve.x2;
            state.current.y = curve.y2;
            return 0;
        }

        int ExportCubicTo(const FT_Vector* control1, const FT_Vector* control2, const FT_Vector* to, void* user)
        {
            CurveExportState& state = *static_cast<CurveExportState*>(user);
            if (!state.hasCurrent)
            {
                return 1;
            }

            StrokeOutlineCurve& curve = state.out->EmplaceBack();
            curve.kind = StrokeOutlineCurveKind::Cubic;
            curve.x0 = state.current.x;
            curve.y0 = state.current.y;
            curve.x1 = FromFreeTypeCoord(control1->x);
            curve.y1 = FromFreeTypeCoord(control1->y);
            curve.x2 = FromFreeTypeCoord(control2->x);
            curve.y2 = FromFreeTypeCoord(control2->y);
            curve.x3 = FromFreeTypeCoord(to->x);
            curve.y3 = FromFreeTypeCoord(to->y);
            state.current.x = curve.x3;
            state.current.y = curve.y3;
            return 0;
        }

        bool ExportStrokedOutlineCurves(Vector<StrokeOutlineCurve>& out, FT_Outline& outline)
        {
            out.Clear();

            CurveExportState state{};
            state.out = &out;

            FT_Outline_Funcs funcs{};
            funcs.move_to = &ExportMoveTo;
            funcs.line_to = &ExportLineTo;
            funcs.conic_to = &ExportConicTo;
            funcs.cubic_to = &ExportCubicTo;
            funcs.shift = 0;
            funcs.delta = 0;
            return FT_Outline_Decompose(&outline, &funcs, &state) == 0;
        }

        bool BuildStrokedOutlineCurvesInternal(
            Vector<StrokeOutlineCurve>& out,
            Span<const StrokeOutlineSourcePoint> points,
            Span<const StrokeOutlineSourceCommand> commands,
            const StrokeOutlineStyle& style)
        {
            out.Clear();
            if (!style.IsVisible() || commands.IsEmpty())
            {
                return false;
            }

            FreeTypeLibrary library{};
            if (!library.Initialize())
            {
                return false;
            }

            FT_Stroker stroker = nullptr;
            if (FT_Stroker_New(library.Get(), &stroker) != 0)
            {
                return false;
            }

            HE_AT_SCOPE_EXIT([&]()
            {
                FT_Stroker_Done(stroker);
            });

            FT_Stroker_Set(
                stroker,
                static_cast<FT_Fixed>(Round(style.width * 0.5f * StrokeOutlineCoordScale)),
                ToFreeTypeCap(style.cap),
                ToFreeTypeJoin(style.join),
                static_cast<FT_Fixed>(Round(style.miterLimit * 65536.0f)));

            if (!BuildStrokeInputPath(stroker, points, commands))
            {
                return false;
            }

            FT_UInt pointCount = 0;
            FT_UInt contourCount = 0;
            if (FT_Stroker_GetCounts(stroker, &pointCount, &contourCount) != 0
                || (pointCount == 0)
                || (contourCount == 0))
            {
                return false;
            }

            Vector<FT_Vector> outlinePoints{};
            Vector<uint8_t> outlineTags{};
            Vector<uint16_t> outlineContours{};
            outlinePoints.Resize(pointCount);
            outlineTags.Resize(pointCount);
            outlineContours.Resize(contourCount);

            FT_Outline outline{};
            outline.n_points = 0;
            outline.n_contours = 0;
            outline.points = outlinePoints.Data();
            outline.tags = outlineTags.Data();
            outline.contours = outlineContours.Data();

            FT_Stroker_Export(stroker, &outline);
            return ExportStrokedOutlineCurves(out, outline);
        }
    }

    bool BuildStrokedOutlineCurves(
        Vector<StrokeOutlineCurve>& out,
        Span<const StrokeOutlineSourcePoint> points,
        Span<const StrokeOutlineSourceCommand> commands,
        const StrokeOutlineStyle& style)
    {
        return BuildStrokedOutlineCurvesInternal(out, points, commands, style);
    }

    bool BuildStrokedOutlineCurves(
        Vector<StrokeOutlineCurve>& out,
        float pointScale,
        schema::List<StrokePoint>::Reader points,
        schema::List<StrokeCommand>::Reader commands,
        uint32_t firstCommand,
        uint32_t commandCount,
        const StrokeOutlineStyle& style)
    {
        out.Clear();
        if ((commandCount == 0) || ((firstCommand + commandCount) > commands.Size()))
        {
            return false;
        }

        Vector<StrokeOutlineSourcePoint> sourcePoints{};
        sourcePoints.Resize(points.Size());
        for (uint32_t pointIndex = 0; pointIndex < points.Size(); ++pointIndex)
        {
            sourcePoints[pointIndex].x = static_cast<float>(points[pointIndex].GetX()) * pointScale;
            sourcePoints[pointIndex].y = static_cast<float>(points[pointIndex].GetY()) * pointScale;
        }

        Vector<StrokeOutlineSourceCommand> sourceCommands{};
        sourceCommands.Resize(commandCount);
        for (uint32_t commandIndex = 0; commandIndex < commandCount; ++commandIndex)
        {
            const StrokeCommand::Reader command = commands[firstCommand + commandIndex];
            sourceCommands[commandIndex].type = command.GetType();
            sourceCommands[commandIndex].firstPoint = command.GetFirstPoint();
        }

        return BuildStrokedOutlineCurvesInternal(
            out,
            Span<const StrokeOutlineSourcePoint>(sourcePoints.Data(), sourcePoints.Size()),
            Span<const StrokeOutlineSourceCommand>(sourceCommands.Data(), sourceCommands.Size()),
            style);
    }
}
