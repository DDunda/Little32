#include "PickFile.h"

const COMDLG_FILTERSPEC c_rgSaveTypes[] =
{
	{L"Assembly Source Files (*.asm)", L"*.asm"},
	{L"Text Document (*.txt)",         L"*.txt"},
	{L"All Documents (*.*)",           L"*.*"  }
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