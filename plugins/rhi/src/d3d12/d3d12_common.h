// Copyright Chad Engler

#pragma once

#include "he/core/wstr.h"
#include "he/rhi/types.h"

#if HE_RHI_ENABLE_D3D12

#if !defined(NOMINMAX)
    #define NOMINMAX
#endif

#pragma warning(push)
#pragma warning(disable : 4668)

#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>

#pragma warning(pop)

#define HE_DX_SAFE_RELEASE(x) do { if (x) { x->Release(); x = nullptr; } } while(0)

#if HE_RHI_ENABLE_NAMES
    #define HE_DX_SET_NAME(x, name) do { if (x && name) { x->SetName(HE_TO_WCSTR(name)); } } while(0)
#else
    #define HE_DX_SET_NAME(...)
#endif

#endif
