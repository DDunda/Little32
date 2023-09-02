#include "Sprite.h"

void Sprite::Render(SDL::Renderer& r)
{
	r.CopyF(txt, shape);
}