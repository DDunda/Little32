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

namespace Little32
{
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
	/// <param name="file_name">The name of the file to read palettes from.</param>
	void LoadPalettes(std::vector<std::array<SDL::Colour, 16>>& palettes, const std::string& file_name = "palette.png");
}

#endif