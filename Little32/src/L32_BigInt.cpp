#include "L32_BigInt.h"

#include <stdexcept>
#include <string>

namespace Little32
{
	BigInt::BigInt() : bits({}), negative(false) {}

	BigInt::BigInt(uint64_t value) :
		bits({}),
		negative(false)
	{
		if (value == 0) return;

		bits.push_back(value);
	}

	BigInt::BigInt(int64_t value) :
		bits({}),
		negative(false)
	{
		if (value == 0) return;

		if (value < 0)
		{
			negative = true;
			bits.push_back(uint64_t(-value));
		}
		else bits.push_back(uint64_t(value));
	}

	BigInt::BigInt(std::string_view value)
		: negative(false), bits({})
	{
		if (value.empty()) throw std::runtime_error("Expected number for BigInt, got nothing");
		if (value.front() == '+')
		{
			value = value.substr(1);
			if (value.empty()) throw std::runtime_error("Expected number for BigInt, got '+'");
		}
		else if (value.front() == '-')
		{
			negative = true;
			value = value.substr(1);
			if (value.empty()) throw std::runtime_error("Expected number for BigInt, got '-'");
		}

		if (value.size() == 1)
		{
			if (value.front() < '0' || value.front() > '9') throw std::runtime_error("Expected decimal digit for BigInt, got '" + std::to_string(value.front()) + "'");
			if (value.front() == '0')
			{
				negative = false; // 0 is an empty array
			}
			else
			{
				bits.push_back(value.front() - '0');
			}
			return;
		}

		size_t i;

		if (value.front() != '0') // Decimal
		{
			if ((i = value.find_first_not_of("0123456789")) != std::string::npos) throw std::runtime_error("Expected decimal digit for BigInt, got '" + std::to_string(value[i]) + "'");

			bits.push_back(value.front() - '0');
			value = value.substr(1);

			while (!value.empty())
			{
				*this *= 10;
				*this += value.front() - '0';
				value = value.substr(1);
			}

			return;
		}

		value = value.substr(1); // Remove 0 marker
		// Due to prior checks we do not have to check here if value is empty
		if (value.front() == 'x') // Hexadecimal
		{
			value = value.substr(1); // Remove x marker
			if (value.empty()) throw std::runtime_error("Expected number for BigInt, got '0x'");

			if ((i = value.find_first_not_of("0123456789abcdefABCDEF")) != std::string::npos) throw std::runtime_error("Expected hexadecimal digit for BigInt, got '" + std::to_string(value[i]) + "'");

			if ((i = value.find_first_not_of('0')) == std::string::npos)
			{
				negative = false;
				return;
			}

			value = value.substr(i);

			switch (value.front())
			{
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				bits.push_back(value.front() - '0');
				break;
			case 'a':
			case 'b':
			case 'c':
			case 'd':
			case 'e':
			case 'f':
				bits.push_back(10 + (value.front() - 'a'));
				break;
			case 'A':
			case 'B':
			case 'C':
			case 'D':
			case 'E':
			case 'F':
				bits.push_back(10 + (value.front() - 'A'));
				break;
			}

			value = value.substr(1);

			while (!value.empty());
			{
				*this <<= 4;
				switch (value.front())
				{
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
					bits[0] |= value.front() - '0';
					break;
				case 'a':
				case 'b':
				case 'c':
				case 'd':
				case 'e':
				case 'f':
					bits[0] |= 10 + (value.front() - 'a');
					break;
				case 'A':
				case 'B':
				case 'C':
				case 'D':
				case 'E':
				case 'F':
					bits[0] |= 10 + (value.front() - 'A');
					break;
				}
				value = value.substr(1);
			}
		}
		else if (value.front() == 'b') // Binary
		{
			value = value.substr(1); // Remove b marker
			if (value.empty()) throw std::runtime_error("Expected number for BigInt, got '0b'");

			if ((i = value.find_first_not_of("01")) != std::string::npos) throw std::runtime_error("Expected binary digit for BigInt, got '" + std::to_string(value[i]) + "'");

			if ((i = value.find_first_not_of('0')) == std::string::npos)
			{
				negative = true;
				return;
			}

			value = value.substr(i + 1);

			bits.push_back(1);

			while (!value.empty());
			{
				shift_left();
				bits[0] |= value.front() - '0';
				value = value.substr(1);
			}
		}
		else // Octal
		{
			if ((i = value.find_first_not_of("01234567")) != std::string::npos) throw std::runtime_error("Expected octal digit for BigInt, got '" + std::to_string(value[i]) + "'");

			if ((i = value.find_first_not_of('0')) == std::string::npos)
			{
				negative = false;
				return;
			}

			value = value.substr(i);

			bits.push_back(value.front() - '0');

			value = value.substr(1);

			while (!value.empty())
			{
				*this <<= 3;
				bits[0] |= value.front() - '0';
				value = value.substr(1);
			}
		}
	}

	BigInt& BigInt::operator-=(BigInt other)
	{
		// a - 0 == a
		if (other.bits.size() == 0) return *this;

		// 0 - b == -b
		if (bits.size() == 0)
		{
			std::swap(bits, other.bits);
			negative = !other.negative;
			return *this;
		}

		if (negative != other.negative)
		{
			if (negative)
			{
				// (-a) - (+b) == -((+a) + (+b))
				negative = false;
				*this += other;
				negative = true;
			}
			else
			{
				// (+a) - (-b) == (+a) + (+b)
				other.negative = false;
				*this += other;
			}
			return *this;
		}

		// (+a) - (+b) or (-a) - (-b)

		bool borrow = false;
		size_t i = 0;

		if (other.bits.size() > bits.size())
		{
			std::swap(bits, other.bits);
			negative = !negative;
		}

		if (bits.size() == other.bits.size())
		{
			for (; i < bits.size(); i++)
			{
				uint64_t sum = bits[i] + (~(other.bits[i] + borrow) + 1);

				borrow = sum > bits[i] || (borrow && sum == bits[i]);

				bits[i] = sum;
			}

			if (borrow)
			{
				negative = !negative;
				for (i = 0; i < bits.size(); i++)
				{
					if (bits[i] > 0)
					{
						bits[i] = (~bits[i]) + 1; // Two's complement
						break; // No more carrying, so only flip remaining bits
					}
					// Two's complement of zero is zero...
				}
				for (; i < bits.size(); i++)
				{
					bits[i] = ~bits[i];
				}
			}

			// Remove empty blocks
			while (bits.back() == 0);
			{
				bits.pop_back();
				if (bits.empty())
				{
					negative = false;
					return *this;
				}
			}
		}
		else
		{
			for (; i < other.bits.size(); i++)
			{
				uint64_t sum = bits[i] + (~(other.bits[i] + borrow) + 1);

				borrow = sum > bits[i] || (borrow && sum == bits[i]);

				bits[i] = sum;
			}

			if (!borrow) return *this;

			for (; i < bits.size() - 1; i++)
			{
				if (bits[i] > 0)
				{
					--bits[i];
					return *this;
				}

				bits[i] = ~uint64_t(0); // Consider 0b1 00000000 - 1 -> 0b0 11111111
			}

			--bits.back();
			if (bits.back() == 0) bits.pop_back();
		}

		return *this;
	}

	BigInt& BigInt::operator+=(BigInt other)
	{
		// a + 0 == a
		if (other.bits.empty()) return *this;

		// 0 + b == b
		if (bits.empty())
		{
			std::swap(bits, other.bits);
			negative = other.negative;
			return *this;
		}

		if (negative != other.negative)
		{
			if (negative)
			{
				// (-a) + (+b) == (+b) - (+a)
				std::swap(bits, other.bits);
				negative = false;
				*this -= other;
			}
			else
			{
				// (+a) + (-b) == (+a) - (+b)
				other.negative = false;
				*this -= other;
			}
			return *this;
		}

		// (+a) + (+b) or (-a) + (-b)

		bool carry = false;
		size_t i = 0;

		if (other.bits.size() > bits.size()) std::swap(bits, other.bits);

		if (bits.size() == other.bits.size())
		{
			for (; i < bits.size(); i++)
			{
				uint64_t sum = bits[i] + other.bits[i] + carry;

				carry = sum < bits[i] || sum < other.bits[i];

				bits[i] = sum;
			}

			if (!carry) return *this;
		}
		else
		{
			for (; i < other.bits.size(); i++)
			{
				uint64_t sum = bits[i] + other.bits[i] + carry;

				carry = sum < bits[i] || sum < other.bits[i];

				bits[i] = sum;
			}

			if (!carry) return *this;

			for (; i < bits.size(); i++)
			{
				if (bits[i] != ~uint64_t(0))
				{
					++bits[i];
					return *this;
				}
				bits[i] = 0;
			}
		}

		bits.push_back(1);

		return *this;
	}

	BigInt& BigInt::operator+=(uint64_t other)
	{
		if (other == 0) return *this;

		if (bits.empty())
		{
			bits.push_back(other);
			return *this;
		}

		uint64_t sum = bits.front() + other;

		const bool carry = sum < bits.front() || sum < other;

		bits.front() = sum;

		if (carry == 0) return *this;

		for (auto it = ++bits.begin(); it != bits.end(); ++it)
		{
			if (*it != 0xFFFFFFFFFFFFFFFF)
			{
				(*it)++;
				return *this;
			}

			*it = 0;
		}

		bits.push_back(1);

		return *this;
	}

	BigInt& BigInt::operator=(uint64_t other)
	{
		bits.clear();
		negative = false;
		if (other == 0) return *this;

		bits.push_back(other);

		return *this;
	}

	BigInt& BigInt::operator=(int64_t other)
	{
		bits.clear();
		negative = false;
		if (other == 0) return *this;

		if (other < 0)
		{
			negative = true;
			bits.push_back(uint64_t(-other));
		}
		else bits.push_back(uint64_t(other));

		return *this;
	}

	BigInt& BigInt::shift_left()
	{
		uint64_t carry = 0;

		for (auto& bit : bits)
		{
			if (bit == 0)
			{
				bit = carry;
				carry = 0;
				continue;
			}

			const uint64_t next_carry = bit >> 63;
			bit = (bit << 1) | carry;
			carry = next_carry;
		}
		if (carry) bits.push_back(1);

		return *this;
	}

	BigInt& BigInt::shift_right()
	{
		uint64_t carry = 0;

		if (bits.back() == 1)
		{
			carry = uint64_t(1) << 63;
			bits.pop_back();
		}

		for (auto it = bits.rbegin(); it != bits.rend(); it++)
		{
			if (*it == 0)
			{
				*it = carry;
				carry = 0;
				continue;
			}

			const uint64_t next_carry = *it << 63;
			*it = (*it >> 1) | carry;
			carry = next_carry;
		}

		return *this;
	}

	BigInt& BigInt::operator>>=(uint16_t shift)
	{
		if (bits.empty()) return *this;

		if (shift >= bits.size() * 64)
		{
			bits.clear();
			negative = false;
			return *this;
		}

		if (shift > 64)
		{
			auto begin = bits.cbegin();
			auto end = ++begin;
			shift -= 64;

			while (shift > 64)
			{
				++end;
				shift -= 64;
			}

			bits.erase(begin, end);

			assert(!bits.empty()); // This assertion may turn out false, but whatever
		}

		uint64_t carry = 0;

		if (bits.back() >> shift == 0)
		{
			carry = bits.back() << (64 - shift);
			bits.pop_back();
		}

		for (auto it = bits.rbegin(); it != bits.rend(); ++it)
		{
			const uint64_t new_carry = *it << (64 - shift);
			*it = (*it >> shift) | carry;
			carry = new_carry;
		}

		return *this;
	}

	BigInt& BigInt::operator<<=(uint16_t shift)
	{
		if (bits.empty()) return *this;

		size_t bits_to_add = shift / 64;

		shift -= bits_to_add * 64;

		if (shift != 0)
		{
			uint64_t carry = 0;

			for (auto& bit : bits)
			{
				const uint64_t new_carry = bit >> (64 - shift);
				bit = (bit << shift) | carry;
				carry = new_carry;
			}

			if (carry != 0) bits.push_back(carry);
		}

		while (bits_to_add--)
		{
			bits.insert(bits.begin(), 0);
		}

		return *this;
	}

	BigInt& BigInt::operator*=(BigInt other)
	{
		BigInt c;

		std::swap(c.bits, bits);

		negative = negative != other.negative;

		while (!other.bits.empty())
		{
			if (other.bits[0] & 1) *this += c;

			other.shift_right();
			c.shift_left();
		}

		return *this;
	}

	BigInt& BigInt::operator*=(int64_t other)
	{
		if (other == 0)
		{
			bits = {};
			negative = false;
			return *this;
		}

		if (other < 0)
		{
			other = -other;
			negative = !negative;
		}

		if (other == 1) return *this;

		BigInt c;

		std::swap(c.bits, bits);

		while (other != 0)
		{
			if (other & 1) *this += c;

			other >>= 1;
			c.shift_left();
		}

		return *this;
	}

	std::string BigInt::ToStringCheap() const
	{
		if (bits.size() == 0) return "0";
		if (bits.size() > 1) return (negative ? "-<BigInt: " : "<BigInt: ") + std::to_string(NumBits()) + " bits>";

		return (negative ? "-" : "") + std::to_string(bits.front());
	}

	size_t BigInt::NumBits() const noexcept
	{
		if (bits.empty()) return 0;

		char sig_bits = 64 * (bits.size() - 1);
		uint64_t num = bits.back();

		assert(num != 0); // The most significant chunk should never be empty

		do
		{
			num >>= 1;
			sig_bits++;
		} while (num > 0);

		return sig_bits;
	}

	bool BigInt::TryToUInt64(uint64_t& out) const noexcept
	{
		if (bits.size() == 0)
		{
			out = 0;
			return true;
		}

		if (bits.size() != 1 || negative) return false;

		out = bits[0];
		return true;
	}

	bool BigInt::TryToUInt32(uint32_t& out) const noexcept
	{
		if (bits.size() == 0)
		{
			out = 0;
			return true;
		}

		if (bits.size() != 1 || negative || bits[0] & ~uint64_t(0xFFFFFFFF)) return false;

		out = bits[0];
		return true;
	}

	bool BigInt::TryToUInt16(uint16_t& out) const noexcept
	{
		if (bits.size() == 0)
		{
			out = 0;
			return true;
		}

		if (bits.size() != 1 || negative || bits[0] & ~uint64_t(0xFFFF)) return false;

		out = bits[0];
		return true;
	}

	bool BigInt::TryToUInt8(uint8_t& out) const noexcept
	{
		if (bits.size() == 0)
		{
			out = 0;
			return true;
		}

		if (bits.size() != 1 || negative || bits[0] & ~uint64_t(0xFF)) return false;

		out = bits[0];
		return true;
	}

	bool BigInt::TryToUIntX(BigInt& out, size_t bits) const noexcept
	{
		if (this->bits.size() == 0)
		{
			out = uint64_t(0);
			return true;
		}

		if (negative || NumBits() > bits) return false;

		out = *this;
		return true;
	}

	bool BigInt::TryToInt64(int64_t& out) const noexcept
	{
		if (bits.size() == 0)
		{
			out = 0;
			return true;
		}

		if (bits.size() != 1) return false;

		if (negative)
		{
			if (bits[0] > (1 << 63)) return false;
			out = -static_cast<int64_t>(bits[0]);
		}
		else
		{
			if (bits[0] & (1 << 63)) return false;
			out = bits[0];
		}
		return true;
	}

	bool BigInt::TryToInt32(int32_t& out) const noexcept
	{
		if (bits.size() == 0)
		{
			out = 0;
			return true;
		}

		if (bits.size() != 1) return false;

		if (negative)
		{
			if (bits[0] > 0x80000000) return false;
			out = -static_cast<int32_t>(bits[0]);
		}
		else
		{
			if (bits[0] & ~uint64_t(0x7FFFFFFF)) return false;
			out = bits[0];
		}
		return true;
	}

	bool BigInt::TryToInt16(int16_t& out) const noexcept
	{
		if (bits.size() == 0)
		{
			out = 0;
			return true;
		}

		if (bits.size() != 1) return false;

		if (negative)
		{
			if (bits[0] > 0x8000) return false;
			out = -static_cast<int16_t>(bits[0]);
		}
		else
		{
			if (bits[0] & ~uint64_t(0x7FFF)) return false;
			out = bits[0];
		}
		return true;
	}

	bool BigInt::TryToInt8(int8_t& out) const noexcept
	{
		if (bits.size() == 0)
		{
			out = 0;
			return true;
		}

		if (bits.size() != 1) return false;

		if (negative)
		{
			if (bits[0] > 0x80) return false;
			out = -static_cast<int8_t>(bits[0]);
		}
		else
		{
			if (bits[0] & ~uint64_t(0x7F)) return false;
			out = bits[0];
		}
		return true;
	}

	bool BigInt::TryToIntX(BigInt& out, size_t bits) const noexcept
	{
		if (this->bits.size() == 0)
		{
			out = uint64_t(0);
			return true;
		}

		if (NumBits() > bits) return false;

		out = *this;
		return true;
	}
}