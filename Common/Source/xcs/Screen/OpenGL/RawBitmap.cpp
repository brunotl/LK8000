/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "../RawBitmap.hpp"
#include "Canvas.hpp"
#include "Texture.hpp"
#include "Scope.hpp"

#ifdef USE_GLSL
#include "Shaders.hpp"
#include "Program.hpp"
#else
#include "Compatibility.hpp"
#endif

#include <assert.h>

/**
 * Returns minimum width that is greater then the given width and
 * that is acceptable as image width (width must be multiple of 4)
 */
static inline unsigned
CorrectedWidth(unsigned nWidth)
{
  return ((nWidth + 3) / 4) * 4;
}

RawBitmap::RawBitmap(unsigned nWidth, unsigned nHeight)
  :width(nWidth), height(nHeight),
   corrected_width(CorrectedWidth(nWidth)),
   texture(new GLTexture(CorrectedWidth(nWidth), nHeight)),
   dirty(true)
{
  assert(nWidth > 0);
  assert(nHeight > 0);

  texture->EnableInterpolation();

  buffer = new BGRColor[corrected_width * height];
}

RawBitmap::~RawBitmap()
{
  delete texture;
  delete[] buffer;
}

GLTexture &
RawBitmap::BindAndGetTexture() const
{
  texture->Bind();

  if (dirty) {
#ifdef HAVE_GLES
    /* 16 bit 5/6/5 on Android */
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, corrected_width, this->height,
                    GL_RGB, GL_UNSIGNED_SHORT_5_6_5, buffer);
#else
    /* 32 bit R/G/B/A on full OpenGL */
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, corrected_width, this->height,
                    GL_BGRA, GL_UNSIGNED_BYTE, buffer);
#endif

    dirty = false;
  }

  return *texture;
}

void
RawBitmap::StretchTo(unsigned width, unsigned height,
                     Canvas &dest_canvas,
                     unsigned dest_x, unsigned dest_y, unsigned dest_width, unsigned dest_height) const
{
  GLTexture &texture = BindAndGetTexture();

#ifdef USE_GLSL
  OpenGL::texture_shader->Use();
#else
  const GLEnable<GL_TEXTURE_2D> scope;
  OpenGL::glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
#endif

  texture.Draw(dest_x, dest_y, dest_width, dest_height, 0, 0, width, height);
}
