#include "L32_VarReference.h"

#include "L32_ConfigObject.h"
#include "L32_String.h"
#include "L32_Types.h"
#include "L32_VarValue.h"

namespace Little32
{
	bool VarReference::TryResolveReference(const VarValue* ref_value, const ConfigObject* root, const VarValue*& out) noexcept
	{
		if (state == RESOLVED_REF)
		{
			out = target;
			return true;
		}
		if (state == INVALID_REF) return false;

		state = RESOLVING_REF;

		const VarValue* target_value = ref_value != nullptr ? ref_value->parent : nullptr;
		std::string cur_path = "";

		if (is_root_reference)
		{
			target_value = nullptr;
			cur_path = "/"; // A second slash will be added inside the loop
		}

		for (const auto& name : names)
		{
			if (!cur_path.empty()) cur_path += '/';
			cur_path += name;

			if (name == "..")
			{
				if (target_value == nullptr) return false;
				target_value = target_value->parent;
			}
			else if (target_value == nullptr)
			{
				if (root->settings.count(name) == 0) return false;
				target_value = &root->settings.at(name);
			}
			else if (target_value->type == OBJECT_VAR)
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
				target_value = target_value->reference_value->ResolveReference(target_value, root);
			}
		}

		if (target_value == nullptr) return false;

		target = target_value;
		state = RESOLVED_REF;

		out = target_value;
		return true;
	}
	const VarValue* VarReference::ResolveReference(const VarValue* ref_value, const ConfigObject* root)
	{
		if (state == RESOLVED_REF) return target;
		if (state == INVALID_REF) throw std::runtime_error("Cannot resolve empty reference");

		state = RESOLVING_REF;

		const VarValue* target_value = ref_value != nullptr ? ref_value->parent : nullptr;
		std::string cur_path = "";

		if (is_root_reference)
		{
			target_value = nullptr;
			cur_path = "/"; // A second slash will be added inside the loop
		}

		for (const auto& name : names)
		{
			if (!cur_path.empty()) cur_path += '/';
			cur_path += name;

			if (name == "..")
			{
				if (target_value == nullptr) throw std::runtime_error("Cannot use backreference at root level: <" + cur_path + ">");
				target_value = target_value->parent;
			}
			else if (target_value == nullptr)
			{
				if (root->settings.count(name) == 0) throw std::runtime_error("Could not reference path: <" + cur_path + ">");
				target_value = &root->settings.at(name);
			}
			else if (target_value->type == OBJECT_VAR)
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
				target_value = target_value->reference_value->ResolveReference(target_value, root);
			}
		}

		if (target_value == nullptr) throw std::runtime_error("Reference resolved to root: <" + cur_path + ">");

		target = target_value;
		state = RESOLVED_REF;

		return target_value;
	}
}