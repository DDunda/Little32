#pragma once

#ifndef L32_VarReference_h_
#define L32_VarReference_h_

#include <list>
#include <string>

namespace Little32
{
	struct ConfigObject;
	struct VarValue;

	enum VarReferenceState
	{
		UNRESOLVED_REF = 0,
		RESOLVING_REF,
		RESOLVED_REF,
		INVALID_REF = -1
	};

	struct VarReference
	{
		bool is_root_reference = false; // '//'

		std::list<std::string> names = {};

		VarReferenceState state = UNRESOLVED_REF;
		const VarValue* target = nullptr;

		bool TryResolveReference(const VarValue* ref_value, const ConfigObject* root, const VarValue*& out) noexcept;
		const VarValue* ResolveReference(const VarValue* ref_value, const ConfigObject* root);

		inline std::string ToString() const
		{
			std::string out = "<";
			if (is_root_reference) out += "//";

			if (!names.empty())
			{
				for (const std::string& name : names)
				{
					out += name + '/';
				}

				out.pop_back(); // Remove last slash
			}

			return out + '>';
		}

		inline bool operator==(const VarReference& other) const
		{
			auto it1 = names.cbegin();
			auto it2 = other.names.cbegin();

			do
			{
				if (it1 == names.cend()) return it2 == other.names.cend(); // They must end at the same time, after matching every part
				if (it2 == other.names.cend()) return false;
			} while (*it1++ == *it2++);

			return false;
		}

		inline bool operator!=(const VarReference& other) const
		{
			return !(*this == other);
		}
	};
}

#endif