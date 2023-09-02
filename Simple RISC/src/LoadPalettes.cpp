#include <image.hpp>

#include "LoadPalettes.h"

const std::array<SDL::Colour, 16> fallback_palettes[] =
{
	{{
		{  0,  0,  0}, {127,  0,  0}, {127, 51,  0}, {127,106,  0}, {  0,127,  0}, {  0,  0,127}, { 87,  0,127}, {127,127,127},
		{ 64, 64, 64}, {255,  0,  0}, {255,106,  0}, {255,216,  0}, {  0,255,  0}, {  0,  0,255}, {178,  0,255}, {255,255,255}
	}},
	{{
		{  0,  0,  0}, { 17, 17, 17}, { 34, 34, 34}, { 51, 51, 51}, { 68, 68, 68}, { 85, 85, 85}, {102,102,102}, {119,119,119},
		{136,136,136}, {153,153,153}, {170,170,170}, {187,187,187}, {204,204,204}, {221,221,221}, {238,238,238}, {255,255,255}
	}}
};

void LoadPalettes(std::vector<std::array<SDL::Colour, 16>>& palettes, const std::string& file_name)
{
	SDL::Surface palette_image = SDL::IMG::Load(file_name.c_str());

	palettes.clear();

	if (palette_image.surface == NULL || (palette_image.surface->w * palette_image.surface->h) % 16 != 0)
	{
		for (const std::array<SDL::Colour, 16>&p : fallback_palettes)
		{
			palettes.push_back(p);
		}
		return;
	}

	if (palette_image.surface->format->format != (Uint32)SDL::PixelFormatEnum::RGBA32)
	{
		palette_image = palette_image.ConvertSurfaceFormat((Uint32)SDL::PixelFormatEnum::RGBA32, 0);
	}

	if (palette_image.MustLock())
	{
		palette_image.Lock();
	}

	SDL::Colour* pixels = (SDL::Colour*)palette_image.surface->pixels;

	const SDL::Colour* end = pixels + palette_image.surface->w * palette_image.surface->h;

	while (pixels != end)
	{
		std::array<SDL::Colour, 16> p = {};
		memcpy(p.data(), pixels, sizeof(p));
		palettes.push_back(p);
		pixels += 16;
	}

	if (palette_image.MustLock())
	{
		palette_image.Unlock();
	}
}