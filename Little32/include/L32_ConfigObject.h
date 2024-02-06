#pragma once

#ifndef L32_ConfigObject_h_
#define L32_ConfigObject_h_

#include "L32_Types.h"
#include "L32_VarValue.h"

#include <render.hpp>

#include <string>
#include <unordered_map>
#include <vector>

namespace SDL
{
	struct Point;
}

namespace Little32
{
	struct VarReference;
	struct BigInt;

	struct ConfigObject
	{
		std::unordered_map<std::string, VarValue> settings;

		ConfigObject(std::initializer_list<std::unordered_map<std::string, VarValue>::value_type> _Ilist);

		ConfigObject(std::unordered_map<std::string, VarValue>&& settings);

		ConfigObject();

		bool TryFindValue(std::string_view path, const VarValue*& out) const;

		bool TryFindValueType(std::string_view path, const VarValue*& out, ValueType type) const;

		bool TryFindInt64(std::string_view path, int64_t& out) const;

		bool TryFindUInt64(std::string_view path, uint64_t& out) const;

		bool TryFindInt32(std::string_view path, int32_t& out) const;

		bool TryFindUInt32(std::string_view path, uint32_t& out) const;

		bool TryFindInt16(std::string_view path, int16_t& out) const;

		bool TryFindUInt16(std::string_view path, uint16_t& out) const;

		bool TryFindInt8(std::string_view path, int8_t& out) const;

		bool TryFindUInt8(std::string_view path, uint8_t& out) const;

		bool TryFindIntX(std::string_view path, BigInt& out, size_t bits) const;

		bool TryFindUIntX(std::string_view path, BigInt& out, size_t bits) const;

		bool TryFindInteger(std::string_view path, BigInt& out) const;

		bool TryFindFloat(std::string_view path, float& out) const;

		bool TryFindString(std::string_view path, std::string& out) const;

		bool TryFindColour(std::string_view path, SDL::Colour& out) const;

		bool TryFindVector(std::string_view path, SDL::Point& out) const;

		bool TryFindObject(std::string_view path, const ConfigObject*& out) const;

		bool TryFindList(std::string_view path, const std::vector<VarValue>*& out) const;

		bool TryFindBool(std::string_view path, bool& out) const;

		const VarValue& FindValue(std::string_view path) const;

		const VarValue& FindValueType(std::string_view path, ValueType type) const;

		bool operator==(const ConfigObject& other) const;

		bool operator!=(const ConfigObject& other) const;
	};
}

#endif