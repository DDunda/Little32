#include "L32_VarValue.h"

#include "L32_BigInt.h"
#include "L32_ConfigObject.h"
#include "L32_String.h"
#include "L32_VarReference.h"

namespace Little32
{
	inline void VarValue::FreeType() noexcept
	{
		switch (type)
		{
		case OBJECT_VAR:
			if (object_values != nullptr) delete object_values;
			break;

		case INTEGER_VAR:
			if (integer_value != nullptr) delete integer_value;
			break;

		case STRING_VAR:
			if (string_value != nullptr) delete string_value;
			break;

		case LIST_VAR:
			if (list_values != nullptr) delete list_values;
			break;

		case REFERENCE_VAR:
			if (reference_value != nullptr) delete reference_value;
			break;
		}

		for (size_t i = 0; i < 16; ++i) data[i] = 0;

		type = UNKNOWN_VAR;
	}
	bool VarValue::operator ==(const VarValue& other) const
	{
		if (type != other.type) return false;

		switch (type)
		{
		case Little32::UNKNOWN_VAR:
			return true;
		case Little32::OBJECT_VAR:
			return *object_values == *other.object_values;
		case Little32::STRING_VAR:
			return *string_value == *other.string_value;
		case Little32::INTEGER_VAR:
			return *integer_value == *other.integer_value;
		case Little32::FLOAT_VAR:
			return float_value == other.float_value;
		case Little32::BOOLEAN_VAR:
			return boolean_value == other.boolean_value;
		case Little32::VECTOR_VAR:
			return vector_value == other.vector_value;
		case Little32::COLOUR_VAR:
			return colour_value.r == other.colour_value.r
				&& colour_value.g == other.colour_value.g
				&& colour_value.b == other.colour_value.b
				&& colour_value.a == other.colour_value.a;
		case Little32::LIST_VAR:
			if (list_values->size() != other.list_values->size()) return false;
			for (size_t i = list_values->size(); i--;)
			{
				if ((*list_values)[i] != (*other.list_values)[i]) return false;
			}
			return true;
		case Little32::REFERENCE_VAR:
			return *reference_value == *other.reference_value;
		}

		return true;
	}

	bool VarValue::operator !=(const VarValue& other) const
	{
		return !(*this == other);
	}

	VarValue& VarValue::operator=(const VarValue& other)
	{
		FreeType();

		original_line_start = other.original_line_start;
		original_line_end = other.original_line_end;

		file_pos_start = other.file_pos_start;
		file_pos_end = other.file_pos_end;

		type = other.type;

		switch (type)
		{
		case OBJECT_VAR:
			object_values = new ConfigObject();
			for (auto& [var_name, value] : other.object_values->settings)
			{
				(object_values->settings[var_name] = value).parent = this;
			}
			break;

		case STRING_VAR:
			string_value = new std::string(*(other.string_value));
			break;

		case INTEGER_VAR:
			integer_value = new BigInt(*other.integer_value);
			break;

		case FLOAT_VAR:
			float_value = other.float_value;
			break;

		case BOOLEAN_VAR:
			boolean_value = other.boolean_value;
			break;

		case VECTOR_VAR:
			vector_value = other.vector_value;
			break;

		case COLOUR_VAR:
			colour_value = other.colour_value;
			break;

		case LIST_VAR:
			list_values = new std::vector<VarValue>();
			for (const auto& value : *other.list_values)
			{
				list_values->push_back(value);
				list_values->back().parent = this;
			}
			break;

		case REFERENCE_VAR:
			reference_value = new VarReference(*other.reference_value);
			break;
		}

		return *this;
	}

	VarValue& VarValue::operator=(VarValue&& other) noexcept
	{
		FreeType();

		original_line_start = other.original_line_start;
		original_line_end = other.original_line_end;

		file_pos_start = other.file_pos_start;
		file_pos_end = other.file_pos_end;

		parent = other.parent;

		type = other.type;
		other.type = UNKNOWN_VAR;

		switch (type)
		{
		case OBJECT_VAR:
			std::swap(object_values, other.object_values);
			for (auto& [var_name, value] : object_values->settings)
			{
				value.parent = this;
			}
			break;

		case STRING_VAR:
			std::swap(string_value, other.string_value);
			break;

		case INTEGER_VAR:
			std::swap(integer_value, other.integer_value);
			break;

		case FLOAT_VAR:
			float_value = other.float_value;
			break;

		case BOOLEAN_VAR:
			boolean_value = other.boolean_value;
			break;

		case VECTOR_VAR:
			vector_value = other.vector_value;
			break;

		case COLOUR_VAR:
			colour_value = other.colour_value;
			break;

		case LIST_VAR:
			std::swap(list_values, other.list_values);
			for (auto& value : *list_values)
			{
				value.parent = this;
			}
			break;

		case REFERENCE_VAR:
			std::swap(reference_value, other.reference_value);
			break;
		}
		return *this;
	}

	inline const VarValue& VarValue::operator[](const std::string& key) const
	{
		assert(type == OBJECT_VAR);
		return object_values->settings.at(key);
	}

	inline const VarValue& VarValue::operator[](const size_t index) const
	{
		assert(type == LIST_VAR);
		return list_values->at(index);
	}

	VarValue::VarValue() { type = UNKNOWN_VAR; }

	VarValue::VarValue(const VarValue& other)
	{
		original_line_start = other.original_line_start;
		original_line_end = other.original_line_end;

		file_pos_start = other.file_pos_start;
		file_pos_end = other.file_pos_end;

		type = other.type;

		switch (type)
		{
		case OBJECT_VAR:
			object_values = new ConfigObject();
			for (auto& [var_name, value] : other.object_values->settings)
			{
				(object_values->settings[var_name] = value).parent = this;
			}
			return;

		case STRING_VAR:
			string_value = new std::string(*(other.string_value));
			return;

		case INTEGER_VAR:
			integer_value = new BigInt(*other.integer_value);
			return;

		case FLOAT_VAR:
			float_value = other.float_value;
			return;

		case BOOLEAN_VAR:
			boolean_value = other.boolean_value;
			return;

		case VECTOR_VAR:
			vector_value = other.vector_value;
			return;

		case COLOUR_VAR:
			colour_value = other.colour_value;
			return;

		case LIST_VAR:
			list_values = new std::vector<VarValue>();
			for (const auto& value : *other.list_values)
			{
				list_values->push_back(value);
				list_values->back().parent = this;
			}
			return;

		case REFERENCE_VAR:
			reference_value = new VarReference(*other.reference_value);
			return;
		}
	}

	VarValue::VarValue(VarValue&& other) noexcept
	{
		original_line_start = other.original_line_start;
		original_line_end = other.original_line_end;

		file_pos_start = other.file_pos_start;
		file_pos_end = other.file_pos_end;

		parent = other.parent;

		type = other.type;
		other.type = UNKNOWN_VAR;

		switch (type)
		{
		case OBJECT_VAR:
			std::swap(object_values, other.object_values);
			for (auto& [var_name, value] : object_values->settings)
			{
				value.parent = this;
			}
			return;

		case STRING_VAR:
			std::swap(string_value, other.string_value);
			return;

		case INTEGER_VAR:
			std::swap(integer_value, other.integer_value);
			return;

		case FLOAT_VAR:
			float_value = other.float_value;
			return;

		case BOOLEAN_VAR:
			boolean_value = other.boolean_value;
			return;

		case VECTOR_VAR:
			vector_value = other.vector_value;
			return;

		case COLOUR_VAR:
			colour_value = other.colour_value;
			return;

		case LIST_VAR:
			std::swap(list_values, other.list_values);
			for (auto& value : *list_values)
			{
				value.parent = this;
			}
			return;

		case REFERENCE_VAR:
			std::swap(reference_value, other.reference_value);
			return;
		}
	}

	// Takes ownership of the pointer; dont delete it yourself!
	VarValue::VarValue(ConfigObject* object)
	{
		type = OBJECT_VAR;
		object_values = object;

		for (auto& [name, value] : object_values->settings)
		{
			value.parent = this;
		}
	}

	VarValue::VarValue(const ConfigObject& object)
	{
		type = OBJECT_VAR;
		object_values = new ConfigObject();

		for (auto& [name, value] : object.settings)
		{
			object_values->settings[name] = value;
			object_values->settings[name].parent = this;
		}
	}

	// Takes ownership of the pointer; dont delete it yourself!
	VarValue::VarValue(std::string* string)
	{
		type = STRING_VAR;
		string_value = string;
	}

	VarValue::VarValue(const std::string_view& string)
	{
		type = STRING_VAR;
		string_value = new std::string(string);
	}

	VarValue::VarValue(uint64_t integer)
	{
		type = INTEGER_VAR;
		integer_value = new BigInt(integer);
	}

	VarValue::VarValue(int64_t integer)
	{
		type = INTEGER_VAR;
		integer_value = new BigInt(integer);
	}

	VarValue::VarValue(uint32_t integer)
	{
		type = INTEGER_VAR;
		integer_value = new BigInt(static_cast<uint64_t>(integer));
	}

	VarValue::VarValue(int32_t integer)
	{
		type = INTEGER_VAR;
		integer_value = new BigInt(static_cast<int64_t>(integer));
	}

	VarValue::VarValue(uint16_t integer)
	{
		type = INTEGER_VAR;
		integer_value = new BigInt(static_cast<uint64_t>(integer));
	}

	VarValue::VarValue(int16_t integer)
	{
		type = INTEGER_VAR;
		integer_value = new BigInt(static_cast<int64_t>(integer));
	}

	VarValue::VarValue(uint8_t integer)
	{
		type = INTEGER_VAR;
		integer_value = new BigInt(static_cast<uint64_t>(integer));
	}

	VarValue::VarValue(int8_t integer)
	{
		type = INTEGER_VAR;
		integer_value = new BigInt(static_cast<int64_t>(integer));
	}

	VarValue::VarValue(float number)
	{
		type = FLOAT_VAR;
		float_value = number;
	}

	VarValue::VarValue(bool boolean)
	{
		type = BOOLEAN_VAR;
		boolean_value = boolean;
	}

	VarValue::VarValue(const SDL::Point& vector)
	{
		type = VECTOR_VAR;
		vector_value = vector;
	}

	VarValue::VarValue(const SDL::Colour& colour)
	{
		type = COLOUR_VAR;
		colour_value = colour;
	}

	// Takes ownership of the pointer; dont delete it yourself!
	VarValue::VarValue(std::vector<VarValue>* list)
	{
		type = LIST_VAR;
		list_values = list;

		for (auto& value : *list_values)
		{
			value.parent = this;
		}
	}

	VarValue::VarValue(const std::vector<VarValue>& list)
	{
		type = LIST_VAR;
		list_values = new std::vector<VarValue>();
		list_values->reserve(list.size());
		for (auto& value : list)
		{
			list_values->push_back(value);
			list_values->back().parent = this;
		}
	}

	// Takes ownership of the pointer; dont delete it yourself!
	VarValue::VarValue(VarReference* reference)
	{
		type = REFERENCE_VAR;
		reference_value = reference;
	}

	VarValue::VarValue(const VarReference& reference)
	{
		type = REFERENCE_VAR;
		reference_value = new VarReference(reference);
	}

	inline VarValue::~VarValue() { FreeType(); }

	inline void VarValue::CopyToThisList(const VarValue& to_copy)
	{
		assert(type == LIST_VAR);

		list_values->push_back(to_copy);
		list_values->back().parent = this;
	}

	inline void VarValue::CopyToThisObject(const std::string& str, const VarValue& to_copy)
	{
		assert(type == OBJECT_VAR);

		(object_values->settings[str] = to_copy).parent = this;
	}

	bool VarValue::TryResolveNested(ConfigObject* root, std::set<const VarValue*>& evaluating) noexcept
	{
		// We are already attempting to resolve this to a literal, so there must a cycle
		if (evaluating.contains(this)) return false;
		for (auto& ref_node : evaluating)
		{
			// Uh oh, we traversed to a parent of a node we are already checking TryToLiteral for
			// This would cause an infinite cycle.
			if (IsParentOf(ref_node)) return false;
		}
		evaluating.insert(this);
		if (type == LIST_VAR)
		{
			for (auto& value : *list_values)
			{
				if (!value.TryResolveNested(root, evaluating)) return false;
			}
		}
		else if (type == OBJECT_VAR)
		{
			for (auto& [name, value] : object_values->settings)
			{
				if (!value.TryResolveNested(root, evaluating)) return false;
			}
		}
		else if (type == REFERENCE_VAR)
		{
			VarReference* const var_ref = reference_value;
			assert(var_ref != nullptr); // This is a reference variable, therefore it should have this
			const VarValue* other;
			if (!var_ref->TryResolveReference(this, root, other)) return false;

			if (evaluating.contains(other)) return false;
			for (auto& ref_node : evaluating)
			{
				if (other->IsParentOf(ref_node)) return false;
			}

			switch (other->type)
			{
			case OBJECT_VAR:
			{
				auto* new_object = new ConfigObject();
				for (const auto& [var_name, value] : other->object_values->settings)
				{
					VarValue& child = (new_object->settings[var_name] = value);
					child.parent = this;

					if (!child.TryResolveNested(root, evaluating))
					{
						delete new_object;
						return false;
					}
				}
				object_values = new_object;
			}
			break;

			case LIST_VAR:
			{
				auto* new_list = new std::vector<VarValue>();
				for (const auto& value : *(other->list_values))
				{
					new_list->push_back(value);
					VarValue& child = new_list->back();
					child.parent = this;

					if (!child.TryResolveNested(root, evaluating))
					{
						delete new_list;
						return false;
					}
				}
				list_values = new_list;
			}
			break;

			case STRING_VAR:
			{ string_value = new std::string(*(other->string_value)); }
			break;

			case INTEGER_VAR:
			{ integer_value = new BigInt(*(other->integer_value)); }
			break;

			case FLOAT_VAR:
			{ float_value = other->float_value; }
			break;

			case BOOLEAN_VAR:
			{ boolean_value = other->boolean_value; }
			break;

			case VECTOR_VAR:
			{ vector_value = other->vector_value; }
			break;

			case COLOUR_VAR:
			{ colour_value = other->colour_value; }
			break;

			default:
				assert(false); // TryResolveReference should never return another reference
			}

			parent = other->parent; // Getting the parent of this copy will be equivalent to getting the parent of an unresolved reference
			type = other->type;
			delete var_ref;
		}

		evaluating.erase(this);
		return true;
	}

	bool VarValue::TryCopyResolved(const ConfigObject* root, std::set<const VarValue*>& evaluating, VarValue& out) const noexcept
	{
		// We are already attempting to resolve this to a literal, so there must a cycle
		if (evaluating.contains(this)) return false;
		for (auto& ref_node : evaluating)
		{
			// Uh oh, we traversed to a parent of a node we are already checking TryToLiteral for
			// This would cause an infinite cycle.
			if (IsParentOf(ref_node)) return false;
		}
		evaluating.insert(this);

		switch (type)
		{
		case LIST_VAR:
		{
			out.SetListValue(new std::vector<VarValue>());
			for (auto& value : *list_values)
			{
				out.list_values->push_back(VarValue());
				if (!value.TryCopyResolved(root, evaluating, out.list_values->back())) return false;
			}
			break;
		}
		case OBJECT_VAR:
		{
			out.SetObjectValue(new ConfigObject());
			for (auto& [name, value] : object_values->settings)
			{
				out.object_values->settings[name] = VarValue();
				if (!value.TryCopyResolved(root, evaluating, out.object_values->settings.at(name))) return false;
			}
			break;
		}
		case REFERENCE_VAR:
		{
			assert(reference_value != nullptr); // This is a reference variable, therefore it should have this
			const VarValue* other;
			if (!reference_value->TryResolveReference(this, root, other)) return false;

			if (evaluating.contains(other)) return false;
			for (auto& ref_node : evaluating)
			{
				if (other->IsParentOf(ref_node)) return false;
			}

			switch (other->type)
			{
			case OBJECT_VAR:
			{
				auto* new_object = new ConfigObject();
				for (const auto& [var_name, value] : other->object_values->settings)
				{
					VarValue& child = (new_object->settings[var_name] = VarValue());

					if (!value.TryCopyResolved(root, evaluating, child))
					{
						delete new_object;
						return false;
					}

					child.parent = &out;
				}
				out.FreeType();
				out.object_values = new_object;
			}
			break;

			case LIST_VAR:
			{
				auto* new_list = new std::vector<VarValue>();
				for (const auto& value : *(other->list_values))
				{
					new_list->push_back(VarValue());
					VarValue& child = new_list->back();

					if (!value.TryCopyResolved(root, evaluating, child))
					{
						delete new_list;
						return false;
					}

					child.parent = &out;
				}
				out.SetListValue(new_list);
			}
			break;

			case STRING_VAR:
				out.SetStringValue(new std::string(*(other->string_value)));
				break;

			case INTEGER_VAR:
				out.SetIntegerValue(new BigInt(*(other->integer_value)));
				break;

			case FLOAT_VAR:
				out.SetFloatValue(other->float_value);
				break;

			case BOOLEAN_VAR:
				out.SetBooleanValue(other->boolean_value);
				break;

			case VECTOR_VAR:
				out.SetVectorValue(other->vector_value);
				break;

			case COLOUR_VAR:
				out.SetColourValue(other->colour_value);
				break;

			default:
				assert(false); // TryResolveReference should never return another reference
			}

			out.parent = other->parent; // Getting the parent of this copy will be equivalent to getting the parent of an unresolved reference
			out.type = other->type;

			break;
		}

		case STRING_VAR:
			out.SetStringValue(new std::string(*string_value));
			break;

		case INTEGER_VAR:
			out.SetIntegerValue(new BigInt(*integer_value));
			break;

		case FLOAT_VAR:
			out.SetFloatValue(float_value);
			break;

		case BOOLEAN_VAR:
			out.SetBooleanValue(boolean_value);
			break;

		case COLOUR_VAR:
			out.SetColourValue(colour_value);
			break;

		case VECTOR_VAR:
			out.SetVectorValue(vector_value);
			break;

		case UNKNOWN_VAR:
			out = VarValue();
			break;
		}

		evaluating.erase(this);
		return true;
	}

	std::string VarValue::ToString(const ConfigObject& root, bool recursive, bool newlines, std::string tab_str, std::string tabs) const
	{
		switch (type)
		{
		case Little32::UNKNOWN_VAR:
			return "None";

		case Little32::OBJECT_VAR:
		{
			if (!recursive) return "Object[" + std::to_string(object_values->settings.size()) + "]";

			if (object_values->settings.empty()) return "{}";

			std::string out = "{";

			const std::string tabs2 = tabs + tab_str;

			if (newlines)
			{
				for (auto& [name, value] : object_values->settings)
				{
					out += '\n' + tabs2 + name + " = " + value.ToString(root, true, true, tab_str, tabs2) + ',';
				}
			}
			else
			{
				for (auto& [name, value] : object_values->settings)
				{
					out += tabs2 + name + " = " + value.ToString(root, true, false, tab_str, tabs2) + ',';
				}
			}

			out.pop_back(); // Remove last comma

			if (newlines) out += '\n';

			out += tabs + '}';

			return out;
		}

		case Little32::STRING_VAR:
			return '"' + *string_value + '"';

		case Little32::INTEGER_VAR:
			return integer_value->ToStringCheap();

		case Little32::FLOAT_VAR:
			return std::to_string(float_value);

		case Little32::BOOLEAN_VAR:
			return boolean_value ? "true" : "false";

		case Little32::VECTOR_VAR:
			return '(' + std::to_string(vector_value.x) + ", " + std::to_string(vector_value.y) + ')';

		case Little32::COLOUR_VAR:
		{
			SDL::Colour c = colour_value;
			std::string out = "#";
			const char hex[16] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F' };

			out += hex[colour_value.r >> 4];
			out += hex[colour_value.r & 15];
			out += hex[colour_value.g >> 4];
			out += hex[colour_value.g & 15];
			out += hex[colour_value.b >> 4];
			out += hex[colour_value.b & 15];
			out += hex[colour_value.a >> 4];
			out += hex[colour_value.a & 15];

			return out;
		}

		case Little32::LIST_VAR:
		{
			if (!recursive) return "List[" + std::to_string(list_values->size()) + "]";

			if (list_values->empty()) return "[]";

			std::string out = "[";

			const std::string tabs2 = tabs + tab_str;

			if (newlines)
			{
				for (auto& value : *list_values)
				{
					out += '\n' + tabs2 + value.ToString(root, true, true, tab_str, tabs2) + ',';
				}
			}
			else
			{
				for (auto& value : *list_values)
				{
					out += tabs2 + value.ToString(root, true, false, tab_str, tabs2) + ',';
				}
			}

			out.pop_back(); // Remove last comma

			if (newlines) out += '\n';

			out += tabs + ']';

			return out;
		}

		case Little32::REFERENCE_VAR:
			if (!recursive) return reference_value->ToString();
			else return reference_value->ToString() + ": " +
				reference_value
				->ResolveReference(this, &root)
				->ToString(root, true, newlines, tab_str, tabs);
		}
	}

	bool VarValue::TryFindValue(const ConfigObject& root, std::string path, const VarValue*& out) const
	{
		out = nullptr;

		path += '/';

		size_t i;

		const VarValue* target_value = this;

		while ((i = path.find_first_of('/')) != std::string::npos)
		{
			std::string name = path.substr(0, i);
			path = path.substr(i + 1);

			if (target_value->type == OBJECT_VAR)
			{
				if (target_value->object_values->settings.count(name) == 0) return false;
				target_value = &target_value->object_values->settings.at(name);
			}
			else if (target_value->type == LIST_VAR) // list
			{
				const size_t list_length = target_value->list_values->size();
				if (list_length == 0) return false;

				int32_t weird_index;
				if (!decToI32(name, weird_index)) return false;
				size_t index;
				if (weird_index < 0)
				{
					if (-weird_index > list_length) return false;
					index = list_length + weird_index;
				}
				else
				{
					if (weird_index >= list_length) return false;
					index = weird_index;
				}

				auto it = target_value->list_values->cbegin();
				std::advance(it, index);
				target_value = &*it;
			}
			else return false;

			if (target_value != nullptr && target_value->type == REFERENCE_VAR)
			{
				if (target_value->reference_value->state == RESOLVING_REF) return false;
				target_value = target_value->reference_value->ResolveReference(target_value, &root);
			}
		}

		out = target_value;
		return true;
	}

	bool VarValue::TryFindValueType(const ConfigObject& root, std::string path, const VarValue*& out, ValueType type) const
	{
		out = nullptr;

		const VarValue* var;

		if (!TryFindValue(root, path, var)) return false;
		if (var->GetType() != type) return false;
		out = var;
		return true;
	}

	bool VarValue::TryFindInt64(const ConfigObject& root, std::string path, int64_t& out) const
	{
		const VarValue* var;

		if (!TryFindValue(root, path, var)) return false;
		if (var->type != INTEGER_VAR) return false;
		return var->integer_value->TryToInt64(out);
	}

	bool VarValue::TryFindUInt64(const ConfigObject& root, std::string path, uint64_t& out) const
	{
		const VarValue* var;

		if (!TryFindValue(root, path, var)) return false;
		if (var->type != INTEGER_VAR) return false;
		return var->integer_value->TryToUInt64(out);
	}

	bool VarValue::TryFindInt32(const ConfigObject& root, std::string path, int32_t& out) const
	{
		const VarValue* var;

		if (!TryFindValue(root, path, var)) return false;
		if (var->type != INTEGER_VAR) return false;
		return var->integer_value->TryToInt32(out);
	}

	bool VarValue::TryFindUInt32(const ConfigObject& root, std::string path, uint32_t& out) const
	{
		const VarValue* var;

		if (!TryFindValue(root, path, var)) return false;
		if (var->type != INTEGER_VAR) return false;
		return var->integer_value->TryToUInt32(out);
	}

	bool VarValue::TryFindInt16(const ConfigObject& root, std::string path, int16_t& out) const
	{
		const VarValue* var;

		if (!TryFindValue(root, path, var)) return false;
		if (var->type != INTEGER_VAR) return false;
		return var->integer_value->TryToInt16(out);
	}

	bool VarValue::TryFindUInt16(const ConfigObject& root, std::string path, uint16_t& out) const
	{
		const VarValue* var;

		if (!TryFindValue(root, path, var)) return false;
		if (var->type != INTEGER_VAR) return false;
		return var->integer_value->TryToUInt16(out);
	}

	bool VarValue::TryFindInt8(const ConfigObject& root, std::string path, int8_t& out) const
	{
		const VarValue* var;

		if (!TryFindValue(root, path, var)) return false;
		if (var->type != INTEGER_VAR) return false;
		return var->integer_value->TryToInt8(out);
	}

	bool VarValue::TryFindUInt8(const ConfigObject& root, std::string path, uint8_t& out) const
	{
		const VarValue* var;

		if (!TryFindValue(root, path, var)) return false;
		if (var->type != INTEGER_VAR) return false;
		return var->integer_value->TryToUInt8(out);
	}

	bool VarValue::TryFindIntX(const ConfigObject& root, std::string path, BigInt& out, size_t bits) const
	{
		const VarValue* var;
		if (!TryFindValue(root, path, var)) return false;
		if (var->type != INTEGER_VAR) return false;
		return var->integer_value->TryToIntX(out, bits);
	}

	bool VarValue::TryFindUIntX(const ConfigObject& root, std::string path, BigInt& out, size_t bits) const
	{
		const VarValue* var;
		if (!TryFindValue(root, path, var)) return false;
		if (var->type != INTEGER_VAR) return false;
		return var->integer_value->TryToUIntX(out, bits);
	}

	bool VarValue::TryFindInteger(const ConfigObject& root, std::string path, BigInt& out) const
	{
		const VarValue* var;

		if (!TryFindValue(root, path, var)) return false;
		if (var->GetType() != INTEGER_VAR) return false;
		out = var->GetIntegerValue();
		return true;
	}

	bool VarValue::TryFindFloat(const ConfigObject& root, std::string path, float& out) const
	{
		const VarValue* var;

		if (!TryFindValue(root, path, var)) return false;
		if (var->GetType() != INTEGER_VAR) return false;
		out = (const float&)*var;
		return true;
	}

	bool VarValue::TryFindString(const ConfigObject& root, std::string path, std::string& out) const
	{
		const VarValue* var;

		if (!TryFindValue(root, path, var)) return false;
		if (var->GetType() != STRING_VAR) return false;
		out = var->GetStringValue();
		return true;
	}

	bool VarValue::TryFindColour(const ConfigObject& root, std::string path, SDL::Colour& out) const
	{
		const VarValue* var;

		if (!TryFindValue(root, path, var)) return false;
		if (var->GetType() != COLOUR_VAR) return false;
		out = var->GetColourValue();
		return true;
	}

	bool VarValue::TryFindVector(const ConfigObject& root, std::string path, SDL::Point& out) const
	{
		const VarValue* var;

		if (!TryFindValue(root, path, var)) return false;
		if (var->GetType() != VECTOR_VAR) return false;
		out = var->GetVectorValue();
		return true;
	}

	bool VarValue::TryFindObject(const ConfigObject& root, std::string path, const ConfigObject*& out) const
	{
		const VarValue* var;

		if (!TryFindValue(root, path, var)) return false;
		if (var->GetType() != OBJECT_VAR) return false;
		out = &(var->GetObjectValue());
		return true;
	}

	bool VarValue::TryFindList(const ConfigObject& root, std::string path, const std::vector<VarValue>*& out) const
	{
		const VarValue* var;

		if (!TryFindValue(root, path, var)) return false;
		if (var->GetType() != LIST_VAR) return false;
		out = &(var->GetListValues());
		return true;
	}

	bool VarValue::TryFindBool(const ConfigObject& root, std::string path, bool& out) const
	{
		const VarValue* var;

		if (!TryFindValue(root, path, var)) return false;
		if (var->GetType() != BOOLEAN_VAR) return false;
		out = var->GetBooleanValue();
		return true;
	}

	const VarValue& VarValue::FindValue(const ConfigObject& root, std::string path) const
	{
		std::string cur_path = "";

		path += '/';

		size_t i = 0;

		const VarValue* target_value = this;

		while ((i = path.find_first_of('/')) != std::string::npos)
		{
			std::string name = path.substr(0, i);
			path = path.substr(i + 1);

			if (!cur_path.empty()) cur_path += '/';
			cur_path += name;

			if (target_value->type == OBJECT_VAR)
			{
				if (target_value->object_values->settings.count(name) == 0) throw std::runtime_error("Could not reference path: <" + cur_path + ">");
				target_value = &target_value->object_values->settings.at(name);
			}
			else if (target_value->type == LIST_VAR) // list
			{
				const size_t list_length = target_value->list_values->size();
				if (list_length == 0) throw std::runtime_error("Attempted to index empty list at: <" + cur_path + ">");

				int32_t weird_index;
				if (!decToI32(name, weird_index)) throw std::runtime_error("Could not deduce list index at: <" + cur_path + ">");
				size_t index;
				if (weird_index < 0)
				{
					if (-weird_index > list_length) throw std::runtime_error("Attempted to reverse index out of bounds at: <" + cur_path + ">");
					index = list_length + weird_index;
				}
				else
				{
					if (weird_index >= list_length) throw std::runtime_error("Attempted to index out of bounds at: <" + cur_path + ">");
					index = weird_index;
				}

				auto it = target_value->list_values->cbegin();
				std::advance(it, index);
				target_value = &*it;
			}
			else throw std::runtime_error("Cannot get child from '" + std::string(VALUETYPE_NAMES.at(target_value->type)) + "' type at: <" + cur_path + ">");

			if (target_value != nullptr && target_value->type == REFERENCE_VAR)
			{
				if (target_value->reference_value->state == RESOLVING_REF) throw std::runtime_error("Reference cycle detected: <" + cur_path + ">");
				target_value = target_value->reference_value->ResolveReference(target_value, &root);
			}
		}

		return *target_value;
	}

	const VarValue& VarValue::FindValueType(const ConfigObject& root, std::string path, ValueType type) const
	{
		const VarValue& value = FindValue(root, path);
		if (value.GetType() != type) throw std::runtime_error("Expected value of '" + std::string(VALUETYPE_NAMES.at(type)) + "' type, got '" + std::string(VALUETYPE_NAMES.at(value.GetType())) + "' type");
		return value;
	}
}