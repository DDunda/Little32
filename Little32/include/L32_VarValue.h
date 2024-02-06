#pragma once

#ifndef L32_VarValue_h_
#define L32_VarValue_h_

#include "L32_BigInt.h"
#include "L32_Types.h"

#include <render.hpp>

#include <cassert>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace SDL
{
	struct Point;
}

namespace Little32
{
	struct ConfigObject;
	struct ConfigParser;
	struct VarReference;

	struct VarValue
	{	friend ConfigParser;
		friend VarReference;
	private:
		ValueType type = UNKNOWN_VAR;
		VarValue* parent = nullptr;

		union
		{
			char data[16] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
			ConfigObject* object_values;
			std::vector<VarValue>* list_values;
			std::string* string_value;
			BigInt* integer_value;
			float float_value;
			bool boolean_value;
			SDL::Point vector_value;
			SDL::Colour colour_value;
			VarReference* reference_value;
		};

		inline void FreeType() noexcept;

	public:
		size_t original_line_start = 0;
		size_t original_line_end = 0;

		size_t file_pos_start = 0;
		size_t file_pos_end = 0;

		bool operator ==(const VarValue& other) const;

		bool operator !=(const VarValue& other) const;

		inline constexpr explicit operator const ConfigObject& () const
		{
			assert(type == OBJECT_VAR);
			return *object_values;
		}

		inline constexpr explicit operator ConfigObject& ()
		{
			assert(type == OBJECT_VAR);
			return *object_values;
		}

		inline constexpr explicit operator const std::string& () const
		{
			assert(type == STRING_VAR);
			return *string_value;
		}

		inline constexpr explicit operator std::string& ()
		{
			assert(type == STRING_VAR);
			return *string_value;
		}

		inline constexpr explicit operator const BigInt& () const
		{
			assert(type == INTEGER_VAR);
			return *integer_value;
		}

		inline constexpr explicit operator const float& () const
		{
			assert(type == FLOAT_VAR);
			return float_value;
		}

		inline constexpr explicit operator float& ()
		{
			assert(type == FLOAT_VAR);
			return float_value;
		}

		inline constexpr explicit operator const bool& () const
		{
			assert(type == BOOLEAN_VAR);
			return boolean_value;
		}

		inline constexpr explicit operator bool& ()
		{
			assert(type == BOOLEAN_VAR);
			return boolean_value;
		}

		inline constexpr explicit operator const SDL::Point& () const
		{
			assert(type == VECTOR_VAR);
			return vector_value;
		}

		inline constexpr explicit operator SDL::Point& ()
		{
			assert(type == VECTOR_VAR);
			return vector_value;
		}

		inline constexpr explicit operator const SDL::Colour& () const
		{
			assert(type == COLOUR_VAR);
			return colour_value;
		}

		inline constexpr explicit operator SDL::Colour& ()
		{
			assert(type == COLOUR_VAR);
			return colour_value;
		}

		inline constexpr explicit operator const std::vector<VarValue>& () const
		{
			assert(type == LIST_VAR);
			return *list_values;
		}

		inline constexpr explicit operator std::vector<VarValue>& ()
		{
			assert(type == LIST_VAR);
			return *list_values;
		}

		inline constexpr explicit operator const VarReference& () const
		{
			assert(type == REFERENCE_VAR);
			return *reference_value;
		}

		inline constexpr explicit operator VarReference& ()
		{
			assert(type == REFERENCE_VAR);
			return *reference_value;
		}

		VarValue& operator=(const VarValue& other);

		VarValue& operator=(VarValue&& other) noexcept;

		inline const VarValue& operator[](const std::string& key) const;

		inline const VarValue& operator[](const size_t index) const;

		VarValue();

		VarValue(const VarValue& other);

		VarValue(VarValue&& other) noexcept;

		// Takes ownership of the pointer; dont delete it yourself!
		VarValue(ConfigObject* object);

		VarValue(const ConfigObject& object);

		// Takes ownership of the pointer; dont delete it yourself!
		VarValue(std::string* string);

		VarValue(const std::string_view& string);

		explicit VarValue(uint64_t integer);

		explicit VarValue(int64_t integer);

		explicit VarValue(uint32_t integer);

		explicit VarValue(int32_t integer);

		explicit VarValue(uint16_t integer);

		explicit VarValue(int16_t integer);

		explicit VarValue(uint8_t integer);

		explicit VarValue(int8_t integer);

		VarValue(float number);

		explicit VarValue(bool boolean);

		VarValue(const SDL::Point& vector);

		VarValue(const SDL::Colour& colour);

		// Takes ownership of the pointer; dont delete it yourself!
		VarValue(std::vector<VarValue>* list);

		VarValue(const std::vector<VarValue>& list);

		// Takes ownership of the pointer; dont delete it yourself!
		VarValue(VarReference* reference);

		VarValue(const VarReference& reference);

		inline ~VarValue();

		inline constexpr ValueType GetType() const { return type; }

		inline constexpr ConfigObject& GetObjectValue() const noexcept
		{
			assert(type == OBJECT_VAR);

			return *object_values;
		}

		inline constexpr std::string& GetStringValue() const noexcept
		{
			assert(type == STRING_VAR);

			return *string_value;
		}

		inline constexpr BigInt& GetIntegerValue() const noexcept
		{
			assert(type == INTEGER_VAR);

			return *integer_value;
		}

		inline constexpr float GetFloatValue() const noexcept
		{
			assert(type == FLOAT_VAR);

			return float_value;
		}

		inline constexpr bool GetBooleanValue() const noexcept
		{
			assert(type == BOOLEAN_VAR);

			return boolean_value;
		}

		inline constexpr SDL::Point GetVectorValue() const noexcept
		{
			assert(type == VECTOR_VAR);

			return vector_value;
		}

		inline constexpr SDL::Colour GetColourValue() const noexcept
		{
			assert(type == COLOUR_VAR);

			return colour_value;
		}

		inline constexpr std::vector<VarValue>& GetListValues() const noexcept
		{
			assert(type == LIST_VAR);

			return *list_values;
		};

		inline void CopyToThisList(const VarValue& to_copy);

		inline void CopyToThisObject(const std::string& str, const VarValue& to_copy);

		inline constexpr VarValue* GetParent() const { return parent; }

		inline constexpr bool IsParentOf(const VarValue* other) const noexcept
		{
			if (type != OBJECT_VAR && type != LIST_VAR) return false;

			while (other != nullptr)
			{
				if (other->parent == this) return true;
				other = other->parent;
			}

			return false;
		}

		/// <summary>
		/// Attempts to resolve references in this object and its children, if it has them.
		/// This function will not convert references that point to one of their parents,
		/// or form infinite cycles.
		/// </summary>
		/// <param name="root">The base of the tree, used for references that use '//'</param>
		/// <param name="evaluating">The variables currently being resolved;
		/// pass an empty set, or the parents of this variable for a slight perf improvement (?).
		/// On failure this can be used to reconstruct the path to the offending variable.</param>
		/// <returns>Whether this object was fully resolved</returns>
		bool TryResolveNested(ConfigObject* root, std::set<const VarValue*>& evaluating) noexcept;

		/// <summary>
		/// Attempts to resolve references in this object and its children, if it has them, into a new copy.
		/// Information in the out variable will be destroyed if it exists.
		/// This function will not convert references that point to one of their parents,
		/// or form infinite cycles.
		/// </summary>
		/// <param name="root">The base of the tree, used for references that use '//'</param>
		/// <param name="evaluating">The variables currently being resolved;
		/// pass an empty set, or the parents of this variable for a slight perf improvement (?).
		/// On failure this can be used to reconstruct the path to the offending variable.</param>
		/// <returns>Whether this object was fully resolved</returns>
		bool TryCopyResolved(const ConfigObject* root, std::set<const VarValue*>& evaluating, VarValue& out) const noexcept;

		inline constexpr void SetObjectValue(ConfigObject* value)
		{
			if (type != OBJECT_VAR)
			{
				FreeType();
				type = OBJECT_VAR;
			}
			else if (object_values != nullptr)
			{
				if (object_values == value) return;
				delete object_values;
			}
			object_values = value;
		}

		inline constexpr void SetStringValue(std::string* value) noexcept
		{
			if (type != STRING_VAR)
			{
				FreeType();
				type = STRING_VAR;
			}
			else if (string_value != nullptr)
			{
				if (string_value == value) return;
				delete string_value;
			}
			string_value = value;
		}

		inline constexpr void SetIntegerValue(BigInt* value) noexcept
		{
			if (type != INTEGER_VAR)
			{
				FreeType();
				type = INTEGER_VAR;
			}
			else if (integer_value != nullptr)
			{
				if (integer_value == value) return;
				delete integer_value;
			}
			integer_value = value;
		}

		inline constexpr void SetFloatValue(float value) noexcept
		{
			if (type != FLOAT_VAR)
			{
				FreeType();
				type = FLOAT_VAR;
			}
			float_value = value;
		}

		inline constexpr void SetBooleanValue(bool value) noexcept
		{
			if (type != BOOLEAN_VAR)
			{
				FreeType();
				type = BOOLEAN_VAR;
			}
			boolean_value = value;
		}

		inline constexpr void SetVectorValue(SDL::Point value) noexcept
		{
			if (type != VECTOR_VAR)
			{
				FreeType();
				type = VECTOR_VAR;
			}
			vector_value = value;
		}

		inline constexpr void SetColourValue(SDL::Colour value) noexcept
		{
			if (type != COLOUR_VAR)
			{
				FreeType();
				type = COLOUR_VAR;
			}
			colour_value = value;
		}

		inline constexpr void SetListValue(std::vector<VarValue>* value) noexcept
		{
			if (type != LIST_VAR)
			{
				FreeType();
				type = LIST_VAR;
			}
			else if (list_values != nullptr)
			{
				if (list_values == value) return;
				delete list_values;
			}
			list_values = value;

			for (auto& value : *list_values)
			{
				value.parent = this;
			}
		}

		inline constexpr void SetReferenceValue(VarReference* value)
		{
			if (type != REFERENCE_VAR)
			{
				FreeType();
				type = REFERENCE_VAR;
			}
			else if (reference_value != nullptr)
			{
				if (reference_value == value) return;
				delete reference_value;
			}
			reference_value = value;
		}

		std::string ToString(const ConfigObject& root, bool recursive = true, bool newlines = true, std::string tab_str = "    ", std::string tabs = "") const;

		bool TryFindValue(const ConfigObject& root, std::string path, const VarValue*& out) const;

		bool TryFindValueType(const ConfigObject& root, std::string path, const VarValue*& out, ValueType type) const;

		bool TryFindInt64(const ConfigObject& root, std::string path, int64_t& out) const;

		bool TryFindUInt64(const ConfigObject& root, std::string path, uint64_t& out) const;

		bool TryFindInt32(const ConfigObject& root, std::string path, int32_t& out) const;

		bool TryFindUInt32(const ConfigObject& root, std::string path, uint32_t& out) const;

		bool TryFindInt16(const ConfigObject& root, std::string path, int16_t& out) const;

		bool TryFindUInt16(const ConfigObject& root, std::string path, uint16_t& out) const;

		bool TryFindInt8(const ConfigObject& root, std::string path, int8_t& out) const;

		bool TryFindUInt8(const ConfigObject& root, std::string path, uint8_t& out) const;

		bool TryFindIntX(const ConfigObject& root, std::string path, BigInt& out, size_t bits) const;

		bool TryFindUIntX(const ConfigObject& root, std::string path, BigInt& out, size_t bits) const;

		bool TryFindInteger(const ConfigObject& root, std::string path, BigInt& out) const;

		bool TryFindFloat(const ConfigObject& root, std::string path, float& out) const;

		bool TryFindString(const ConfigObject& root, std::string path, std::string& out) const;

		bool TryFindColour(const ConfigObject& root, std::string path, SDL::Colour& out) const;

		bool TryFindVector(const ConfigObject& root, std::string path, SDL::Point& out) const;

		bool TryFindObject(const ConfigObject& root, std::string path, const ConfigObject*& out) const;

		bool TryFindList(const ConfigObject& root, std::string path, const std::vector<VarValue>*& out) const;

		bool TryFindBool(const ConfigObject& root, std::string path, bool& out) const;

		const VarValue& FindValue(const ConfigObject& root, std::string path) const;

		const VarValue& FindValueType(const ConfigObject& root, std::string path, ValueType type) const;
	};
}

#endif