#pragma once

#ifndef L32_EmptyDeviceFactory_h_
#define L32_EmptyDeviceFactory_h_

#include "L32_BigInt.h"
#include "L32_IDeviceFactory.h"
#include "L32_IDeviceSettings.h"
#include "L32_VarValue.h"

namespace Little32
{
	struct EmptyDeviceFactory : IDeviceFactory
	{
		void CreateFromSettings(
			Computer& computer,
			word& start_address,
			const IDeviceSettings& settings,
			std::unordered_map<std::string, word>& labels,
			std::filesystem::path cur_path
		) const
		{
			assert(settings.settings.contains("size_words"));
			const VarValue& size_val = settings.settings.at("size_words");

			assert(size_val.GetType() == INTEGER_VAR);

			const BigInt& bint = size_val.GetIntegerValue();

			if (bint.bits.empty()) return;

			assert(bint.NumBits() <= 32);

			word offset = static_cast<word>(bint.bits[0]);

			if (bint.negative) offset = (~offset) + 1;

			start_address += offset;
		}

		// Throw errors if settings are invalid to provide detailed information to users.
		inline void VerifySettings(const IDeviceSettings& settings, std::filesystem::path path) const
		{
			if (!settings.named_labels.empty()) throw std::exception("Expected no named labels for empty device");
			if (!settings.settings.contains("size_words")) throw std::exception("Expected 'size_words' in Empty device");
			const VarValue& size_val = settings.settings.at("size_words");

			if (size_val.GetType() != INTEGER_VAR)
			{
				throw std::runtime_error(
					"Expected 'size_words' to be type '" +
						std::string(VALUETYPE_NAMES.at(INTEGER_VAR)) +
					"', got type '" +
						std::string(VALUETYPE_NAMES.at(size_val.GetType())) +
					'\''
				);
			}

			const BigInt& bint = size_val.GetIntegerValue();

			if (bint.NumBits() > 32)
			{
				throw std::runtime_error(
					"'size_words' must be 32 bits or less (" +
					bint.ToStringCheap() + ')'
				);
			}
		}
	};
}

#endif