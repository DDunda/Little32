#include "GUIButton.h"

using namespace SDL;

GUIButton::ClickObserver::ClickObserver(GUIButton& parent) : parent(parent)
{
	Input::RegisterEventType(Event::Type::MOUSEBUTTONDOWN, *this);
}

GUIButton::GUIButton(Button button, Uint32 window_ID, Rect area, Colour neutral_colour, Colour hover_colour, Colour click_colour) :
	Subject<const Point, const Uint32>(),
	click_observer(new ClickObserver(*this)),
	button(button),
	window_ID(window_ID),
	area(area),
	neutral_colour(neutral_colour),
	hover_colour(hover_colour),
	click_colour(click_colour) {}

void GUIButton::Render(Renderer& r) const
{
	if (!area.contains(Input::mouse))
	{
		r.SetDrawColour(neutral_colour);
	}
	else if (!Input::button(button))
	{
		r.SetDrawColour(hover_colour);
	}
	else
	{
		r.SetDrawColour(click_colour);
	}

	r.FillRect(area);
}