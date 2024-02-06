#pragma once

#ifndef L32_ImageLoader_h_
#define L32_ImageLoader_h_

#include <mutex.hpp>
#include <render.hpp>
#include <SDL_image.hpp>

#include <filesystem>
#include <map>

namespace Little32
{
	class ImageLoader
	{
		inline static std::shared_ptr<SDL_Renderer> renderer = nullptr;

		inline static void LoaderDestructor(SDL_Texture* texture)
		{
			image_lock->Lock();
			for (auto& [path, txt] : loaded_images)
			{
				if (txt.txt != texture) continue;

				txt.share_count--;
				if (txt.share_count > 0)
				{
					image_lock->Unlock();
					return;
				}

				loaded_images.erase(path);

				SDL_DestroyTexture(texture);

				image_lock->Unlock();
				return;
			}

			// This texture must always be found
			assert(false);
		}

		struct TextureBlob
		{
			size_t share_count = 0;
			SDL_Texture* txt = nullptr;
		};

		inline static SDL::Mutex* image_lock = nullptr;
		inline static std::map<std::filesystem::path, TextureBlob> loaded_images = {};

	public:

		static void Init(std::shared_ptr<SDL_Renderer> renderer)
		{
			ImageLoader::renderer = renderer;
			image_lock = new SDL::Mutex();
		}

		static void Quit()
		{
			ImageLoader::renderer = nullptr;
			delete image_lock;
			image_lock = nullptr;
		}

		static SDL::Texture GetImage(std::filesystem::path image_path)
		{
			image_path = std::filesystem::weakly_canonical(image_path);

			image_lock->Lock();

			for (auto& [path, txt] : loaded_images)
			{
				if (image_path != path) continue;

				txt.share_count++;

				SDL::Texture texture = SDL::Texture(
					renderer,
					std::shared_ptr<SDL_Texture>(txt.txt, LoaderDestructor)
				);

				image_lock->Unlock();
				return texture;
			}

			SDL_Texture* txt_ptr = IMG_LoadTexture(
				renderer.get(),
				image_path.string().c_str()
			);

			if (txt_ptr == NULL)
			{
				//std::cout << "Failed to load image at '" + image_path.string() << "'" << std::endl;
				image_lock->Unlock();
				return {};
			}

			SDL::Texture texture = SDL::Texture(
				renderer,
				std::shared_ptr<SDL_Texture>(
					(loaded_images[image_path] =
					{
						1,
						txt_ptr
					}).txt,
					LoaderDestructor
				)
			);

			image_lock->Unlock();
			return texture;
		}
	};
}

#endif