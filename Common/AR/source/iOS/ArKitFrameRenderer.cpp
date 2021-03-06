/*******************************************************************************
 *  Copyright 2012-2019 Esri
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 ******************************************************************************/

#include "ArKitFrameRenderer.h"
#include <QOpenGLShaderProgram>
#include <QGuiApplication>
#include <QScreen>

using namespace Esri::ArcGISRuntime::Toolkit::Internal;

// This class renders the textures (created from the AR frames) into a quad using OpenGL.
// The first texture contains the Y component and the second texture contains the CbCr components.
// The RGB final color is created using the space color matrix transformation given in the documentation:
// https://developer.apple.com/documentation/arkit/displaying_an_ar_experience_with_metal?language=objc

namespace {
// positions of the quad vertices in space
const GLfloat kVerticesReversePortrait[] = {
  +1.0f, +1.0f,
  -1.0f, +1.0f,
  +1.0f, -1.0f,
  -1.0f, -1.0f
};
const GLfloat kVerticesRightLandscape[] = {
  -1.0f, +1.0f,
  -1.0f, -1.0f,
  +1.0f, +1.0f,
  +1.0f, -1.0f
};
const GLfloat kVerticesLeftLandscape[] = {
  +1.0f, -1.0f,
  +1.0f, +1.0f,
  -1.0f, -1.0f,
  -1.0f, +1.0f
};
const GLfloat kVerticesPortrait[] = {
  -1.0f, -1.0f,
  +1.0f, -1.0f,
  -1.0f, +1.0f,
  +1.0f, +1.0f
};

// positions of the texture in the quad
const GLfloat kTextures[] = {
  0.0f, 1.0f,
  0.0f, 0.0f,
  1.0f, 1.0f,
  1.0f, 0.0f
};
} // anonymous namespace

ArKitFrameRenderer::ArKitFrameRenderer() :
  m_textureY(QOpenGLTexture::Target2D),
  m_textureCbCr(QOpenGLTexture::Target2D)
{
}

ArKitFrameRenderer::~ArKitFrameRenderer()
{
}

/*!
  \internal
  This functions runs on the rendering thread.
 */
void ArKitFrameRenderer::initGL()
{
  initializeOpenGLFunctions();

  m_program.reset(new QOpenGLShaderProgram());

  m_program->addCacheableShaderFromSourceCode(QOpenGLShader::Vertex,
                                              "attribute vec4 a_position;"
                                              "attribute vec2 a_texCoord;"
                                              "uniform vec2 u_verticesRatio;"
                                              "varying vec2 v_texCoord;"
                                              "void main() {"
                                              "  float x = a_position.x * u_verticesRatio.x;"
                                              "  float y = a_position.y * u_verticesRatio.y;"
                                              "  gl_Position = vec4(x, y, a_position.zw);"
                                              "  v_texCoord = a_texCoord;"
                                              "}");

  m_program->addCacheableShaderFromSourceCode(QOpenGLShader::Fragment,
                                              "precision mediump float;"
                                              "varying vec2 v_texCoord;"
                                              "uniform sampler2D u_textureY;"
                                              "uniform sampler2D u_textureCbCr;"
                                              "const mat4 ycbcrToRgb = mat4("
                                              "  vec4(+1.0000, +1.0000, +1.0000, +0.0000),"
                                              "  vec4(+0.0000, -0.3441, +1.7720, +0.0000),"
                                              "  vec4(+1.4020, -0.7141, +0.0000, +0.0000),"
                                              "  vec4(-0.7010, +0.5291, -0.8860, +1.0000)"
                                              ");"
                                              "void main() {"
                                              "  float y = texture2D(u_textureY, v_texCoord).r;"
                                              "  vec2 cbcr = texture2D(u_textureCbCr, v_texCoord).rg;"
                                              "  gl_FragColor = ycbcrToRgb * vec4(y, cbcr.r, cbcr.g, 1.0);"
                                              "}");

  m_program->link();
  m_program->bind();

  m_uniformTextureY = m_program->uniformLocation("u_textureY");
  m_uniformTextureCbCr = m_program->uniformLocation("u_textureCbCr");
  m_uniformVerticesRatio = m_program->uniformLocation("u_verticesRatio");
  m_attributeVertices = m_program->attributeLocation("a_position");
  m_attributeUvs = m_program->attributeLocation("a_texCoord");

  m_program->release();
}

/*!
  \internal
  This functions runs on the rendering thread.
 */
void ArKitFrameRenderer::render(float verticesRatioX, float verticesRatioY)
{
  // the texture ids are not valid, do nothing
  if (m_textureY.textureId() == 0 || m_textureCbCr.textureId() == 0)
    return;

  // if the window size is invalid, do nothing
  if (m_size.width() == 0 || m_size.height() == 0)
    return;

  m_program->bind();

  glClear(GL_DEPTH_BUFFER_BIT);
  glDepthMask(GL_FALSE);

  glUniform1i(m_uniformTextureY, 1);
  glUniform1i(m_uniformTextureCbCr, 2);
  glUniform2f(m_uniformVerticesRatio, verticesRatioX, verticesRatioY);

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, m_textureY.textureId());

  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, m_textureCbCr.textureId());

  glEnableVertexAttribArray(m_attributeVertices);

  // get the screen orientation
  const Qt::ScreenOrientations orientation = QGuiApplication::screens().front()->orientation();

  switch (orientation)
  {
    case Qt::PortraitOrientation:
      glVertexAttribPointer(m_attributeVertices, 2, GL_FLOAT, GL_FALSE, 0, kVerticesPortrait);
      break;
    case Qt::LandscapeOrientation:
      glVertexAttribPointer(m_attributeVertices, 2, GL_FLOAT, GL_FALSE, 0, kVerticesRightLandscape);
      break;
    case Qt::InvertedPortraitOrientation:
      glVertexAttribPointer(m_attributeVertices, 2, GL_FLOAT, GL_FALSE, 0, kVerticesReversePortrait);
      break;
    case Qt::InvertedLandscapeOrientation:
      glVertexAttribPointer(m_attributeVertices, 2, GL_FLOAT, GL_FALSE, 0, kVerticesLeftLandscape);
      break;
    default:
      break;
  }

  glEnableVertexAttribArray(m_attributeUvs);
  glVertexAttribPointer(m_attributeUvs, 2, GL_FLOAT, GL_FALSE, 0, kTextures);

  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  glDepthMask(GL_TRUE);
  m_program->release();
}

void ArKitFrameRenderer::setSize(const QSizeF& size)
{
  m_size = size;
}

void ArKitFrameRenderer::updateTextreDataY(int width, int height, const void* data)
{
  if (m_textureY.width() != width || m_textureY.height() != height)
  {
    if (m_textureY.isCreated())
      m_textureY.destroy();

    m_textureY.setSize(width, height);
    m_textureY.setFormat(QOpenGLTexture::R8_UNorm);
    m_textureY.allocateStorage();
  }

  m_textureY.setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, data);
}

void ArKitFrameRenderer::updateTextreDataCbCr(int width, int height, const void* data)
{
  if (m_textureCbCr.width() != width || m_textureCbCr.height() != height)
  {
    if (m_textureCbCr.isCreated())
      m_textureCbCr.destroy();

    m_textureCbCr.setSize(width, height);
    m_textureCbCr.setFormat(QOpenGLTexture::RG8_UNorm);
    m_textureCbCr.allocateStorage();
  }

  m_textureCbCr.setData(QOpenGLTexture::RG, QOpenGLTexture::UInt8, data);
}
