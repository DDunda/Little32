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

		inline void Render() { if (enabled) txt.CopyF(shape); }
		inline void Render(SDL::Rect viewport)
		{
			if (!enabled) return;
		
			SDL_Renderer* r = txt.renderer.get();

			SDL::Rect vp;
			SDL_RenderGetViewport(r, &vp.rect);
			const SDL::Rect new_viewport = viewport + vp.pos;
			SDL_RenderSetViewport(r, &new_viewport.rect);

			txt.CopyF(shape);

			SDL_RenderSetViewport(r, &vp.rect);
		}
		inline void SetScaleMode(const SDL::Texture::ScaleMode mode) { txt.SetScaleMode(mode); };
		inline void SetBlendMode(const SDL::BlendMode mode) { txt.SetBlendMode(mode); };
	};

	struct SpriteGroup
	{
		std::vector<Sprite> sprites;
		SDL::Rect viewport;

		inline void Render()
			{ for (auto& s : sprites) s.Render(viewport); }
		inline void SetScaleMode(const SDL::Texture::ScaleMode mode)
			{ for (auto& s : sprites) s.SetScaleMode(mode); }
		inline void SetBlendMode(const SDL::BlendMode mode)
			{ for (auto& s : sprites) s.SetBlendMode(mode); }
	};
}

#endif