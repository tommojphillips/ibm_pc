/* sdl3_font.c
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * Font
 */

#include <stdint.h>

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "sdl3_font.h"

#define DBG_PRINT
#ifdef DBG_PRINT
#include <stdio.h>
#define dbg_print(x, ...) printf(x, __VA_ARGS__)
#else
#define dbg_print(x, ...)
#endif

 /* Font texture map */

int font_create_textures(SDL_Renderer* renderer, TTF_TextEngine* engine, FONT_TEXTURE_DATA* font) {

	if (font == NULL) {
		dbg_print("Error: Failed to create text texture. Font map is NULL\n");
		return 1;
	}

	if (engine == NULL) {
		dbg_print("Error: Failed to create text texture. Text engine is NULL\n");
		return 1;
	}

	if (font->ttf == NULL) {
		dbg_print("Error: Failed to create text texture. Font not loaded\n");
		return 1;
	}

	for (int i = 0; i < 256; ++i) {
				
		SDL_Color white = { 255, 255, 255, 255 };
		SDL_Surface* glyph = TTF_RenderGlyph_Blended(font->ttf, i, white);
		if (glyph == NULL) {
			dbg_print("Error: Failed to create text texture. Could not render glyph. SDL_Err: %s\n", SDL_GetError());
			return 1;
		}

		font->textures[i] = SDL_CreateTextureFromSurface(renderer, glyph);

		SDL_DestroySurface(glyph);
		glyph = NULL;
		
		if (font->textures[i] == NULL) {
			dbg_print("Error: Failed to create text texture. Could not create text texture from surface. SDL_Err: %s\n", SDL_GetError());
			return 1;
		}
		
		if (!SDL_SetTextureBlendMode(font->textures[i], SDL_BLENDMODE_BLEND)) {
			dbg_print("Error: Failed to create text texture. Could not set blend mode. SDL_Err: %s\n", SDL_GetError());
			return 1;
		}
		
		if (!SDL_SetTextureScaleMode(font->textures[i], SDL_SCALEMODE_NEAREST)) {
			dbg_print("Error: Failed to create text texture. Could not set scale mode. SDL_Err: %s\n", SDL_GetError());
			return 1;
		}
	}

	return 0;
}

void font_destroy_textures(FONT_TEXTURE_DATA* font) {
	if (font != NULL) {
		for (int i = 0; i < 256; ++i) {
			if (font->textures[i] != NULL) {
				SDL_DestroyTexture(font->textures[i]);
				font->textures[i] = NULL;
			}
		}
	}
}

int font_create_map(FONT_TEXTURE_DATA** font) {
	*font = calloc(1, sizeof(FONT_TEXTURE_DATA));
	if (*font == NULL) {
		dbg_print("Failed to allocate font map\n");
		return 1;
	}
	return 0;
}
void font_close_font(FONT_TEXTURE_DATA* font) {
	if (font->ttf != NULL) {
		dbg_print("Closing font\n");
		TTF_CloseFont(font->ttf);
		font->ttf = NULL;
	}
}
int font_open_font(FONT_TEXTURE_DATA* font, const char* font_file) {
	if (font_file == NULL) {
		dbg_print("Error: Failed to open font. font file path is NULL\n");
		return 1;
	}

	font_close_font(font);

	font->ttf = TTF_OpenFont(font_file, 25);
	if (font->ttf == NULL) {
		dbg_print("Error: Failed to open font. SDL_Err: %s\n", SDL_GetError());
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Failed to open font", SDL_GetError(), NULL);
		return 1;
	}

	dbg_print("Loaded font: %s\n", font_file);
	return 0;
}
void font_destroy_map(FONT_TEXTURE_DATA* font) {
	if (font != NULL) {
		font_close_font(font);
		font_destroy_textures(font);
		free(font);
	}
}
