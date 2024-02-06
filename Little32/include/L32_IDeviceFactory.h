#pragma once

#ifndef L32_IDeviceFactory_h_
#define L32_IDeviceFactory_h_

#include "L32_Types.h"

#include <filesystem>
#include <unordered_map>
#include <string>

namespace Little32
{
	struct Computer;
	struct IDeviceSettings;

	struct IDeviceFactory
	{
		virtual void CreateFromSettings(
			Computer& computer,
			word& start_address,
			const IDeviceSettings& settings,
			std::unordered_map<std::string, word>& labels,
			std::filesystem::path cur_path
		) const = 0;

		// Throw errors if settings are invalid to provide detailed information to users.
		virtual void VerifySettings(const IDeviceSettings& settings, std::filesystem::path path) const = 0;
	};
}

#endif