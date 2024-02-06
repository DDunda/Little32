#pragma once

#ifndef L32_Types_h_
#define L32_Types_h_

#include<cstdint>
#include<map>

namespace Little32
{
	typedef uint32_t word;
	typedef uint8_t byte;

	enum Device_ID
	{
		NULL_DEVICE = 0,
		COMPUTERINFO_DEVICE = 1,
		ROM_DEVICE = 2,
		RAM_DEVICE = 3,
		CHARDISPLAY_DEVICE = 4,
		COLOURCHARDISPLAY_DEVICE = 5,
		KEYBOARD_DEVICE = 6
	};

	enum ValueType
	{
		  UNKNOWN_VAR,
		   OBJECT_VAR,
		   STRING_VAR, // "([a-zA-Z0-9!@#$%^&*()~`\-_=+';:,<.>/? \t]|\\([nrftv\\"]|x[0-9a-fA-F]{1,2}))*"
		  INTEGER_VAR, // ([1-9]\d*|0x[0-9a-fA-F]+|0b[01]+|0[0-7]+|0)
		    FLOAT_VAR, // ([1-9]\d*|0)\.(\d*[1-9]|0)?(e[-+]?([1-9]\d*|0))?
		  BOOLEAN_VAR,
		   VECTOR_VAR, // (\d+,\d+)
		   COLOUR_VAR, // #[0-9a-fA-F]{1,6}
		     LIST_VAR,
		REFERENCE_VAR  // <(\/\/)?(([0-9a-zA-Z_]+|\.\.)\/)*([0-9a-zA-Z_]+|\.\.)>
	};

	static const std::map<ValueType, const char*> VALUETYPE_NAMES
	{
		{ UNKNOWN_VAR, "unknown" },
		{ OBJECT_VAR, "object" },
		{ STRING_VAR, "string" },
		{ INTEGER_VAR, "integer" },
		{ FLOAT_VAR, "float" },
		{ BOOLEAN_VAR, "boolean" },
		{ VECTOR_VAR, "vector" },
		{ COLOUR_VAR, "colour" },
		{ LIST_VAR, "list" },
		{ REFERENCE_VAR, "reference" }
	};
}

#endif