/*
 * Copyright (C) 2014 Nebuleon Fumika <nebuleon@gcw-zero.com>
 * 2015 Cong Xu <congusbongus@gmail.com>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include <SDL_image.h>

#include "box.h"
#include "game.h"
#include "main.h"
#include "high_score.h"
#include "init.h"
#include "input.h"
#include "particle.h"
#include "pickup.h"
#include "platform.h"
#include "player.h"
#include "space.h"
#include "sound.h"
#include "title.h"

SDL_Surface *icon = NULL;

void Initialize(bool* Continue, bool* Error)
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0)
	{
		*Continue = false;  *Error = true;
		printf("SDL initialisation failed: %s\n", SDL_GetError());
		SDL_ClearError();
		return;
	}
	else
		printf("SDL initialisation succeeded\n");

	Window = SDL_CreateWindow(
		"Falling Time",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		SCREEN_WIDTH * SCREEN_SCALE, SCREEN_HEIGHT * SCREEN_SCALE,
		SDL_WINDOW_RESIZABLE);
	if (Window == NULL)
	{
		*Continue = false;  *Error = true;
		printf("SDL_CreateWindow failed: %s\n", SDL_GetError());
		SDL_ClearError();
		return;
	}
	else
		printf("SDL_CreateWindow succeeded\n");
	Renderer = SDL_CreateRenderer(Window, -1, 0);
	if (Renderer == NULL)
	{
		*Continue = false;  *Error = true;
		printf("SDL_CreateRenderer failed: %s\n", SDL_GetError());
		SDL_ClearError();
		return;
	}
	else
		printf("SDL_CreateRenderer succeeded\n");
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	SDL_RenderSetLogicalSize(Renderer, SCREEN_WIDTH, SCREEN_HEIGHT);

	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024) == -1)
	{
		*Continue = false;  *Error = true;
		printf("Mix_OpenAudio failed: %s\n", SDL_GetError());
		SDL_ClearError();
		return;
	}
	else
		printf("Mix_OpenAudio succeeded\n");

	InputInit();
	HighScoresInit();

	if (TTF_Init() == -1)
	{
		*Continue = false;  *Error = true;
		printf("TTF_Init failed: %s\n", SDL_GetError());
		SDL_ClearError();
		return;
	}
	else
		printf("TTF_Init succeeded\n");

	icon = IMG_Load("data/graphics/icon.png");
	if (icon == NULL)
	{
		*Continue = false;  *Error = true;
		printf("IMG_Load failed: %s\n", SDL_GetError());
		SDL_ClearError();
		return;
	}
	SDL_SetWindowIcon(Window, icon);

#define LOAD_IMG(_t, _path)\
	_t = LoadTex("data/graphics/" _path);\
	if (_t.T == NULL)\
	{\
		*Continue = false;  *Error = true;\
		return;\
	}
	LOAD_IMG(PlayerSpritesheets[0], "penguin_ball.png");
	LOAD_IMG(PlayerSpritesheets[1], "penguin_black.png");
	LOAD_IMG(PlayerSpritesheets[2], "penguin_blue.png");
	LOAD_IMG(PlayerSpritesheets[3], "penguin_red.png");
	LOAD_IMG(PickupTex, "eggplant.png");

#define LOAD_ANIM(_anim, _path, _w, _h, _fps)\
	if (!AnimationLoad(&(_anim), "data/graphics/" _path, (_w), (_h), (_fps)))\
	{\
		*Continue = false;  *Error = true;\
		return;\
	}
	LOAD_ANIM(Spark, "sparks.png", 4, 4, 20);
	LOAD_ANIM(SparkRed, "sparks_red.png", 4, 4, 20);
	LOAD_ANIM(Tail, "tail.png", 21, 21, 15);

	if (!BoxTexesLoad())
	{
		*Continue = false;  *Error = true;
		return;
	}
	if (!TitleImagesLoad())
	{
		*Continue = false;  *Error = true;
		return;
	}
	if (!BackgroundsLoad(&BG))
	{
		*Continue = false;  *Error = true;
		return;
	}

#define LOAD_SOUND(_sound, _path)\
	_sound = Mix_LoadWAV("data/sounds/" _path);\
	if (_sound == NULL)\
	{\
		*Continue = false;  *Error = true;\
		printf("Mix_LoadWAV failed: %s\n", SDL_GetError());\
		SDL_ClearError();\
		return;\
	}
	LOAD_SOUND(SoundBeep, "beep.ogg");
	LOAD_SOUND(SoundPlayerBounce, "bounce.ogg");
	LOAD_SOUND(SoundStart, "start.ogg");
	LOAD_SOUND(SoundLose, "lose.ogg");
	LOAD_SOUND(SoundScore, "score.ogg");
	SoundLoad();

	music = Mix_LoadMUS("data/sounds/music.ogg");
	if (music == NULL)
	{
		*Continue = false;  *Error = true;
		printf("Mix_LoadMUS failed: %s\n", SDL_GetError());
		SDL_ClearError();
		return;
	}

#define LOAD_FONT(_f, _file, _size)\
	_f = TTF_OpenFont("data/" _file, _size);\
	if (_f == NULL)\
	{\
		*Continue = false;  *Error = true;\
		printf("TTF_OpenFont failed: %s\n", SDL_GetError());\
		SDL_ClearError();\
		return;\
	}
	LOAD_FONT(font, "LondrinaSolid-Regular.otf", 20);
	LOAD_FONT(hsFont, "LondrinaSolid-Regular.otf", 16);

	SpaceInit(&space);
	ParticlesInit();
	PickupsInit();

	SDL_ShowCursor(0);

	InitializePlatform();

	// Title screen. (-> title.c)
	MusicSetLoud(false);
	Mix_PlayMusic(music, -1);
	ToTitleScreen(true);
}

void Finalize()
{
	PickupsFree();
	ParticlesFree();
	SpaceFree(&space);
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		SDL_DestroyTexture(PlayerSpritesheets[i].T);
	}
	SDL_DestroyTexture(PickupTex.T);
	SDL_FreeSurface(icon);
	AnimationFree(&Spark);
	AnimationFree(&SparkRed);
	AnimationFree(&Tail);
	BoxTexesFree();
	TitleImagesFree();
	BackgroundsFree(&BG);
	Mix_FreeChunk(SoundPlayerBounce);
	Mix_FreeChunk(SoundBeep);
	Mix_FreeChunk(SoundStart);
	Mix_FreeChunk(SoundLose);
	Mix_FreeChunk(SoundScore);
	Mix_FreeMusic(music);
	SoundFree();
	TTF_CloseFont(font);
	TTF_CloseFont(hsFont);
	HighScoresFree();
	InputFree();
	SDL_DestroyRenderer(Renderer);
	SDL_DestroyWindow(Window);
	SDL_Quit();
}
