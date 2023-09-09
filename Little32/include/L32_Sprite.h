#pragma once

#ifndef L32_Sprite_h
#define L32_Sprite_h

#include <render.hpp>
#include <vector>

namespace Little32
{
	struct Sprite
	{
		SDL::Texture txt;
		SDL::FRect shape;
		bool enabled = true;

		inline Sprite(SDL::Texture& txt, const SDL::FRect& shape, bool enabled = true) : txt(txt), shape(shape), enabled(enabled) {}
		inline Sprite(const Sprite& spr) : txt(spr.txt), shape(spr.shape), enabled(spr.enabled) {}
		inline Sprite(Sprite&& spr) noexcept : enabled(spr.enabled) { std::swap(txt, spr.txt); std::swap(shape, spr.shape); }

		inline Sprite& operator=(const Sprite& spr) { txt = spr.txt; shape = spr.shape; enabled = spr.enabled; return *this; }
		inline Sprite& operator=(Sprite&& spr) noexcept { std::swap(txt, spr.txt); std::swap(shape, spr.shape); enabled = spr.enabled; return *this; }

		inline void Render(SDL::Renderer& r) { if (enabled) r.CopyF(txt, shape); }
		inline void SetScaleMode(const SDL::Texture::ScaleMode mode) { txt.SetScaleMode(mode); };
		inline void SetBlendMode(const SDL::BlendMode mode) { txt.SetBlendMode(mode); };
	};

	struct SpriteGroup
	{
		std::vector<Sprite> sprites;

		inline void Render(SDL::Renderer& r)
			{ for (auto& s : sprites) s.Render(r); }
		inline void SetScaleMode(const SDL::Texture::ScaleMode mode)
			{ for (auto& s : sprites) s.SetScaleMode(mode); }
		inline void SetBlendMode(const SDL::BlendMode mode)
			{ for (auto& s : sprites) s.SetBlendMode(mode); }
	};
}

#endif