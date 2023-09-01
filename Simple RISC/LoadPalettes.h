#pragma once

#ifndef SR_LoadPalettes_h_
#define SR_LoadPalettes_h_

#include <array>
#include <string>

#include <pixels.hpp>

extern const std::array<SDL::Colour, 16> fallback_palettes[];

/// <summary>
/// Loads an image containing one or more colour palettes, and assigns it to <paramref name="palettes" />.
/// </summary>
/// <param name="palettes">The vector to output the palettes to.</param>
/// <param name="file_name">The name of the file to read palettes from.</param>
void LoadPalettes(std::vector<std::array<SDL::Colour, 16>>& palettes, const std::string& file_name = "palette.png");

#endif