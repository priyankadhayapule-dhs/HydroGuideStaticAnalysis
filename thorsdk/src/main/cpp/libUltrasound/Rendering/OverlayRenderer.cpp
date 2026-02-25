//
// Copyright 2018 EchoNous Inc.
//

#define LOG_TAG "OverlayRenderer"

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <GLES3/gl3.h>
#include <ThorUtils.h>
#include <OverlayRenderer.h>

#include <stdio.h>

#define BMP_DEBUG 0

const char* const OverlayRenderer::mVertexShaderSource =
#include "overlay_vertex.glsl"
;

const char* const OverlayRenderer::mFragmentShaderSource =
#include "overlay_fragment.glsl"
;

//-----------------------------------------------------------------------------
OverlayRenderer::OverlayRenderer() :
    Renderer(),
    mProgram(0),
    mTextureHandle(0),
    mTextureUniformHandle(-1)
{
    memset(mVB, 0, sizeof(mVB));
}

//-----------------------------------------------------------------------------
OverlayRenderer::~OverlayRenderer()
{
    close();
}

//-----------------------------------------------------------------------------
ThorStatus OverlayRenderer::open()
{
    ThorStatus      retVal = THOR_ERROR;

    float x_start = -1.0;
    float x_end   = 1.0;

    float y_start = -1.0;
    float y_end   = 1.0;

    float u_x = 0.5f/getWidth();
    float u_y = 0.5f/getHeight();

    float p0_x = x_start;
    float p0_y = y_end;

    float p1_x = x_end;
    float p1_y = y_end;

    float p2_x = x_start;
    float p2_y = y_start;

    float p3_x = x_end;
    float p3_y = y_start;

    // buffer location and binding
    float locData[16];     //4*4
    short idxData[4];

    mProgram = createProgram(mVertexShaderSource, mFragmentShaderSource);
    if (!checkGlError("createProgram")) goto err_ret;

    glGenTextures(1, &mTextureHandle);
    if (!checkGlError("glGenTextures")) goto err_ret;

    glBindTexture(GL_TEXTURE_2D, mTextureHandle);
    if (!checkGlError("glBindTexture")) goto err_ret;

    // texture parameter
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    if (!checkGlError("glTexParameteri")) goto err_ret;

    mTextureUniformHandle = glGetUniformLocation(mProgram, "u_MTexture");   // it's MTexture as we reuse M-mode glsl
    if (!checkGlError("glGetUniformLocation")) goto err_ret;

    // texture init
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, mTextureHandle);
    if (!checkGlError("BindOverlayTexture")) goto err_ret;

    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA,
                 getHeight(),
                 getWidth(),
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 0);

    if (!checkGlError("glTexImage2D-Overlay-Initialize")) goto err_ret;

    glGenBuffers(2, mVB);

    locData[0] = p0_x;
    locData[1] = p0_y;
    locData[2] = u_x;
    locData[3] = u_y;

    locData[4] = p1_x;
    locData[5] = p1_y;
    locData[6] = u_x;
    locData[7] = 1.0f - u_y;

    locData[8]  = p2_x;
    locData[9]  = p2_y;
    locData[10] = 1.0f - u_x;
    locData[11] = u_y;

    locData[12] = p3_x;
    locData[13] = p3_y;
    locData[14] = 1.0f - u_x;
    locData[15] = 1.0f - u_y;

    glBindBuffer(GL_ARRAY_BUFFER, mVB[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 16, locData, GL_STATIC_DRAW);

    idxData[0] = 0;
    idxData[1] = 1;
    idxData[2] = 2;
    idxData[3] = 3;

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVB[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(short) * 4, idxData, GL_STATIC_DRAW);

    glVertexAttribPointer(6, 4, GL_FLOAT, false, sizeof(float) * 4, 0);
    glEnableVertexAttribArray(6);

    // default scissor update = true (fullscreen update)
    mScissorUpdate = true;

    retVal = THOR_OK;

err_ret:
    return(retVal);
}

//-----------------------------------------------------------------------------
void OverlayRenderer::close()
{
    if (0 != mTextureHandle)
    {
        glDeleteTextures(1, &mTextureHandle);
        mTextureHandle = 0;
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
bool OverlayRenderer::prepare()
{
    // set glScissor
    if (mScissorUpdate)
    {
        glScissor(getX(), getY(), getWidth(), getHeight());
    }

    return(true);
}

//-----------------------------------------------------------------------------
void OverlayRenderer::setScissorUpdate(bool value)
{
    mScissorUpdate = value;
}

//-----------------------------------------------------------------------------
uint32_t OverlayRenderer::draw()
{
    glUseProgram(mProgram);
    checkGlError("glUseProgram");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, mTextureHandle);

    glUniform1i(mTextureUniformHandle, 4);

    glBindBuffer(GL_ARRAY_BUFFER, mVB[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mVB[1]);
    glDrawElements(GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_SHORT, 0);
    checkGlError("glDrawArrays");

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, 0);

    glDisable(GL_BLEND);

    // set scissor box to 0, 0
    glScissor(getX(), getY(), 0, 0);

done:
    return(0);
}

//-----------------------------------------------------------------------------
void OverlayRenderer::setTexture(uint8_t* texturePtr)
{
    //set texture
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, mTextureHandle);

    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA,
                 getHeight(),
                 getWidth(),
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 texturePtr);

    checkGlError("setTexture");
}

//-----------------------------------------------------------------------------
void OverlayRenderer::setTextureBmpObj(uint8_t* inputTex)
{
    // RGBA texture image
    int imgWidth = getWidth();
    int imgHeight = getHeight();
    int bytesPerPix = 4;
    int idx, texI, texJ, texIdx;

    u_char *texImg = new u_char[imgWidth * imgHeight * bytesPerPix];

    u_char r, g, b, a;

    // converting to texImg
    for (int j = 0; j < imgHeight; j++)
    {
        for (int i = 0; i < imgWidth; i++)
        {
            // read image
            idx = imgWidth * j + i;
            r = inputTex[bytesPerPix * idx];
            g = inputTex[bytesPerPix * idx + 1];
            b = inputTex[bytesPerPix * idx + 2];
            a = inputTex[bytesPerPix * idx + 3];

            // i, j no flip
            texI = i;
            texJ = j;

            // store image
            texIdx = imgHeight * texI + texJ;
            texImg[bytesPerPix * texIdx] = r;
            texImg[bytesPerPix * texIdx + 1] = g;
            texImg[bytesPerPix * texIdx + 2] = b;
            texImg[bytesPerPix * texIdx + 3] = a;
        }
    }

    // setting Texture
    setTexture(texImg);

    delete[] texImg;
}

//-----------------------------------------------------------------------------
void OverlayRenderer::setTextureBmpObjAlphaX(uint8_t* inputTex, int alphaX)
{
    // RGBA texture image
    int imgWidth = getWidth();
    int imgHeight = getHeight();
    int bytesPerPix = 4;
    int idx, texI, texJ, texIdx;

    u_char *texImg = new u_char[imgWidth * imgHeight * bytesPerPix];

    u_char r, g, b, a;
    int alpha;

    // converting to texImg
    for (int j = 0; j < imgHeight; j++)
    {
        for (int i = 0; i < imgWidth; i++)
        {
            // read image
            idx = imgWidth * j + i;
            r = inputTex[bytesPerPix * idx];
            g = inputTex[bytesPerPix * idx + 1];
            b = inputTex[bytesPerPix * idx + 2];
            a = inputTex[bytesPerPix * idx + 3];

            alpha = a * alphaX;
            if (alpha > 255)
                alpha = 255;
            a = (u_char) alpha;

            // i, j no flip
            texI = i;
            texJ = j;

            // store image
            texIdx = imgHeight * texI + texJ;
            texImg[bytesPerPix * texIdx] = r;
            texImg[bytesPerPix * texIdx + 1] = g;
            texImg[bytesPerPix * texIdx + 2] = b;
            texImg[bytesPerPix * texIdx + 3] = a;
        }
    }

    // setting Texture
    setTexture(texImg);

    delete[] texImg;
}

//-----------------------------------------------------------------------------
void OverlayRenderer::setTextureFlipUD(uint8_t *inputTex)
{
    // RGBA texture image
    int imgWidth = getWidth();
    int imgHeight = getHeight();
    int bytesPerPix = 4;
    int idx, texI, texJ, texIdx;

    u_char *texImg = new u_char[imgWidth * imgHeight * bytesPerPix];

    u_char r, g, b, a;

    // converting to texImg (flipUD)
    for (int j = 0; j < imgHeight; j++)
    {
        for (int i = 0; i < imgWidth; i++)
        {
            // read image
            idx = imgWidth * j + i;
            r = inputTex[bytesPerPix * idx];
            g = inputTex[bytesPerPix * idx + 1];
            b = inputTex[bytesPerPix * idx + 2];
            a = inputTex[bytesPerPix * idx + 3];

            // flip UD
            texI = i;
            texJ = imgHeight - j - 1;

            // store image
            texIdx = imgHeight * texI + texJ;
            texImg[bytesPerPix * texIdx] = r;
            texImg[bytesPerPix * texIdx + 1] = g;
            texImg[bytesPerPix * texIdx + 2] = b;
            texImg[bytesPerPix * texIdx + 3] = a;
        }
    }

    // setting Texture
    setTexture(texImg);

    delete[] texImg;
}

//-----------------------------------------------------------------------------
void OverlayRenderer::setTextureFlipLR(uint8_t* inputTex)
{
    // RGBA texture image
    int imgWidth = getWidth();
    int imgHeight = getHeight();
    int bytesPerPix = 4;
    int idx, texI, texJ, texIdx;

    u_char* texImg = new u_char[imgWidth * imgHeight * bytesPerPix];

    u_char r, g, b, a;

    // converting to texImg (flipLR)
    for (int j = 0; j < imgHeight; j++) {
        for (int i = 0; i < imgWidth; i++) {
            // read image
            idx = imgWidth * j + i;
            r = inputTex[bytesPerPix * idx];
            g = inputTex[bytesPerPix * idx + 1];
            b = inputTex[bytesPerPix * idx + 2];
            a = inputTex[bytesPerPix * idx + 3];

            // flip LR
            texI = imgWidth - i - 1;
            texJ = j;

            // store image
            texIdx = imgHeight * texI + texJ;
            texImg[bytesPerPix * texIdx] = r;
            texImg[bytesPerPix * texIdx + 1] = g;
            texImg[bytesPerPix * texIdx + 2] = b;
            texImg[bytesPerPix * texIdx + 3] = a;
        }
    }

    // setting Texture
    setTexture(texImg);

    delete [] texImg;
}

//-----------------------------------------------------------------------------
void OverlayRenderer::saveImgToBmp(char* filename, uint8_t* imgPtr, int width, int height)
{
    // BMP in BGRA format.  Assumes imgWidth -> multiple of 4s
    FILE *f;

    int extrabytes = (4 - ((width * 4) % 4))%4;
    int filesize = 54 + ((width * 4) + extrabytes) * height;

    unsigned char bmpfileheader[14] = {'B','M', 0,0,0,0, 0,0, 0,0, 54,0,0,0};
    unsigned char bmpinfoheader[40] = {40,0,0,0, 0,0,0,0, 0,0,0,0, 1,0, 32,0};
    unsigned char bmppad[4] = {0,0,0,0};

    bmpfileheader[ 2] = (unsigned char)(filesize    );
    bmpfileheader[ 3] = (unsigned char)(filesize>> 8);
    bmpfileheader[ 4] = (unsigned char)(filesize>>16);
    bmpfileheader[ 5] = (unsigned char)(filesize>>24);

    bmpinfoheader[ 4] = (unsigned char)(       width    );
    bmpinfoheader[ 5] = (unsigned char)(       width>> 8);
    bmpinfoheader[ 6] = (unsigned char)(       width>>16);
    bmpinfoheader[ 7] = (unsigned char)(       width>>24);
    bmpinfoheader[ 8] = (unsigned char)(       height    );
    bmpinfoheader[ 9] = (unsigned char)(       height>> 8);
    bmpinfoheader[10] = (unsigned char)(       height>>16);
    bmpinfoheader[11] = (unsigned char)(       height>>24);

    f = fopen(filename,"wb");

    fwrite(bmpfileheader,1,14,f);
    fwrite(bmpinfoheader,1,40,f);

    fwrite(imgPtr,4,width*height,f);

    fclose(f);
}

void OverlayRenderer::saveImgToBmp24(char* filename, uint8_t* imgPtr, int width, int height)
{
    //BMP in BGR format.  Assumes imgWidth -> multiple of 4s
    FILE *f;

    int extrabytes = (4 - ((width * 3) % 4))%4;
    int filesize = 54 + ((width * 3) + extrabytes) * height;

    unsigned char bmpfileheader[14] = {'B','M', 0,0,0,0, 0,0, 0,0, 54,0,0,0};
    unsigned char bmpinfoheader[40] = {40,0,0,0, 0,0,0,0, 0,0,0,0, 1,0, 24,0};
    unsigned char bmppad[4] = {0,0,0,0};

    bmpfileheader[ 2] = (unsigned char)(filesize    );
    bmpfileheader[ 3] = (unsigned char)(filesize>> 8);
    bmpfileheader[ 4] = (unsigned char)(filesize>>16);
    bmpfileheader[ 5] = (unsigned char)(filesize>>24);

    bmpinfoheader[ 4] = (unsigned char)(       width    );
    bmpinfoheader[ 5] = (unsigned char)(       width>> 8);
    bmpinfoheader[ 6] = (unsigned char)(       width>>16);
    bmpinfoheader[ 7] = (unsigned char)(       width>>24);
    bmpinfoheader[ 8] = (unsigned char)(       height    );
    bmpinfoheader[ 9] = (unsigned char)(       height>> 8);
    bmpinfoheader[10] = (unsigned char)(       height>>16);
    bmpinfoheader[11] = (unsigned char)(       height>>24);

    f = fopen(filename,"wb");

    fwrite(bmpfileheader,1,14,f);
    fwrite(bmpinfoheader,1,40,f);

    fwrite(imgPtr,3,width*height,f);

    fclose(f);
}

//-----------------------------------------------------------------------------
void OverlayRenderer::readBmpTexture(char* filename)
{
    FILE *fptr;
    BmpHeader header;
    BmpInfoHeader infoheader;
    ssize_t rd;
    int bytesPerPix, imgWidth, imgHeight;
    u_char* img;
    u_char* texImg;
    int idx, texIdx;
    u_char r,g,b,a;
    int texHeight, texI, texJ;

    if((fptr = fopen(filename,"r")) == NULL)
    {
        ALOGE("Unable to open BMP file \"%s\"\n",filename);
        goto error_;
    }
    /* Read and check BMP header */
    if(fread(&header.type, 2, 1, fptr) != 1)
    {
        ALOGE("Failed to read BMP header");
        goto error_;
    }

    if(header.type != 'M'*256+'B'){
        ALOGE("File is not BMP type");
        goto error_;
    }

    do
    {
        if((rd = fread(&header.size, 4, 1, fptr)) != 1) break;
        if((rd = fread(&header.reserved, 4, 1, fptr)) != 1) break;
        if((rd = fread(&header.offset, 4, 1, fptr)) != 1) break;
    }while(0);

    if(rd != 1){
        ALOGE("Error reading file");
        goto error_;
    }
    /* Read and check the information header */
    if (fread(&infoheader, sizeof(BmpInfoHeader), 1, fptr) != 1) {
        ALOGE("Failed to read BMP info header");
        goto error_;
    }

    imgWidth = infoheader.width;
    imgHeight = infoheader.height;
    bytesPerPix = infoheader.bits/8;

#if BMP_DEBUG
     ALOGD("File size: %d bytes", header.size);
     ALOGD("Offset to image data is %d bytes", header.offset);
     ALOGD("Image size = %d x %d", infoheader.width, infoheader.height);
     ALOGD("Number of colour planes is %d", infoheader.planes);
     ALOGD("Bits per pixel is %d", infoheader.bits);
     ALOGD("Compression type is %d", infoheader.compression);
     ALOGD("Number of colours is %d", infoheader.ncolours);
     ALOGD("Number of required colours is %d", infoheader.importantcolours);
     ALOGD("Bytes per pixel is %d", bytesPerPix);
#endif
    /* Seek to the start of the image data */
    fseek(fptr, header.offset, SEEK_SET);

    if (bytesPerPix != 4)
    {
        ALOGE("Only supports BGRA formats.");
        goto error_;
    }

    img = new u_char[imgWidth * imgHeight * bytesPerPix];
    texImg = new u_char[imgWidth * imgHeight * bytesPerPix];

    // read image
    fread(img, sizeof(u_char), imgWidth * imgHeight * bytesPerPix, fptr);

    texHeight = getHeight();

    // converting to texImg (rotate image)
    for (int j = 0; j < imgHeight; j++) {
        for (int i = 0; i < imgWidth; i++) {
            // read image
            idx = imgWidth * j + i;
            b = img[bytesPerPix * idx];
            g = img[bytesPerPix * idx + 1];
            r = img[bytesPerPix * idx + 2];
            a = img[bytesPerPix * idx + 3];

            texI = imgWidth - i - 1;
            texJ = imgHeight - j - 1;

            // store image
            if (texJ < texHeight)
            {
                texIdx = texHeight * texI + texJ;
                texImg[bytesPerPix * texIdx] = r;
                texImg[bytesPerPix * texIdx + 1] = g;
                texImg[bytesPerPix * texIdx + 2] = b;
                texImg[bytesPerPix * texIdx + 3] = a;
            }

        }
    }

    // setting texture image
    setTexture(texImg);

    delete [] img;
    delete [] texImg;

error_:
    if (fptr)
        fclose(fptr);

    return;
}
