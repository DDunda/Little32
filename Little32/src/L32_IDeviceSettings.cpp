#include "L32_IDeviceSettings.h"

#include "L32_BigInt.h"
#include "L32_ConfigObject.h"
#include "L32_String.h"
#include "L32_VarValue.h"

#include <exception>

namespace Little32
{
	IDeviceSettings::IDeviceSettings(size_t num,
		std::string type,
		std::unordered_map<std::string, VarValue> settings,
		std::unordered_map<std::string, std::vector<std::string>> named_labels,
		std::unordered_map<std::string, word> relative_labels
	) noexcept : component_num(num),
	component_type(type),
	settings(settings),
	named_labels(named_labels),
	relative_labels(relative_labels)
	{}

	IDeviceSettings::IDeviceSettings(size_t num, const ConfigObject& root, const VarValue& obj, std::set<std::string>& all_labels)
		: component_num(num)
	{
		if (obj.GetType() != OBJECT_VAR)
		{
			throw std::runtime_error
			(
				"Expected components list of '" +
				std::string(VALUETYPE_NAMES.at(OBJECT_VAR)) +
				"', got '" +
				std::string(VALUETYPE_NAMES.at(obj.GetType())) +
				"' as " +
				ToOrdinal(num + 1) +
				" value"
			);
		}

		std::set<const VarValue*> evaluating = {};
		VarValue settings_value;

		if (!obj.TryCopyResolved(&root, evaluating, settings_value))
		{
			throw std::runtime_error
			(
				"Could not load " +
				ToOrdinal(num + 1) +
				" component due to unresolveable reference/s"
			);
		}

		if (!settings_value.TryFindString(root, "component_type", component_type))
		{
			throw std::runtime_error
			(
				ToOrdinal(num + 1) +
				" component does not specify component_type"
			);
		}

		settings_value.GetObjectValue().settings.erase("component_type");

		const ConfigObject* labels_obj = nullptr;

		if (settings_value.TryFindObject(root, "labels", labels_obj))
		{
			for (const auto& [label, value] : labels_obj->settings)
			{
				if (all_labels.contains(label)) throw std::runtime_error("Duplicate label detected in " + ToOrdinal(num + 1) + " component (" + label + ")");

				all_labels.insert(label);

				if (value.GetType() == STRING_VAR)
				{
					const std::string& label_name = value.GetStringValue();

					if (!named_labels.contains(label_name))
						named_labels[label_name] = { label };
					else
						named_labels[label_name].push_back(label);

					continue;
				}
				else if (value.GetType() != INTEGER_VAR)
				{
					throw std::runtime_error(
						"Unknown label type '" +
						std::string(VALUETYPE_NAMES.at(value.GetType())) +
						"' in " +
						ToOrdinal(component_num + 1) +
						" component (" + label + ")"
					);
				}

				const size_t bits = value.GetIntegerValue().NumBits();

				if (bits > 32)
				{
					throw std::runtime_error
					(
						"Label offset is too large in " +
						ToOrdinal(component_num + 1) +
						" component (" +
						label +
						": " +
						value.GetIntegerValue().ToStringCheap() +
						")"
					);
				}
				else if (bits == 0)
				{
					relative_labels[label] = 0;
				}
				else if (value.GetIntegerValue().negative)
				{
					relative_labels[label] = (~value.GetIntegerValue().bits[0]) + 1;
				}
				else
				{
					relative_labels[label] = value.GetIntegerValue().bits[0];
				}
			}

			settings_value.GetObjectValue().settings.erase("labels");
		}

		settings = settings_value.GetObjectValue().settings;
	}

	bool IDeviceSettings::operator==(const IDeviceSettings & other) const
	{
		if (!(component_type == other.component_type
			&& settings == other.settings
			&& relative_labels.size() == other.relative_labels.size()
			&& named_labels.size() == other.named_labels.size())) return false;

		for (const auto& [name, value] : relative_labels)
		{
			if (other.relative_labels.count(name) == 0) return false;
			if (value != other.relative_labels.at(name)) return false;
		}

		for (const auto& [name, value] : named_labels)
		{
			if (other.named_labels.count(name) == 0) return false;
			if (value != other.named_labels.at(name)) return false;
		}

		return true;
	}

	bool IDeviceSettings::operator!=(const IDeviceSettings & other) const
	{
		return !(*this == other);
	}
}