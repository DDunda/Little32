#include "L32_IO.h"

#include <SDL_image.hpp>

namespace Little32
{
	constexpr COMDLG_FILTERSPEC c_rgSaveTypes[]
	{
		{L"Assembly Source Files (*.asm)", L"*.asm"},
		{L"Text Document (*.txt)",         L"*.txt"},
		{L"All Documents (*.*)",           L"*.*"  }
	};

	constexpr std::array<SDL::Colour, 16> fallback_palettes[]
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

	long PickFile(std::wstring& out_str)
	{
		// CoCreate the File Open Dialog object.
		IFileDialog* pfd = NULL;
		long hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));
		if (!SUCCEEDED(hr)) return hr;

		// Set the options on the dialog.
		unsigned long dwFlags;

		// Before setting, always get the options first in order not to override existing options.
		hr = pfd->GetOptions(&dwFlags);
		if (SUCCEEDED(hr))
		{
			// In this case, get shell items only for file system items.
			hr = pfd->SetOptions(dwFlags | FOS_FORCEFILESYSTEM);
			if (SUCCEEDED(hr))
			{
				hr = pfd->SetFileTypes(ARRAYSIZE(c_rgSaveTypes), c_rgSaveTypes);
				if (SUCCEEDED(hr))
				{
					hr = pfd->SetFileTypeIndex(1);
					if (SUCCEEDED(hr))
					{
						hr = pfd->SetDefaultExtension(L"asm");
						if (SUCCEEDED(hr))
						{
							hr = pfd->Show(NULL);
							if (SUCCEEDED(hr))
							{
								// Obtain the result, once the user clicks the 'Open' button.
								// The result is an IShellItem object.
								IShellItem* psiResult;
								hr = pfd->GetResult(&psiResult);
								if (SUCCEEDED(hr))
								{
									wchar_t* tmp_str = NULL;
									hr = psiResult->GetDisplayName(SIGDN_FILESYSPATH, &tmp_str);

									if (SUCCEEDED(hr))
									{
										out_str = tmp_str;
										CoTaskMemFree(tmp_str);
									}

									psiResult->Release();
								}
							}
						}
					}
				}
			}
		}

		pfd->Release();

		return hr;
	}

	// https://stackoverflow.com/a/116220
	void StreamToString(std::istream& stream, std::string& out_str)
	{
		constexpr size_t read_size = 4096;
		stream.exceptions(std::ios_base::badbit);

		if (not stream)
		{
			throw std::ios_base::failure("file does not exist");
		}

		auto buf = std::string(read_size, '\0');
		while (stream.read(&buf[0], read_size))
		{
			out_str.append(buf, 0, stream.gcount());
		}
		out_str.append(buf, 0, stream.gcount());
	}

	void LoadPalettes(std::vector<std::array<SDL::Colour, 16>>& palettes, const std::string& file_name)
	{
		using namespace SDL;

		Surface palette_image = IMG::Load(file_name.c_str());

		palettes.clear();

		if (palette_image.surface == NULL || (palette_image.surface->w * palette_image.surface->h) % 16 != 0)
		{
			palettes.reserve(sizeof(fallback_palettes) / sizeof(std::array<SDL::Colour, 16>));
			for (const auto& p : fallback_palettes)
			{
				palettes.push_back(p);
			}
			return;
		}

		if (palette_image.surface->format->format != (Uint32)SDL::PixelFormatEnum::RGBA32)
		{
			palette_image = palette_image.ConvertSurfaceFormat((Uint32)SDL::PixelFormatEnum::RGBA32, 0);
		}

		const bool is_locked = palette_image.MustLock();

		if (is_locked) palette_image.Lock();

		const SDL::Colour* pixels = (SDL::Colour*)palette_image.surface->pixels;
		const SDL::Colour* end = pixels + palette_image.surface->w * palette_image.surface->h;

		while (pixels != end)
		{
			std::array<SDL::Colour, 16> p = {};
			memcpy(p.data(), pixels, sizeof(p));
			palettes.push_back(p);
			pixels += 16;
		}

		if (is_locked) palette_image.Unlock();
	}
}