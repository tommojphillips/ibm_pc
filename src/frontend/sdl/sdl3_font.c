/* sdl3_font.c
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * Font
 */

#include <stdint.h>
#include <stdio.h>

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "sdl3_font.h"

#define DBG_PRINT
#ifdef DBG_PRINT
#define dbg_print(x, ...) printf(x, __VA_ARGS__)
#else
#define dbg_print(x, ...)
#endif

 /* Font texture map */

int font_create_textures(SDL_Renderer* renderer, TTF_TextEngine* engine, int w, int h, FONT_TEXTURE_DATA* font) {
	int result = 0;
	TTF_Text* text = NULL;
	SDL_Surface* surface = NULL;
	char str[2] = { 0, '\0' };

	if (font == NULL) {
		dbg_print("Error: Failed to create text texture. Font map is NULL\n");
		result = 1;
		goto cleanup;
	}

	if (engine == NULL) {
		dbg_print("Error: Failed to create text texture. Text Engine is NULL\n");
		result = 1;
		goto cleanup;
	}

	if (font->ttf == NULL) {
		dbg_print("Error: Failed to create text texture. Font not loaded\n");
		result = 1;
		goto cleanup;
	}

	surface = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_ARGB32);
	if (surface == NULL) {
		dbg_print("Error: Failed to create text texture. Could not create sdl surface. SDL_Err: %s\n", SDL_GetError());
		result = 1;
		goto cleanup;
	}

	for (int i = 0; i < 256; ++i) {
		str[0] = (char)i;

		Uint32 color = 0x00000000;
		SDL_FillSurfaceRect(surface, NULL, color);

		text = TTF_CreateText(engine, font->ttf, &str[0], 25);
		if (text == NULL) {
			dbg_print("Error: Failed to create text texture. Could not create ttf text. SDL_Err: %s\n", SDL_GetError());
			result = 1;
			goto cleanup;
		}
		TTF_SetTextColor(text, 0xFF, 0xFF, 0xFF, 0xFF);
		if (!TTF_DrawSurfaceText(text, 0, 0, surface)) {
			dbg_print("Error: Failed to create text texture. Could not draw text on surface. SDL_Err: %s\n", SDL_GetError());
			result = 1;
			goto cleanup;
		}

		font->textures[i] = SDL_CreateTextureFromSurface(renderer, surface);
		if (font->textures[i] == NULL) {
			dbg_print("Error: Failed to create text texture. Could not create text texture from surface. SDL_Err: %s\n", SDL_GetError());
			result = 1;
			goto cleanup;
		}

		TTF_DestroyText(text);
		text = NULL;
	}

cleanup:

	if (text != NULL) {
		TTF_DestroyText(text);
		text = NULL;
	}
	if (surface != NULL) {
		SDL_DestroySurface(surface);
		surface = NULL;
	}

	return result;
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
	*font = (FONT_TEXTURE_DATA*)calloc(1, sizeof(FONT_TEXTURE_DATA));
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
		return 1;
	}

	dbg_print("Loaded font: %s\n", font_file);
	return 0;
}
void font_destroy_map(FONT_TEXTURE_DATA* font) {
	if (font != NULL) {
		font_destroy_textures(font);

		if (font->ttf != NULL) {
			TTF_CloseFont(font->ttf);
			font->ttf = NULL;
		}

		free(font);
	}
}
