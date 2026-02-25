//
// Copyright 2019 EchoNous Inc.
//

#pragma once

#include <Renderer.h>
#include <CineBuffer.h>

////// FREETYPE related definitions //////
struct font_t {
    unsigned int font_texture;
    float pt;
    float advance[128];
    float width[128];
    float height[128];
    float tex_x1[128];
    float tex_x2[128];
    float tex_y1[128];
    float tex_y2[128];
    float offset_x[128];
    float offset_y[128];
    int initialized;
};

typedef struct font_t font_t;

static inline int
nextp2(int x)
{
    int val = 1;
    while(val < x) val <<= 1;
    return val;
}
//////////////////////////////////////////

class OverlayHRRenderer : public Renderer
{
public: // Construction
    OverlayHRRenderer();
    ~OverlayHRRenderer();

    OverlayHRRenderer(const OverlayHRRenderer& that) = delete;
    OverlayHRRenderer& operator=(OverlayHRRenderer const&) = delete;

public: // Renderer overrides
    ThorStatus  open();
    void        close();
    bool        prepare();
    uint32_t    draw();

public:
    void        setHeartRate(float heartRate);
    void        setTextSize(float textSize);
    void        setAppPath(const char* appPath);

private: // Methods
    font_t*     load_font(const char* path, int pointSize, int dpi);
    void        render_text(float x, float y, float textSize, const char* msg, float* color);

private: // Properties
    GLuint                  mVB[3];

    CineBuffer*             mCineBufferPtr;
    uint32_t                mDataType;

    // FREETYPE handles
    font_t*         mFont;
    GLuint          mFTProgram;
    GLuint          mFTTextureHandle;
    int             mFTTextureUniformHandle;
    int             mFTTextColorUniformHandle;

    float           mHeartRate;
    float           mTextSize;

    // AppPath
    std::string     mAppPath;

    static const char* const    mFTVertexShaderSource;
    static const char* const    mFTFragmentShaderSource;
};
