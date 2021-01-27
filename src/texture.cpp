#include "texture.h"
#include "color.h"

#include <assert.h>
#include <iostream>
#include <algorithm>

using namespace std;

namespace CS248 {

inline void uint8_to_float( float dst[4], unsigned char* src ) {
  uint8_t* src_uint8 = (uint8_t *)src;
  dst[0] = src_uint8[0] / 255.f;
  dst[1] = src_uint8[1] / 255.f;
  dst[2] = src_uint8[2] / 255.f;
  dst[3] = src_uint8[3] / 255.f;
}

inline void float_to_uint8( unsigned char* dst, float src[4] ) {
  uint8_t* dst_uint8 = (uint8_t *)dst;
  dst_uint8[0] = (uint8_t) ( 255.f * max( 0.0f, min( 1.0f, src[0])));
  dst_uint8[1] = (uint8_t) ( 255.f * max( 0.0f, min( 1.0f, src[1])));
  dst_uint8[2] = (uint8_t) ( 255.f * max( 0.0f, min( 1.0f, src[2])));
  dst_uint8[3] = (uint8_t) ( 255.f * max( 0.0f, min( 1.0f, src[3])));
}

void Sampler2DImp::generate_mips(Texture& tex, int startLevel) {

  // NOTE: 
  // This starter code allocates the mip levels and generates a level 
  // map by filling each level with placeholder data in the form of a 
  // color that differs from its neighbours'. You should instead fill
  // with the correct data!

  // Extra credit: Implement this

  // check start level
  if ( startLevel >= tex.mipmap.size() ) {
    std::cerr << "Invalid start level"; 
  }

  // allocate sublevels
  int baseWidth  = tex.mipmap[startLevel].width;
  int baseHeight = tex.mipmap[startLevel].height;
  int numSubLevels = (int)(log2f( (float)max(baseWidth, baseHeight)));

  numSubLevels = min(numSubLevels, kMaxMipLevels - startLevel - 1);
  tex.mipmap.resize(startLevel + numSubLevels + 1);

  int width  = baseWidth;
  int height = baseHeight;
  for (int i = 1; i <= numSubLevels; i++) {

    MipLevel& level = tex.mipmap[startLevel + i];

    // handle odd size texture by rounding down
    width  = max( 1, width  / 2); assert(width  > 0);
    height = max( 1, height / 2); assert(height > 0);

    level.width = width;
    level.height = height;
    level.texels = vector<unsigned char>(4 * width * height);

  }

  // fill all 0 sub levels with interchanging colors
  Color colors[3] = { Color(1,0,0,1), Color(0,1,0,1), Color(0,0,1,1) };
  for(size_t i = 1; i < tex.mipmap.size(); ++i) {

    Color c = colors[i % 3];
    MipLevel& mip = tex.mipmap[i];

    for(size_t i = 0; i < 4 * mip.width * mip.height; i += 4) {
      float_to_uint8( &mip.texels[i], &c.r );
    }
  }

}

Color Sampler2DImp::sample_nearest(Texture &tex,
                                   float u, float v,
                                   int level)
{

  // Task 4: Implement nearest neighbour interpolation
  if (level == 0)
  {
    const auto& mip = tex.mipmap.at(level);
    float tx = u * mip.width;
    float ty = v * mip.height;
    int x = floor(tx);
    int y = floor(ty);
    Color nearest;
    nearest.r = mip.texels.at(4 * (x + y * mip.width)) / 255.0;
    nearest.g = mip.texels.at(4 * (x + y * mip.width) + 1) / 255.0;
    nearest.b = mip.texels.at(4 * (x + y * mip.width) + 2) / 255.0;
    nearest.a = mip.texels.at(4 * (x + y * mip.width) + 3) / 255.0;
    return nearest;
  }
  // return magenta for invalid level
  return Color(1, 0, 1, 1);
}

Color Sampler2DImp::sample_bilinear(Texture &tex,
                                    float u, float v,
                                    int level)
{

  // Task 4: Implement bilinear filtering
  if (level >= tex.mipmap.size())
  {
    // return magenta for invalid level
    return Color(1, 0, 1, 1);
  }
  auto lerp = [](float x, Color v0, Color v1) -> Color {
    return v0 + x * (v1 + (-1 * v0));
  };
  const auto &mip = tex.mipmap.at(level);
  float tx = u * mip.width;
  float ty = v * mip.height;

  auto get_color = [&mip](int x, int y) -> Color {
    Color c;
    c.r = mip.texels.at(4 * (x + y * mip.width)) / 255.f;
    c.g = mip.texels.at(4 * (x + y * mip.width) + 1) / 255.f;
    c.b = mip.texels.at(4 * (x + y * mip.width) + 2) / 255.f;
    c.a = mip.texels.at(4 * (x + y * mip.width) + 3) / 255.f;
    return c;
  };

  int lx = floor(tx - 0.5);
  int ly = floor(ty - 0.5);
  // clamp edges
  if (lx < 0 || lx == mip.width - 1 || ly < 0 || ly == mip.height - 1) {
    return get_color(max(lx, 0), max(ly, 0));
  }

  Color u01 = get_color(lx, ly);
  Color u11 = get_color(lx + 1, ly);
  Color u00 = get_color(lx, ly + 1);
  Color u10 = get_color(lx + 1, ly + 1);
  float s = tx - (lx + 0.5);
  float t = ly + 1.5 - ty;
  Color u0 = lerp(s, u00, u10);
  Color u1 = lerp(s, u01, u11);
  return lerp(t, u0, u1);
}

Color Sampler2DImp::sample_trilinear(Texture &tex,
                                     float u, float v,
                                     float u_scale, float v_scale)
{

  // Extra credit: Implement trilinear filtering

  // return magenta for invalid level
  return Color(1,0,1,1);
}

} // namespace CS248
