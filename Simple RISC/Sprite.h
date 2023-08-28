#pragma once

#ifndef SR_Sprite_h
#define SR_Sprite_h

#include <render.hpp>

struct Sprite
{
	SDL::Texture txt;
	SDL::Rect shape;

	void Render(SDL::Renderer& r);
};

#endif