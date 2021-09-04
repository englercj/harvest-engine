// STB Compress (https://github.com/jrk/stb/blob/master/stb.h)

#pragma once

typedef unsigned int    stb_uint;
typedef unsigned char   stb_uchar;
typedef unsigned int    stb_uint32;

stb_uint stb_compress(stb_uchar* out, stb_uchar* in, stb_uint len);
