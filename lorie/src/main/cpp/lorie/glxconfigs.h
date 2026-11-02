/*
 * Replaces the GLX config list provided by Mesa's swrast_dri, which is not shipped with the application.
 * Mirrors dri_fill_in_modes() of Mesa to stay compatible with llvmpipe.
 */

#include <stdint.h>
#include <GL/glxtokens.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof((a)) / sizeof((a)[0]))
#endif

static __GLXconfig *lorieGlxConfigs(void) {
    typedef struct { int8_t bits[4], shifts[4], srgb, drawableType; } ColorFormat;
    typedef struct { int8_t depthBits, stencilBits; } ZsFormat;
    typedef struct { int8_t doubleBuffer, copySwap; } BufferMode;
    typedef struct { int8_t samples, accumModes; } SampleGroup;

    static const ColorFormat formats[] = {
        {{10, 10, 10, 2}, {20, 10, 0, 30}, 0, GLX_WINDOW_BIT | GLX_PIXMAP_BIT | GLX_PBUFFER_BIT},
        {{10, 10, 10, 0}, {20, 10, 0, -1}, 0, GLX_WINDOW_BIT},
        {{10, 10, 10, 2}, {0, 10, 20, 30}, 0, GLX_WINDOW_BIT | GLX_PIXMAP_BIT | GLX_PBUFFER_BIT},
        {{10, 10, 10, 0}, {0, 10, 20, -1}, 0, GLX_WINDOW_BIT},
        {{8, 8, 8, 8}, {16, 8, 0, 24}, 0, GLX_WINDOW_BIT | GLX_PIXMAP_BIT | GLX_PBUFFER_BIT},
        {{8, 8, 8, 0}, {16, 8, 0, -1}, 0, GLX_WINDOW_BIT | GLX_PIXMAP_BIT | GLX_PBUFFER_BIT},
        {{8, 8, 8, 8}, {16, 8, 0, 24}, 1, GLX_WINDOW_BIT | GLX_PIXMAP_BIT | GLX_PBUFFER_BIT},
        {{8, 8, 8, 0}, {16, 8, 0, -1}, 1, GLX_WINDOW_BIT | GLX_PIXMAP_BIT | GLX_PBUFFER_BIT},
        {{5, 6, 5, 0}, {11, 5, 0, -1}, 0, GLX_WINDOW_BIT | GLX_PIXMAP_BIT | GLX_PBUFFER_BIT},
    };
    static const ZsFormat zsFormats[] = {{0, 0}, {16, 0}, {24, 0}, {24, 8}, {32, 0}};
    static const BufferMode bufferModes[] = {{0, 0}, {1, 0}, {1, 1}};
    static const int8_t accumBits[] = {0, 16};
    static const SampleGroup sampleGroups[] = {{0, ARRAY_SIZE(accumBits)}, {4, 1}};
    static const int visualTypes[] = {GLX_TRUE_COLOR, GLX_DIRECT_COLOR, GLX_TRUE_COLOR};
    static __GLXconfig configs[ARRAY_SIZE(visualTypes) * ARRAY_SIZE(formats) * ARRAY_SIZE(sampleGroups) *
                               ARRAY_SIZE(zsFormats) * ARRAY_SIZE(bufferModes) * ARRAY_SIZE(accumBits)];
    static int count;

    if (count)
        return configs;

    for (int pass = 0; pass < ARRAY_SIZE(visualTypes); pass++) {
        for (int f = 0; f < ARRAY_SIZE(formats); f++) {
            const ColorFormat *format = &formats[f];
            int rgbBits = format->bits[0] + format->bits[1] + format->bits[2] + format->bits[3];

            for (int g = 0; g < ARRAY_SIZE(sampleGroups); g++)
                for (int z = 0; z < ARRAY_SIZE(zsFormats); z++)
                    for (int b = 0; b < ARRAY_SIZE(bufferModes); b++)
                        for (int a = 0; a < sampleGroups[g].accumModes; a++) {
                            __GLXconfig *c = &configs[count];
                            int slow = accumBits[a] != 0;
                            int composite = pass == ARRAY_SIZE(visualTypes) - 1;

                            if (composite && (rgbBits != 32 || format->bits[0] != 8 || format->bits[1] != 8 ||
                                              format->bits[2] != 8 || slow || sampleGroups[g].samples))
                                continue;

                            c->duplicatedForComp = composite;
                            c->doubleBufferMode = bufferModes[b].doubleBuffer;
                            c->redBits = format->bits[0];
                            c->greenBits = format->bits[1];
                            c->blueBits = format->bits[2];
                            c->alphaBits = format->bits[3];
                            c->redMask = ((1u << format->bits[0]) - 1) << format->shifts[0];
                            c->greenMask = ((1u << format->bits[1]) - 1) << format->shifts[1];
                            c->blueMask = ((1u << format->bits[2]) - 1) << format->shifts[2];
                            c->alphaMask = format->bits[3] ? ((1u << format->bits[3]) - 1) << format->shifts[3] : 0;
                            c->rgbBits = rgbBits;
                            c->accumRedBits = c->accumGreenBits = c->accumBlueBits = c->accumAlphaBits = accumBits[a];
                            c->depthBits = zsFormats[z].depthBits;
                            c->stencilBits = zsFormats[z].stencilBits;
                            c->visualType = visualTypes[pass];
                            c->visualRating = slow ? GLX_SLOW_CONFIG : GLX_NONE;
                            c->transparentPixel = GLX_NONE;
                            c->transparentRed = c->transparentGreen = c->transparentBlue = c->transparentAlpha = -1;
                            c->sampleBuffers = sampleGroups[g].samples ? 1 : 0;
                            c->samples = sampleGroups[g].samples;
                            c->drawableType = format->drawableType;
                            c->renderType = GLX_RGBA_BIT;
                            c->swapMethod = bufferModes[b].copySwap ? GLX_SWAP_COPY_OML : GLX_SWAP_UNDEFINED_OML;
                            c->bindToTextureRgb = c->bindToTextureRgba = GL_TRUE;
                            c->bindToTextureTargets = GLX_TEXTURE_1D_BIT_EXT | GLX_TEXTURE_2D_BIT_EXT |
                                                      GLX_TEXTURE_RECTANGLE_BIT_EXT;
                            c->yInverted = GL_TRUE;
                            c->sRGBCapable = format->srgb;
                            count++;
                        }
        }
    }

    for (int i = 0; i < count - 1; i++)
        configs[i].next = &configs[i + 1];
    return configs;
}
