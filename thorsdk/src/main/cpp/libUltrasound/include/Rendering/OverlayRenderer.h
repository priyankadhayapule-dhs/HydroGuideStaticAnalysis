//
// Copyright 2018 EchoNous Inc.
//

#pragma once

#include <ScanConverterParams.h>
#include <ScanConverterHelper.h>
#include <Renderer.h>

// for handling BMP files
typedef struct {
        unsigned short type;                     /* Magic identifier            */
        unsigned int size;                       /* File size in bytes          */
        unsigned int reserved;
        unsigned int offset;                     /* Offset to image data, bytes */
} BmpHeader;

typedef struct {
        unsigned int size;                 /* Header size in bytes      */
        int width, height;                 /* Width and height of image */
        unsigned short planes;             /* Number of colour planes   */
        unsigned short bits;               /* Bits per pixel            */
        unsigned int compression;          /* Compression type          */
        unsigned int imagesize;            /* Image size in bytes       */
        int xresolution,yresolution;       /* Pixels per meter          */
        unsigned int ncolours;             /* Number of colours         */
        unsigned int importantcolours;     /* Important colours         */
} BmpInfoHeader;

class OverlayRenderer : public Renderer
{
public: // Construction
    OverlayRenderer();
    ~OverlayRenderer();

    OverlayRenderer(const OverlayRenderer& that) = delete;
    OverlayRenderer& operator=(OverlayRenderer const&) = delete;

public: // Renderer overrides
    ThorStatus  open();
    void        close();
    bool        prepare();
    uint32_t    draw();
    void        setScissorUpdate(bool value);


public: // Methods
    void        setTexture(uint8_t* texturePtr);
    // texture image from bitmap object then flipLR
    void        setTextureFlipLR(uint8_t* inputPtr);
    // texture image from bitmap object then flipUD
    void        setTextureFlipUD(uint8_t *inputPtr);
    // texture image from bitmap object for clip
    void        setTextureBmpObj(uint8_t *inputPtr);
    // texture image from bitmap object for clip with Alpha multiplier
    void        setTextureBmpObjAlphaX(uint8_t *inputPtr, int alphaX = 2);

    void        saveImgToBmp(char* filename, uint8_t* imgPtr, int width, int height);     // bmp BGRA
    void        saveImgToBmp24(char* filename, uint8_t* imgPtr, int width, int height);   // bmp BGR

    void        readBmpTexture(char* filename);

private: // Properties
    GLuint                  mProgram;
    GLuint                  mTextureHandle;
    GLint                   mTextureUniformHandle;
    GLuint                  mVB[2];

    uint8_t*                mTexturePtr;
    bool                    mScissorUpdate;

    static const char* const    mVertexShaderSource;
    static const char* const    mFragmentShaderSource;
};
