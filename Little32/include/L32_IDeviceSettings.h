#pragma once

#ifndef L32_IDeviceSettings_h_
#define L32_IDeviceSettings_h_

#include "L32_Types.h"
#include "L32_VarValue.h"

#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace Little32
{
	struct ConfigObject;

	/// <summary>
	/// A package of information used to construct an IDevice for a computer with an IDeviceFactory
	/// </summary>
	struct IDeviceSettings
	{
		// The order these settings were loaded in (0-based).
		size_t component_num;
		// This type of this device (e.g. "RAM", "Keyboard"). Case and whitespace sensitive.
		std::string component_type;
		std::unordered_map<std::string, VarValue> settings;
		// Name of relative address -> labels
		std::unordered_map<std::string, std::vector<std::string>> named_labels;
		// Label -> offset from start of component
		std::unordered_map<std::string, word> relative_labels;

		IDeviceSettings(size_t num,
			std::string type = "",
			std::unordered_map<std::string, VarValue> settings = {},
			std::unordered_map<std::string, std::vector<std::string>> named_labels = {},
			std::unordered_map<std::string, word> relative_labels = {}
		) noexcept;

		/// <summary>
		/// Constructs a ComponentSettings using a specification stored as in a VarValue object.
		/// Will throw exceptions if incorrectly formatted. num should be the index from 0, but
		/// errors will use it as an ordinal from 1 (i.e 0->1st, 1->2nd, 2->3rd).
		/// </summary>
		IDeviceSettings(size_t num, const ConfigObject& root, const VarValue& obj, std::set<std::string>& all_labels);

		inline bool Contains(const std::string& setting_name) const noexcept
			{ return settings.contains(setting_name); }

		inline VarValue& operator[](const std::string& setting_name)
		{
			return settings.at(setting_name);
		}

		inline const VarValue& operator[](const std::string& setting_name) const
		{
			return settings.at(setting_name);
		}

		bool operator==(const IDeviceSettings& other) const;

		bool operator!=(const IDeviceSettings& other) const;
	};
}

#endif