#ifndef RAQM_RAYLIB_H
#define RAQM_RAYLIB_H

#include <raylib.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <raqm.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <rlgl.h>

// -------------------- Unicode Block Definition --------------------
// typedef struct
// {
//     int start;
//     int end;
// } RaqmUnicodeBlock;

// #define RAQM_BLOCK_ASCII (RaqmUnicodeBlock){0x0020, 0x007E}
// #define RAQM_BLOCK_KANNADA (RaqmUnicodeBlock){0x0C80, 0x0CFF}
// #define RAQM_BLOCK_LATIN1 (RaqmUnicodeBlock){0x00A0, 0x00FF}
// #define RAQM_BLOCK_DEVANAGARI (RaqmUnicodeBlock){0x0900, 0x097F}

// -------------------- Glyph Info & Font Struct --------------------
typedef struct
{
    Rectangle source;
    int bitmap_left;
    int bitmap_top;
    int advance;
} RaqmRayCachedGlyph;

#define RAQM_GLYPH_CACHE_SIZE 65536

typedef struct
{
    FT_Library ft;
    FT_Face face;
    int fontSize;
    int ascender;
    Texture2D atlas;
    RaqmRayCachedGlyph *glyphCache[RAQM_GLYPH_CACHE_SIZE];
    RaqmRayCachedGlyph *placeholderGlyph;
} RaqmRayFont;

// -------------------- Text Object --------------------
typedef struct
{
    RaqmRayCachedGlyph **glyphs;
    size_t glyphCount;
    raqm_t *rq;
    int *x_offsets;
    int *y_offsets;
} RaqmRayText;

// -------------------- Init & Cleanup --------------------
static inline int RaqmRayFont_Load(RaqmRayFont *font, const char *fontFile, int baseSize)
{
    memset(font, 0, sizeof(*font));
    font->fontSize = baseSize;

    if (FT_Init_FreeType(&font->ft))
    {
        TraceLog(LOG_WARNING, "RAQM: Failed to initialize FreeType");
        return 0;
    }
    if (FT_New_Face(font->ft, fontFile, 0, &font->face))
    {
        TraceLog(LOG_WARNING, "RAQM: Failed to load font face from %s", fontFile);
        FT_Done_FreeType(font->ft);
        return 0;
    }
    FT_Set_Pixel_Sizes(font->face, 0, baseSize);

    font->ascender = (int)((font->face->ascender * 1.0 / font->face->units_per_EM) * baseSize);

    // --- Atlas Generation ---
    int *glyphIndexMap = (int *)calloc(RAQM_GLYPH_CACHE_SIZE, sizeof(int));
    int uniqueGlyphCount = 0;

    unsigned int placeholderIndex = FT_Get_Char_Index(font->face, '?');
    if (placeholderIndex != 0)
    {
        glyphIndexMap[placeholderIndex] = 1;
        uniqueGlyphCount++;
    }

    for (int i = 0; i < font->face->num_glyphs; i++)
    {
        unsigned int glyph_index = i;
        if (glyph_index == 0 || glyphIndexMap[glyph_index] == 1)
            continue;

        glyphIndexMap[glyph_index] = 1;
        uniqueGlyphCount++;
    }

    if (uniqueGlyphCount == 0)
    {
        TraceLog(LOG_WARNING, "RAQM: No unique glyphs found to generate atlas.");
        free(glyphIndexMap);
        FT_Done_Face(font->face);
        FT_Done_FreeType(font->ft);
        return 0;
    }

    GlyphInfo *glyphInfos = (GlyphInfo *)RL_CALLOC(uniqueGlyphCount, sizeof(GlyphInfo));
    int *uniqueGlyphIndices = (int *)RL_CALLOC(uniqueGlyphCount, sizeof(int));
    int currentGlyph = 0;

    for (int i = 0; i < uniqueGlyphCount; i++)
    {
        if (glyphIndexMap[i] == 1)
        {
            if (FT_Load_Glyph(font->face, i, FT_LOAD_RENDER))
                continue;

            FT_GlyphSlot g = font->face->glyph;
            int w = g->bitmap.width;
            int h = g->bitmap.rows;

            Image glyphImage = {0};
            if (w > 0 && h > 0)
            {
                glyphImage.data = (unsigned char *)RL_MALLOC(w * h);
                memcpy(glyphImage.data, g->bitmap.buffer, w * h);
                glyphImage.width = w;
                glyphImage.height = h;
                glyphImage.format = PIXELFORMAT_UNCOMPRESSED_GRAYSCALE;
                glyphImage.mipmaps = 1;
            }

            glyphInfos[currentGlyph].value = i;
            glyphInfos[currentGlyph].image = glyphImage;
            glyphInfos[currentGlyph].offsetX = g->bitmap_left;
            glyphInfos[currentGlyph].offsetY = g->bitmap_top;
            glyphInfos[currentGlyph].advanceX = g->advance.x >> 6;

            uniqueGlyphIndices[currentGlyph] = i;
            currentGlyph++;
        }
    }

    Rectangle *glyphRecs = NULL;
    int atlasPadding = 1;

    Image atlasImage = GenImageFontAtlas(glyphInfos, &glyphRecs, uniqueGlyphCount, baseSize, atlasPadding, 0);

    font->atlas = LoadTextureFromImage(atlasImage);
    SetTextureFilter(font->atlas, TEXTURE_FILTER_BILINEAR);
    UnloadImage(atlasImage);

    for (int i = 0; i < uniqueGlyphCount; i++)
    {
        RaqmRayCachedGlyph *stored = (RaqmRayCachedGlyph *)malloc(sizeof(RaqmRayCachedGlyph));
        stored->source = glyphRecs[i];
        stored->bitmap_left = glyphInfos[i].offsetX;
        stored->bitmap_top = glyphInfos[i].offsetY;
        stored->advance = glyphInfos[i].advanceX;

        unsigned int glyphIndex = uniqueGlyphIndices[i];
        font->glyphCache[glyphIndex] = stored;

        RL_FREE(glyphInfos[i].image.data);
    }

    if (placeholderIndex != 0)
    {
        font->placeholderGlyph = font->glyphCache[placeholderIndex];
    }

    RL_FREE(glyphRecs);
    RL_FREE(glyphInfos);
    RL_FREE(uniqueGlyphIndices);
    free(glyphIndexMap);

    TraceLog(LOG_INFO, "RAQM: Font loaded successfully with texture atlas (%d unique glyphs)", uniqueGlyphCount);
    return 1;
}

static inline void RaqmRayFont_Unload(RaqmRayFont *font)
{
    if (font->atlas.id > 0)
    {
        UnloadTexture(font->atlas);
    }

    for (int i = 0; i < RAQM_GLYPH_CACHE_SIZE; i++)
    {
        if (font->glyphCache[i])
        {
            free(font->glyphCache[i]);
        }
    }

    if (font->face)
        FT_Done_Face(font->face);
    if (font->ft)
        FT_Done_FreeType(font->ft);
    memset(font, 0, sizeof(*font));
}

// -------------------- Text Shaping & Drawing --------------------
static inline int RaqmRayText_Set(RaqmRayText *text, RaqmRayFont *font, const char *str)
{
    if (text->glyphs)
        free(text->glyphs);
    if (text->x_offsets)
        free(text->x_offsets);
    if (text->y_offsets)
        free(text->y_offsets);
    if (text->rq)
        raqm_destroy(text->rq);

    text->rq = raqm_create();
    raqm_set_text_utf8(text->rq, str, (int)strlen(str));
    raqm_set_freetype_face(text->rq, font->face);
    raqm_set_par_direction(text->rq, RAQM_DIRECTION_DEFAULT);

    if (!raqm_layout(text->rq))
        return 0;

    size_t count;
    raqm_glyph_t *glyphs = raqm_get_glyphs(text->rq, &count);
    text->glyphCount = count;
    text->glyphs = (RaqmRayCachedGlyph **)malloc(sizeof(RaqmRayCachedGlyph *) * count);
    text->x_offsets = (int *)malloc(sizeof(int) * count);
    text->y_offsets = (int *)malloc(sizeof(int) * count);

    for (size_t i = 0; i < count; i++)
    {
        unsigned int glyph_index = glyphs[i].index;
        RaqmRayCachedGlyph *cached = (glyph_index < RAQM_GLYPH_CACHE_SIZE) ? font->glyphCache[glyph_index] : NULL;

        if (!cached && font->placeholderGlyph)
        {
            cached = font->placeholderGlyph;
        }

        text->glyphs[i] = cached;
        text->x_offsets[i] = glyphs[i].x_offset >> 6;
        text->y_offsets[i] = glyphs[i].y_offset >> 6;
    }
    return 1;
}

static inline void RaqmRayText_Unload(RaqmRayText *text)
{
    if (text->glyphs)
        free(text->glyphs);
    if (text->x_offsets)
        free(text->x_offsets);
    if (text->y_offsets)
        free(text->y_offsets);
    if (text->rq)
        raqm_destroy(text->rq);
    memset(text, 0, sizeof(*text));
}

// -------------------- Quick Draw API --------------------
static inline void RaqmRay_DrawTextEx(
    RaqmRayFont *font,
    const char *text,
    Vector2 position,
    float fontSize,
    float spacing,
    Color color)
{
    RaqmRayText shaped = {0};
    RaqmRayText_Set(&shaped, font, text);

    float scale = fontSize / (float)font->fontSize;
    float x = position.x;
    float y = position.y + (font->ascender * scale);

    for (size_t i = 0; i < shaped.glyphCount; i++)
    {
        RaqmRayCachedGlyph *glyph = shaped.glyphs[i];
        if (glyph && font->atlas.id > 0)
        {
            Vector2 gpos = {
                x + shaped.x_offsets[i] * scale + glyph->bitmap_left * scale,
                y + shaped.y_offsets[i] * scale - glyph->bitmap_top * scale
            };

            Rectangle dest = {
                .x = gpos.x,
                .y = gpos.y,
                .width = glyph->source.width * scale,
                .height = glyph->source.height * scale
            };

            DrawTexturePro(font->atlas, glyph->source, dest, (Vector2){0, 0}, 0, color);
        }
        if (glyph) x += (glyph->advance * scale) + spacing;
    }

    RaqmRayText_Unload(&shaped);
}

static inline void RaqmRay_DrawText(
    RaqmRayFont *font,
    const char *text,
    int posX,
    int posY,
    float fontSize,
    Color color)
{
    RaqmRay_DrawTextEx(font, text, (Vector2){(float)posX, (float)posY}, fontSize, 0, color);
}

static inline void RaqmRay_DrawTextPro(
    RaqmRayFont *font,
    const char *text,
    Vector2 position,
    Vector2 origin,
    float rotation,
    float fontSize,
    float spacing,
    Color color)
{
    float scaleFactor = 0.75;
    rlPushMatrix();
    rlTranslatef(position.x, position.y, 0.0f);
    rlRotatef(rotation, 0.0f, 0.0f, 1.0f);
    rlTranslatef(-origin.x * scaleFactor, -origin.y, 0.0f);

    RaqmRay_DrawTextEx(font, text, (Vector2){0.0f, 0.0f}, fontSize, spacing, color);

    rlPopMatrix();
}


#endif // RAQM_RAYLIB_H