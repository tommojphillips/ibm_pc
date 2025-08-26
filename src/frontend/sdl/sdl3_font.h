/* sdl3_font.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * Font
 */

#ifndef SDL3_FONT_H
#define SDL3_FONT_H

typedef struct SDL_Texture SDL_Texture;
typedef struct TTF_Font TTF_Font;

 /* Font texture data */
typedef struct FONT_TEXTURE_DATA {
	SDL_Texture* textures[256];
	TTF_Font* ttf;
} FONT_TEXTURE_DATA;

int font_create_textures(SDL_Renderer* renderer, TTF_TextEngine* engine, int w, int h, FONT_TEXTURE_DATA* font);
void font_destroy_textures(FONT_TEXTURE_DATA* font);

int font_create_map(FONT_TEXTURE_DATA** font);
void font_destroy_map(FONT_TEXTURE_DATA* font);

int font_open_font(FONT_TEXTURE_DATA* font, const char* font_file);
void font_close_font(FONT_TEXTURE_DATA* font);

#endif
