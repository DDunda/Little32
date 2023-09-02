#pragma once

#ifndef SR_PickFile_h_
#define SR_PickFile_h_

#include <shlobj.h>
#include <string>

/// <summary>
/// Opens a Windows dialogue that lets a user pick a file
/// </summary>
/// <param name="out_str">The string that accepts a full file path, if successful</param>
/// <returns>A windows HRESULT</returns>
long PickFile(std::wstring& out_str);

#endif