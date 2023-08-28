#include "Sprite.h"

void Sprite::Render(SDL::Renderer& r)
{
	r.Copy(txt, shape);
}