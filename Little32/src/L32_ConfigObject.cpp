#include "L32_ConfigObject.h"

#include "L32_BigInt.h"
#include "L32_VarReference.h"
#include "L32_VarValue.h"

namespace Little32
{
	ConfigObject::ConfigObject(std::initializer_list<std::unordered_map<std::string, VarValue>::value_type> _Ilist) : settings(_Ilist) {}

	ConfigObject::ConfigObject(std::unordered_map<std::string, VarValue>&& settings) : settings(settings) {}

	ConfigObject::ConfigObject() : settings() {}

	bool ConfigObject::TryFindValue(std::string_view path, const VarValue*& out) const
	{
		VarReference ref = VarReference();
		out = nullptr;

		if (path.size() >= 2 && path[0] == '/' && path[1] == '/')
		{
			ref.is_root_reference = true;
		}

		size_t i;

		while ((i = path.find_first_of('/')) != std::string::npos)
		{
			ref.names.push_back(std::string(path.substr(0, i)));
			path = path.substr(i + 1);
		}

		ref.names.push_back(std::string(path));

		try
		{
			out = ref.ResolveReference(nullptr, this);
			return true;
		}
		catch (const std::exception& e)
		{
			return false;
		}
	}

	bool ConfigObject::TryFindValueType(std::string_view path, const VarValue*& out, ValueType type) const
	{
		out = nullptr;

		const VarValue* var;

		if (!TryFindValue(path, var)) return false;
		if (var->GetType() != type) return false;
		out = var;
		return true;
	}

	bool ConfigObject::TryFindInt64(std::string_view path, int64_t& out) const
	{
		const VarValue* var;

		if (!TryFindValue(path, var)) return false;
		if (var->GetType() != INTEGER_VAR) return false;
		return var->GetIntegerValue().TryToInt64(out);
	}

	bool ConfigObject::TryFindUInt64(std::string_view path, uint64_t& out) const
	{
		const VarValue* var;

		if (!TryFindValue(path, var)) return false;
		if (var->GetType() != INTEGER_VAR) return false;
		return var->GetIntegerValue().TryToUInt64(out);
	}

	bool ConfigObject::TryFindInt32(std::string_view path, int32_t& out) const
	{
		const VarValue* var;

		if (!TryFindValue(path, var)) return false;
		if (var->GetType() != INTEGER_VAR) return false;
		return var->GetIntegerValue().TryToInt32(out);
	}

	bool ConfigObject::TryFindUInt32(std::string_view path, uint32_t& out) const
	{
		const VarValue* var;

		if (!TryFindValue(path, var)) return false;
		if (var->GetType() != INTEGER_VAR) return false;
		return var->GetIntegerValue().TryToUInt32(out);
	}

	bool ConfigObject::TryFindInt16(std::string_view path, int16_t& out) const
	{
		const VarValue* var;

		if (!TryFindValue(path, var)) return false;
		if (var->GetType() != INTEGER_VAR) return false;
		return var->GetIntegerValue().TryToInt16(out);
	}

	bool ConfigObject::TryFindUInt16(std::string_view path, uint16_t& out) const
	{
		const VarValue* var;

		if (!TryFindValue(path, var)) return false;
		if (var->GetType() != INTEGER_VAR) return false;
		return var->GetIntegerValue().TryToUInt16(out);
	}

	bool ConfigObject::TryFindInt8(std::string_view path, int8_t& out) const
	{
		const VarValue* var;

		if (!TryFindValue(path, var)) return false;
		if (var->GetType() != INTEGER_VAR) return false;
		return var->GetIntegerValue().TryToInt8(out);
	}

	bool ConfigObject::TryFindUInt8(std::string_view path, uint8_t& out) const
	{
		const VarValue* var;

		if (!TryFindValue(path, var)) return false;
		if (var->GetType() != INTEGER_VAR) return false;
		return var->GetIntegerValue().TryToUInt8(out);
	}

	bool ConfigObject::TryFindIntX(std::string_view path, BigInt& out, size_t bits) const
	{
		const VarValue* var;

		if (!TryFindValue(path, var)) return false;
		if (var->GetType() != INTEGER_VAR) return false;
		return var->GetIntegerValue().TryToIntX(out, bits);
	}

	bool ConfigObject::TryFindUIntX(std::string_view path, BigInt& out, size_t bits) const
	{
		const VarValue* var;

		if (!TryFindValue(path, var)) return false;
		if (var->GetType() != INTEGER_VAR) return false;
		return var->GetIntegerValue().TryToUIntX(out, bits);
	}

	bool ConfigObject::TryFindInteger(std::string_view path, BigInt& out) const
	{
		const VarValue* var;

		if (!TryFindValue(path, var)) return false;
		if (var->GetType() != INTEGER_VAR) return false;
		out = var->GetIntegerValue();
		return true;
	}

	bool ConfigObject::TryFindFloat(std::string_view path, float& out) const
	{
		const VarValue* var;

		if (!TryFindValue(path, var)) return false;
		if (var->GetType() != INTEGER_VAR) return false;
		out = (const float&)*var;
		return true;
	}

	bool ConfigObject::TryFindString(std::string_view path, std::string& out) const
	{
		const VarValue* var;

		if (!TryFindValue(path, var)) return false;
		if (var->GetType() != STRING_VAR) return false;
		out = var->GetStringValue();
		return true;
	}

	bool ConfigObject::TryFindColour(std::string_view path, SDL::Colour& out) const
	{
		const VarValue* var;

		if (!TryFindValue(path, var)) return false;
		if (var->GetType() != COLOUR_VAR) return false;
		out = var->GetColourValue();
		return true;
	}

	bool ConfigObject::TryFindVector(std::string_view path, SDL::Point& out) const
	{
		const VarValue* var;

		if (!TryFindValue(path, var)) return false;
		if (var->GetType() != VECTOR_VAR) return false;
		out = var->GetVectorValue();
		return true;
	}

	bool ConfigObject::TryFindObject(std::string_view path, const ConfigObject*& out) const
	{
		const VarValue* var;

		if (!TryFindValue(path, var)) return false;
		if (var->GetType() != OBJECT_VAR) return false;
		out = &(var->GetObjectValue());
		return true;
	}

	bool ConfigObject::TryFindList(std::string_view path, const std::vector<VarValue>*& out) const
	{
		const VarValue* var;

		if (!TryFindValue(path, var)) return false;
		if (var->GetType() != LIST_VAR) return false;
		out = &(var->GetListValues());
		return true;
	}

	bool ConfigObject::TryFindBool(std::string_view path, bool& out) const
	{
		const VarValue* var;

		if (!TryFindValue(path, var)) return false;
		if (var->GetType() != BOOLEAN_VAR) return false;
		out = var->GetBooleanValue();
		return true;
	}

	const VarValue& ConfigObject::FindValue(std::string_view path) const
	{
		VarReference ref;

		if (path.size() >= 2 && path[0] == '/' && path[1] == '/')
		{
			ref.is_root_reference = true;
		}

		size_t i;

		while ((i = path.find_first_of('/')) != std::string::npos)
		{
			ref.names.push_back(std::string(path.substr(0, i)));
			path = path.substr(i + 1);
		}

		ref.names.push_back(std::string(path));

		return *ref.ResolveReference(nullptr, this);
	}

	const VarValue& ConfigObject::FindValueType(std::string_view path, ValueType type) const
	{
		const VarValue& value = FindValue(path);
		if (value.GetType() != type) throw std::runtime_error("Expected value of '" + std::string(VALUETYPE_NAMES.at(type)) + "' type, got '" + std::string(VALUETYPE_NAMES.at(value.GetType())) + "' type");
		return value;
	}

	bool ConfigObject::operator==(const ConfigObject& other) const
	{
		if (settings.size() != other.settings.size()) return false;

		for (const auto& [name, value] : settings)
		{
			if (other.settings.count(name) == 0) return false;
			//if (value != other.settings.at(name)) return false;
		}

		return true;
	}

	bool ConfigObject::operator!=(const ConfigObject& other) const
	{
		if (settings.size() != other.settings.size()) return true;

		for (const auto& [name, value] : settings)
		{
			if (other.settings.count(name) == 0) return true;
			//if (value != other.settings.at(name)) return true;
		}

		return false;
	}
}