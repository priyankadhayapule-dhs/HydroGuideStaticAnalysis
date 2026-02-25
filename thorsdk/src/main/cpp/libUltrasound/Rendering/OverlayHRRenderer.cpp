//
// Copyright 2018 EchoNous Inc.
//

#define LOG_TAG "OverlayHRRenderer"

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <GLES3/gl3.h>
#include <ThorUtils.h>
#include <OverlayHRRenderer.h>
#include <UltrasoundManager.h>

#include <ft2build.h>
#include FT_FREETYPE_H

const char* const OverlayHRRenderer::mFTVertexShaderSource =
#include "ft_vertex.glsl"
;

const char* const OverlayHRRenderer::mFTFragmentShaderSource =
#include "ft_fragment.glsl"
;

//-----------------------------------------------------------------------------
OverlayHRRenderer::OverlayHRRenderer() :
    Renderer(),
    mCineBufferPtr(nullptr),
    mDataType(0),
    mFont(nullptr),
    mFTProgram(0),
    mHeartRate(0.0f),
    mTextSize(29.0f)
{
    memset(mVB, 0, sizeof(mVB));
}

//-----------------------------------------------------------------------------
OverlayHRRenderer::~OverlayHRRenderer()
{
    close();
}

//-----------------------------------------------------------------------------
ThorStatus OverlayHRRenderer::open()
{
    ThorStatus      retVal = THOR_ERROR;
    std::string     fontPath;

    glGenBuffers(2, mVB);

    // freetype program
    mFTProgram = createProgram(mFTVertexShaderSource, mFTFragmentShaderSource);
    if (!checkGlError("createProgram-FreeType")) goto err_ret;

    // load fonts
    fontPath = mAppPath + "/" + DEFAULT_FONT_FILE_NAME;
    mFont = load_font(fontPath.c_str(), 48, 10);

    mFTTextureUniformHandle = glGetUniformLocation(mFTProgram, "u_text");
    mFTTextColorUniformHandle = glGetUniformLocation(mFTProgram, "u_textColor");
    if (!checkGlError("glGetUniformLocation-FreeType")) goto err_ret;

    glBindBuffer(GL_ARRAY_BUFFER, mVB[0]);
    glVertexAttribPointer(5, 4, GL_FLOAT, false, sizeof(float) * 4, 0);
    glEnableVertexAttribArray(5);

    retVal = THOR_OK;

err_ret:
    return(retVal);
}

//-----------------------------------------------------------------------------
void OverlayHRRenderer::close()
{
    if (0 != mFTTextureHandle)
    {
        glDeleteTextures(1, &mFTTextureHandle);
        mFTTextureHandle = 0;
    }
    if (0 != mVB[0])
    {
        glDeleteBuffers(1, &mVB[0]);
        mVB[0] = 0;
    }
    if (0 != mVB[1])
    {
        glDeleteBuffers(1, &mVB[1]);
        mVB[1] = 0;
    }
}

//-----------------------------------------------------------------------------
void OverlayHRRenderer::setHeartRate(float heartRate)
{
    mHeartRate = heartRate;
}

//-----------------------------------------------------------------------------
void OverlayHRRenderer::setTextSize(float textSize)
{
    mTextSize = textSize;
}

//-----------------------------------------------------------------------------
void OverlayHRRenderer::setAppPath(const char* appPath)
{
    mAppPath = appPath;
}

//-----------------------------------------------------------------------------
bool OverlayHRRenderer::prepare()
{
    glScissor(getX(), getY(), getWidth(), getHeight());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    return(true);
}

//-----------------------------------------------------------------------------
uint32_t OverlayHRRenderer::draw()
{
    //                      R     G     B
    float   colorText[3] = {0.961f, 0.651f, 0.137f};  // White
    float   x;
    float   y;
    std::string heartRateStr;

    x = 0.0f;
    y = -getHeight()/2.0f * (mTextSize/45.0f);  // adjust y offset


    if (mHeartRate < 0.0f)
    {
        heartRateStr = " ---";
    }
    else
    {
        heartRateStr = std::to_string((int) mHeartRate);

        if (mHeartRate < 100.0f)
            heartRateStr = " " + heartRateStr;
    }

    render_text(x, y, mTextSize, heartRateStr.c_str(), colorText);

err_ret:
    return(0);
}


//-----------------------------------------------------------------------------
void OverlayHRRenderer::render_text(float x, float y, float textSize, const char* msg, float* tColor)
{
    // x, y in pixels (from top left corner); textSize: desired size in Pixels

    unsigned i, c;

    float* vertices = new float[strlen(msg) * 16];      //vertice / texture combined.
    short* indices = new short[strlen(msg) * 6];

    float pen_x = 0.0f;

    float textScaleX, textScaleY;
    float textMulti;
    float nx, ny;

    if (nullptr == mFont)
    {
        ALOGE("Font is not loaded");
        return; 
    }

    font_t *font = mFont;

    textMulti = textSize/33.0f;

    textScaleX = 1.0f/getWidth() * textMulti;
    textScaleY = 1.0f/getHeight() * textMulti;

    nx = (2.0f * x) / getWidth() - 1.0f;
    ny = 2.0f * (getHeight() - (y + textSize))/getHeight() - 1.0f;

    for(i = 0; i < strlen(msg); ++i) {
        c = msg[i];

        vertices[16 * i + 0] = nx + (pen_x + font->offset_x[c]) * textScaleX;
        vertices[16 * i + 1] = ny + (font->offset_y[c]) * textScaleY;
        vertices[16 * i + 4] = vertices[16 * i + 0] + (font->width[c] * textScaleX);
        vertices[16 * i + 5] = vertices[16 * i + 1];
        vertices[16 * i + 8] = vertices[16 * i + 0];
        vertices[16 * i + 9] = vertices[16 * i + 1] + (font->height[c] * textScaleY);
        vertices[16 * i + 12] = vertices[16 * i + 4];
        vertices[16 * i + 13] = vertices[16 * i + 9];

        vertices[16 * i + 2] = font->tex_x1[c];
        vertices[16 * i + 3] = font->tex_y2[c];
        vertices[16 * i + 6] = font->tex_x2[c];
        vertices[16 * i + 7] = font->tex_y2[c];
        vertices[16 * i + 10] = font->tex_x1[c];
        vertices[16 * i + 11] = font->tex_y1[c];
        vertices[16 * i + 14] = font->tex_x2[c];
        vertices[16 * i + 15] = font->tex_y1[c];

        indices[i * 6 + 0] = 4 * i + 0;
        indices[i * 6 + 1] = 4 * i + 1;
        indices[i * 6 + 2] = 4 * i + 2;
        indices[i * 6 + 3] = 4 * i + 2;
        indices[i * 6 + 4] = 4 * i + 1;
        indices[i * 6 + 5] = 4 * i + 3;

        pen_x += font->advance[c];
    }

    // use program
    glUseProgram(mFTProgram);
    checkGlError("glUseProgram-FT");

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, font->font_texture);   //mFTTextureHandle

    // texture unit 4
    glUniform1i(mFTTextureUniformHandle, 4);
    glUniform3fv(mFTTextColorUniformHandle, 1, tColor);

    glBindBuffer(GL_ARRAY_BUFFER, mVB[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * strlen(msg) * 16, vertices, GL_DYNAMIC_DRAW);
    checkGlError("BindArrayBuffer");

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVB[1]); // index
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 sizeof(short) * 6 * strlen(msg),
                 indices,
                 GL_DYNAMIC_DRAW);
    checkGlError("BindElementBuffer");

    glDrawElements(GL_TRIANGLES, strlen(msg) * 6, GL_UNSIGNED_SHORT, 0);
    checkGlError("drawElement-FT");

    glBindTexture(GL_TEXTURE_2D, 0);

    delete [] vertices;
    delete [] indices;

    return;
}

//-----------------------------------------------------------------------------
font_t* OverlayHRRenderer::load_font(const char* path, int pointSize, int dpi)
{
    UNUSED(dpi);

    font_t* font;
    FT_Library library;
    FT_Face face;
    unsigned c;
    unsigned i, j;

    if (!path){
        ALOGE("Invalid path to font file\n");
        return NULL;
    }

    if(FT_Init_FreeType(&library)) {
        ALOGE("Error loading Freetype library\n");
        return NULL;
    }

    if (FT_New_Face(library, path, 0, &face)) {
        ALOGE("Error loading font %s\n", path);
        return NULL;
    }

    if (FT_Set_Pixel_Sizes(face, 0, pointSize) ) {
        ALOGE("Error initializing character parameters\n");
        return NULL;
    }

    font = (font_t*) malloc(sizeof(font_t));
    font->initialized = 0;

    glGenTextures(1, &(font->font_texture));
    mFTTextureHandle = font->font_texture;

    checkGlError("generate FT texture");

    // Let each glyph reside in 32x32 section of the font texture
    unsigned segment_size_x = 0, segment_size_y = 0;
    unsigned num_segments_x = 16;
    unsigned num_segments_y = 8;

    FT_GlyphSlot slot;
    FT_Bitmap bmp;
    unsigned glyph_width, glyph_height;

    // First calculate the max width and height of a character in a passed font
    for(c = 0; c < 128; c++) {
        if(FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            ALOGE("FT_Load_Char failed\n");
            free(font);
            return NULL;
        }

        slot = face->glyph;
        bmp = slot->bitmap;

        glyph_width = bmp.width;
        glyph_height = bmp.rows;

        if (glyph_width > segment_size_x) {
            segment_size_x = glyph_width;
        }

        if (glyph_height > segment_size_y) {
            segment_size_y = glyph_height;
        }
    }

    int font_tex_width = nextp2(num_segments_x * segment_size_x);
    int font_tex_height = nextp2(num_segments_y * segment_size_y);

    int bitmap_offset_x = 0, bitmap_offset_y = 0;

    GLubyte* font_texture_data = (GLubyte*) malloc(sizeof(GLubyte) * font_tex_width * font_tex_height);
    memset((void*)font_texture_data, 0, sizeof(GLubyte) * font_tex_width * font_tex_height);

    if (!font_texture_data) {
        ALOGE("Failed to allocate memory for font texture\n");
        free(font);
        return NULL;
    }


    // Fill font texture bitmap with individual bmp data and record appropriate size,
    //  texture coordinates and offsets for every glyph
    for(c = 0; c < 128; c++) {
        if(FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            ALOGE("FT_Load_Char failed\n");
            free(font);
            return NULL;
        }

        slot = face->glyph;
        bmp = slot->bitmap;

        glyph_width = nextp2(bmp.width);
        glyph_height = nextp2(bmp.rows);

        div_t temp = div((int) c, (int) num_segments_x);

        bitmap_offset_x = segment_size_x * temp.rem;
        bitmap_offset_y = segment_size_y * temp.quot;

        for (j = 0; j < glyph_height; j++) {
            for (i = 0; i < glyph_width; i++) {
                font_texture_data[((bitmap_offset_x + i) + (j + bitmap_offset_y) * font_tex_width)] =
                        (i >= bmp.width || j >= bmp.rows)? 0 : bmp.buffer[i + bmp.width * j];
            }
        }

        font->advance[c] = (float)(slot->advance.x >> 6);
        font->tex_x1[c] = (float)bitmap_offset_x / (float) font_tex_width;
        font->tex_x2[c] = (float)(bitmap_offset_x + bmp.width) / (float)font_tex_width;
        font->tex_y1[c] = (float)bitmap_offset_y / (float) font_tex_height;
        font->tex_y2[c] = (float)(bitmap_offset_y + bmp.rows) / (float)font_tex_height;
        font->width[c] = bmp.width;
        font->height[c] = bmp.rows;
        font->offset_x[c] = (float)slot->bitmap_left;
        font->offset_y[c] =  (float)((slot->metrics.horiBearingY-face->glyph->metrics.height) >> 6);
    }

    // TODO: update with proper error handling
    checkGlError("FreeType - texture data generation");

    // active texture unit 4.
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, font->font_texture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, font_tex_width, font_tex_height, 0, GL_LUMINANCE , GL_UNSIGNED_BYTE, font_texture_data);

    checkGlError("FreeType - texture data loading");

    free(font_texture_data);

    FT_Done_Face(face);
    FT_Done_FreeType(library);

    font->initialized = 1;
    ALOGD("Font has been initialized");
    ALOGD("SegmentSize - x: %d, y: %d", segment_size_x, segment_size_y);

    return font;
}
