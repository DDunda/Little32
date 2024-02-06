#pragma once

#ifndef L32_IO_h_
#define L32_IO_h_

#include <array>
#include <istream>
#include <pixels.hpp>
#include <string>
#include <vector>

// We don't want to inherit the min and max macros from windows, it screws up SDL++
#ifndef NOMINMAX
#define NOMINMAX
#include <shlobj.h>
#undef NOMINMAX
#else
#include <shlobj.h>
#endif // !NOMINMAX

namespace SDL
{
	struct Surface;
}

namespace Little32
{
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

	/// <summary>
	/// Opens a Windows dialogue that lets a user pick a file.
	/// </summary>
	/// <param name="out_str">The string that accepts a full file path, if successful</param>
	/// <returns>A windows HRESULT</returns>
	long PickFile(std::wstring& out_str);

	/// <summary>
	/// Converts a stream into a string.
	/// </summary>
	/// <param name="stream">The stream to read from</param>
	/// <param name="out_str">The string to write to</param>
	void StreamToString(std::istream& stream, std::string& out_str);

	/// <summary>
	/// Loads an image containing one or more colour palettes, and assigns it to <paramref name="palettes" />.
	/// </summary>
	/// <param name="palettes">The vector to output the palettes to.</param>
	/// <param name="surface">The surface to read palettes from.</param>
	void LoadPalettes(std::vector<std::array<SDL::Colour, 16>>& palettes, SDL::Surface& surface);
}

#endif