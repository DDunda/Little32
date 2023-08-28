#pragma once

#ifndef SR_GUIButton_h_
#define SR_GUIButton_h_

#include <input.hpp>
#include <render.hpp>

class GUIButton : public Subject<const SDL::Point, const Uint32>
{
	struct ClickObserver : public SDL::InputObserver
	{
		GUIButton& parent;

		ClickObserver(GUIButton& parent);

		void SDL::InputObserver::Notify(const SDL::Event& e)
		{
			if (e.button.windowID != parent.window_ID) return;
			if (e.button.button != (Uint8)parent.button) return;
			if (!SDL::Point(e.button.x, e.button.y).inRect(parent.area)) return;

			parent.Notify(SDL::Point(e.button.x, e.button.y), e.button.windowID);
		}
	};

	ClickObserver* click_observer = nullptr;

public:
	SDL::Button button;
	Uint32 window_ID;
	SDL::Rect area;

	SDL::Colour neutral_colour;
	SDL::Colour hover_colour;
	SDL::Colour click_colour;

	GUIButton(SDL::Button button, Uint32 window_ID, SDL::Rect area, SDL::Colour neutral_colour, SDL::Colour hover_colour, SDL::Colour click_colour);

	void Render(SDL::Renderer& r) const;
};

#endif