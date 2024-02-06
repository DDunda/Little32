#pragma once

#ifndef L32_BigInt_h_
#define L32_BigInt_h_

#include <cassert>
#include <string_view>
#include <vector>

namespace Little32
{
	struct BigInt
	{
		bool negative = false;
		std::vector<uint64_t> bits = {};

		BigInt();

		BigInt(uint64_t value);

		BigInt(int64_t value);

		BigInt(std::string_view value);

		inline constexpr bool operator==(const BigInt& other) const
		{
			if (negative != other.negative) return false;

			if (bits.size() != other.bits.size()) return false;

			for (size_t i = bits.size(); i--;)
			{
				if (bits[i] != other.bits[i]) return false;
			}

			return true;
		}

		inline constexpr bool operator!=(const BigInt& other) const
		{
			if (negative != other.negative) return true;

			if (bits.size() != other.bits.size()) return true;

			for (size_t i = bits.size(); i--;)
			{
				if (bits[i] != other.bits[i]) return true;
			}

			return false;
		}

		BigInt& operator-=(BigInt other);

		BigInt& operator+=(BigInt other);

		BigInt& operator+=(uint64_t other);

		BigInt& operator=(uint64_t other);

		BigInt& operator=(int64_t other);

		BigInt& shift_left();

		BigInt& shift_right();

		BigInt& operator>>=(uint16_t shift);

		BigInt& operator<<=(uint16_t shift);

		BigInt& operator*=(BigInt other);

		BigInt& operator*=(int64_t other);

		std::string ToStringCheap() const;

		size_t NumBits() const noexcept;

		bool TryToUInt64(uint64_t& out) const noexcept;

		bool TryToUInt32(uint32_t& out) const noexcept;

		bool TryToUInt16(uint16_t& out) const noexcept;

		bool TryToUInt8(uint8_t& out) const noexcept;

		bool TryToUIntX(BigInt& out, size_t bits) const noexcept;

		bool TryToInt64(int64_t& out) const noexcept;

		bool TryToInt32(int32_t& out) const noexcept;

		bool TryToInt16(int16_t& out) const noexcept;

		bool TryToInt8(int8_t& out) const noexcept;

		bool TryToIntX(BigInt& out, size_t bits) const noexcept;
	};
}

#endif