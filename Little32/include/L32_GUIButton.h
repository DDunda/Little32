#ifndef L32_GUIButton_h_
#define L32_GUIButton_h_
#pragma once

#include <input.hpp>
#include <render.hpp>

namespace Little32
{
	struct GUIButton : public Subject<const SDL::Point, const Uint32>, public SDL::InputObserver
	{
		SDL::Button button;
		Uint32 window_ID;
		SDL::Rect area;

		SDL::Colour neutral_colour;
		SDL::Colour hover_colour;
		SDL::Colour click_colour;

		virtual void Notify(const SDL::Event& e)
		{
			if (e.button.windowID != window_ID) return;
			if (e.button.button != (Uint8)button) return;
			if (!area.contains(SDL::Point(e.button.x, e.button.y))) return;

			Subject<const SDL::Point, const Uint32>::Notify(SDL::Point(e.button.x, e.button.y), e.button.windowID);
		}

		virtual void Notify(const SDL::Point mouse, const Uint32 w_id)
		{
			if (w_id != window_ID) return;
			if (!area.contains(mouse)) return;

			Subject<const SDL::Point, const Uint32>::Notify(mouse, w_id);
		}

		inline GUIButton(SDL::Button button, Uint32 window_ID, SDL::Rect area, SDL::Colour neutral_colour, SDL::Colour hover_colour, SDL::Colour click_colour) :
			Subject<const SDL::Point, const Uint32>(),
			SDL::InputObserver(),
			button(button),
			window_ID(window_ID),
			area(area),
			neutral_colour(neutral_colour),
			hover_colour(hover_colour),
			click_colour(click_colour)
		{
			SDL::Input::RegisterEventType(SDL::Event::Type::MOUSEBUTTONUP, *this);
		}

		inline void Render(SDL::Renderer& r) const
		{
			if (!area.contains(SDL::Input::mouse))
			{
				r.SetDrawColour(neutral_colour);
			}
			else if (!SDL::Input::button(button))
			{
				r.SetDrawColour(hover_colour);
			}
			else
			{
				r.SetDrawColour(click_colour);
			}

			r.FillRect(area);
		}
	};

	struct GUIButtonGroup
	{
		std::vector<GUIButton> buttons;

		inline void Render(SDL::Renderer& r) const
		{
			for (auto& b : buttons) b.Render(r);
		}
	};
}

#endif