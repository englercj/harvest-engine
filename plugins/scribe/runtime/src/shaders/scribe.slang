// Copyright Chad Engler
//
// Adapted from the public Slug reference shaders.
// SPDX-License-Identifier: MIT
// Slug shader code Copyright 2017 by Eric Lengyel.
// Original source from: https://github.com/EricLengyel/Slug

import slug;

struct VertexInput
{
    // pos.xy = object-space vertex coordinates.
    // pos.zw = object-space normal vector.
    float4 pos : ATTRIB0;

    // tex.xy = em-space sample coordinates.
    //
    // tex.z = location of glyph data in band texture (interpreted as integer):
    // | 31                         24 | 23                         16 | 15                          8 | 7                           0 |
    // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    // |           y coordinate of glyph data in band texture          |           x coordinate of glyph data in band texture          |
    // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    //
    // tex.w = max band indexes and flags (interpreted as integer):
    // | 31                         24 | 23                         16 | 15                          8 | 7                           0 |
    // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    // | 0   0   0 | E | 0   0   0   0 |           band max y          | 0   0   0   0   0   0   0   0 |           band max x          |
    // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
    float4 tex : ATTRIB1;

    // jac = inverse Jacobian matrix entries (00, 01, 10, 11).
    float4 jac : ATTRIB2;

    // bnd = (band scale x, band scale y, band offset x, band offset y).
    float4 bnd : ATTRIB3;

    // col = vertex color (red, green, blue, alpha).
    float4 col : ATTRIB4;
};

struct VertexOutput
{
    // Clip-space vertex position.
    float4 position : SV_Position;

    // Vertex color.
    float4 color : U_COLOR;

    // Em-space sample coordinates.
    float2 texcoord : U_TEXCOORD;

    // Band scale and offset, constant over glyph.
    nointerpolation float4 banding : U_BANDING;

    // (glyph data x coord, glyph data y coord, band max x, band max y and flags), constant over glyph.
    nointerpolation int4 glyph : U_GLYPH;
};

cbuffer ParamStruct : register(b0)
{
    // The four rows of the Model View Projection (MVP) matrix.
    float4 mvp[4];

    // The viewport dimensions, in pixels.
    float4 viewport;
};

Texture2D curveTexture : register(t0);
Texture2D<uint2> bandTexture : register(t1);

[shader("vertex")]
VertexOutput vs_main(VertexInput input, uint vid : SV_VertexID)
{
    float2 p;
    VertexOutput vresult;

    // Apply dynamic dilation to vertex position. Returns new em-space sample position.

    vresult.texcoord = SlugDilate(input.pos, input.tex, input.jac, mvp[0], mvp[1], mvp[3], viewport.xy, p);

    // Apply MVP matrix to dilated vertex position.

    vresult.position.x = p.x * mvp[0].x + p.y * mvp[0].y + mvp[0].w;
    vresult.position.y = p.x * mvp[1].x + p.y * mvp[1].y + mvp[1].w;
    vresult.position.z = p.x * mvp[2].x + p.y * mvp[2].y + mvp[2].w;
    vresult.position.w = p.x * mvp[3].x + p.y * mvp[3].y + mvp[3].w;

    // Unpack or pass through remaining vertex data.

    SlugUnpack(input.tex, input.bnd, vresult.banding, vresult.glyph);
    vresult.color = input.col;
    return vresult;
}

[shader("pixel")]
float4 ps_main(VertexOutput input) : SV_Target
{
    float coverage = SlugRender(curveTexture, bandTexture, input.texcoord, input.banding, input.glyph);
    return input.color * coverage;
}
