/*
Copyright_License {

  XCSoar Glide Compute5r - http://www.xcsoar.org/
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

#ifndef XCSOAR_SCREEN_OPENGL_SHADERS_HPP
#define XCSOAR_SCREEN_OPENGL_SHADERS_HPP

#include "System.hpp"
#include <memory>

class GLProgram;

namespace OpenGL {
  namespace Attribute {
    static constexpr GLuint POSITION = 1;
    static constexpr GLuint TEXCOORD = 2;
    static constexpr GLuint COLOR = 3;
  };

  using program_unique_ptr = std::unique_ptr<GLProgram>;
  
  /**
   * A shader that draws a solid color (#Attribute::COLOR).
   */
  extern program_unique_ptr solid_shader;
  extern GLint solid_projection, solid_modelview;

  /**
   * A shader that copies the texture.
   */
  extern program_unique_ptr texture_shader;
  extern GLint texture_projection, texture_texture;

  /**
   * A shader that copies the inverted texture.
   */
  extern program_unique_ptr invert_shader;
  extern GLint invert_projection, invert_texture;

  /**
   * A shader that copies the texture's alpha channel, but replaces
   * the color (#Attribute::COLOR).
   */
  extern program_unique_ptr alpha_shader;
  extern GLint alpha_projection, alpha_texture;

  void InitShaders();
  void DeinitShaders() noexcept;

  void UpdateShaderProjectionMatrix() noexcept;

  void UpdateShaderTranslate() noexcept;
};

#endif
