/*
 * libjingle
 * Copyright 2015 Google Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

package org.webrtc;

import android.opengl.GLES11Ext;
import android.opengl.GLES20;

import org.webrtc.GlShader;
import org.webrtc.GlUtil;

import java.nio.ByteBuffer;
import java.nio.FloatBuffer;
import java.util.Arrays;
import java.util.IdentityHashMap;
import java.util.Map;

/**
 * Helper class to draw an opaque quad on the target viewport location. Rotation, mirror, and
 * cropping is specified using a 4x4 texture coordinate transform matrix. The frame input can either
 * be an OES texture or YUV textures in I420 format. The GL state must be preserved between draw
 * calls, this is intentional to maximize performance. The function release() must be called
 * manually to free the resources held by this object.
 */
public class GlRectDrawer implements RendererCommon.GlDrawer {
  // Simple vertex shader, used for both YUV and OES.
  private static final String VERTEX_SHADER_STRING =
        "varying vec2 interp_tc;\n"
      + "attribute vec4 in_pos;\n"
      + "attribute vec4 in_tc;\n"
      + "\n"
      + "uniform mat4 texMatrix;\n"
      + "\n"
      + "void main() {\n"
      + "    gl_Position = in_pos;\n"
      + "    interp_tc = (texMatrix * in_tc).xy;\n"
      + "}\n";

  private static final String YUV_FRAGMENT_SHADER_STRING =
        "precision mediump float;\n"
      + "varying vec2 interp_tc;\n"
      + "\n"
      + "uniform sampler2D y_tex;\n"
      + "uniform sampler2D u_tex;\n"
      + "uniform sampler2D v_tex;\n"
      + "\n"
      + "void main() {\n"
      // CSC according to http://www.fourcc.org/fccyvrgb.php
      + "  float y = texture2D(y_tex, interp_tc).r;\n"
      + "  float u = texture2D(u_tex, interp_tc).r - 0.5;\n"
      + "  float v = texture2D(v_tex, interp_tc).r - 0.5;\n"
      + "  gl_FragColor = vec4(y + 1.403 * v, "
      + "                      y - 0.344 * u - 0.714 * v, "
      + "                      y + 1.77 * u, 1);\n"
      + "}\n";

  private static final String RGB_FRAGMENT_SHADER_STRING =
        "precision mediump float;\n"
      + "varying vec2 interp_tc;\n"
      + "\n"
      + "uniform sampler2D rgb_tex;\n"
      + "\n"
      + "void main() {\n"
      + "  gl_FragColor = texture2D(rgb_tex, interp_tc);\n"
      + "}\n";

  private static final String OES_FRAGMENT_SHADER_STRING =
        "#extension GL_OES_EGL_image_external : require\n"
      + "precision mediump float;\n"
      + "varying vec2 interp_tc;\n"
      + "\n"
      + "uniform samplerExternalOES oes_tex;\n"
      + "\n"
      + "void main() {\n"
      + "  gl_FragColor = texture2D(oes_tex, interp_tc);\n"
      + "}\n";

  // Vertex coordinates in Normalized Device Coordinates, i.e. (-1, -1) is bottom-left and (1, 1) is
  // top-right.
  private static final FloatBuffer FULL_RECTANGLE_BUF =
      GlUtil.createFloatBuffer(new float[] {
            -1.0f, -1.0f,  // Bottom left.
             1.0f, -1.0f,  // Bottom right.
            -1.0f,  1.0f,  // Top left.
             1.0f,  1.0f,  // Top right.
          });

  // Texture coordinates - (0, 0) is bottom-left and (1, 1) is top-right.
  private static final FloatBuffer FULL_RECTANGLE_TEX_BUF =
      GlUtil.createFloatBuffer(new float[] {
            0.0f, 0.0f,  // Bottom left.
            1.0f, 0.0f,  // Bottom right.
            0.0f, 1.0f,  // Top left.
            1.0f, 1.0f   // Top right.
          });

  private static class Shader {
    public final GlShader glShader;
    public final int texMatrixLocation;

    public Shader(String fragmentShader) {
      this.glShader = new GlShader(VERTEX_SHADER_STRING, fragmentShader);
      this.texMatrixLocation = glShader.getUniformLocation("texMatrix");
    }
  }

  // The keys are one of the fragments shaders above.
  private final Map<String, Shader> shaders = new IdentityHashMap<String, Shader>();

  /**
   * Draw an OES texture frame with specified texture transformation matrix. Required resources are
   * allocated at the first call to this function.
   */
  @Override
  public void drawOes(int oesTextureId, float[] texMatrix, int x, int y, int width, int height) {
    prepareShader(OES_FRAGMENT_SHADER_STRING, texMatrix);
    GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
    // updateTexImage() may be called from another thread in another EGL context, so we need to
    // bind/unbind the texture in each draw call so that GLES understads it's a new texture.
    GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, oesTextureId);
    drawRectangle(x, y, width, height);
    GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, 0);
  }

  /**
   * Draw a RGB(A) texture frame with specified texture transformation matrix. Required resources
   * are allocated at the first call to this function.
   */
  @Override
  public void drawRgb(int textureId, float[] texMatrix, int x, int y, int width, int height) {
    prepareShader(RGB_FRAGMENT_SHADER_STRING, texMatrix);
    GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
    GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textureId);
    drawRectangle(x, y, width, height);
    // Unbind the texture as a precaution.
    GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, 0);
  }

  /**
   * Draw a YUV frame with specified texture transformation matrix. Required resources are
   * allocated at the first call to this function.
   */
  @Override
  public void drawYuv(int[] yuvTextures, float[] texMatrix, int x, int y, int width, int height) {
    prepareShader(YUV_FRAGMENT_SHADER_STRING, texMatrix);
    // Bind the textures.
    for (int i = 0; i < 3; ++i) {
      GLES20.glActiveTexture(GLES20.GL_TEXTURE0 + i);
      GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, yuvTextures[i]);
    }
    drawRectangle(x, y, width, height);
    // Unbind the textures as a precaution..
    for (int i = 0; i < 3; ++i) {
      GLES20.glActiveTexture(GLES20.GL_TEXTURE0 + i);
      GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, 0);
    }
  }

  private void drawRectangle(int x, int y, int width, int height) {
    // Draw quad.
    GLES20.glViewport(x, y, width, height);
    GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);
  }

  private void prepareShader(String fragmentShader, float[] texMatrix) {
    final Shader shader;
    if (shaders.containsKey(fragmentShader)) {
      shader = shaders.get(fragmentShader);
    } else {
      // Lazy allocation.
      shader = new Shader(fragmentShader);
      shaders.put(fragmentShader, shader);
      shader.glShader.useProgram();
      // Initialize fragment shader uniform values.
      if (fragmentShader == YUV_FRAGMENT_SHADER_STRING) {
        GLES20.glUniform1i(shader.glShader.getUniformLocation("y_tex"), 0);
        GLES20.glUniform1i(shader.glShader.getUniformLocation("u_tex"), 1);
        GLES20.glUniform1i(shader.glShader.getUniformLocation("v_tex"), 2);
      } else if (fragmentShader == RGB_FRAGMENT_SHADER_STRING) {
        GLES20.glUniform1i(shader.glShader.getUniformLocation("rgb_tex"), 0);
      } else if (fragmentShader == OES_FRAGMENT_SHADER_STRING) {
        GLES20.glUniform1i(shader.glShader.getUniformLocation("oes_tex"), 0);
      } else {
        throw new IllegalStateException("Unknown fragment shader: " + fragmentShader);
      }
      GlUtil.checkNoGLES2Error("Initialize fragment shader uniform values.");
      // Initialize vertex shader attributes.
      shader.glShader.setVertexAttribArray("in_pos", 2, FULL_RECTANGLE_BUF);
      shader.glShader.setVertexAttribArray("in_tc", 2, FULL_RECTANGLE_TEX_BUF);
    }
    shader.glShader.useProgram();
    // Copy the texture transformation matrix over.
    GLES20.glUniformMatrix4fv(shader.texMatrixLocation, 1, false, texMatrix, 0);
  }

  /**
   * Release all GLES resources. This needs to be done manually, otherwise the resources are leaked.
   */
  @Override
  public void release() {
    for (Shader shader : shaders.values()) {
      shader.glShader.release();
    }
    shaders.clear();
  }
}
