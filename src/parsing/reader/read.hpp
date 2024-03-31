#pragma once

#include <algorithm>
#include <cmath>
#include <fstream>
#include <vector>

#include "../cursorDef.hpp"
namespace reader {
	template<typename T>
	T ReadStruct(FileCursor cursor) {
		T data{};

		std::memcpy(&data, *cursor, sizeof(T));

		*cursor += sizeof(data);
		return data;
	}

	char ReadByte(FileCursor cursor) {
		char ret = *reinterpret_cast<char*>(*cursor);
		*cursor += 1;
		return ret;
	}

	std::uint16_t ReadUshort(FileCursor cursor) {
		std::uint16_t ret = *reinterpret_cast<uint16_t*>(*cursor);
		*cursor += sizeof(std::uint16_t);
		return ret;
	}

	int ReadInt(FileCursor cursor) {
		int ret = *reinterpret_cast<int*>(*cursor);
		*cursor += sizeof(int);
		return ret;
	}

	std::uint32_t ReadUint(FileCursor cursor) {
		std::uint32_t ret = *reinterpret_cast<std::uint32_t*>(*cursor);
		*cursor += sizeof(std::uint32_t);
		return ret;
	}

	std::uint64_t ReadUint64(FileCursor cursor) {
		std::uint64_t ret = *reinterpret_cast<std::uint64_t*>(*cursor);
		*cursor += sizeof(std::uint64_t);
		return ret;
	}

	bool ByteHasFlag(char byte, char flag) {
		return (byte & flag) == flag;
	}

	int ReadVlqInt32(FileCursor cursor) {
		// FUCK THIS SHIT AAAAAAAAAAAAAAAAA
		// I WISH I HAD CSHARP'S SHIT SO I COULD JUST COPYPASTE SEBEROTH'S CODE
		// INSTEAD OF REWRITING ALL THIS SHIT

		char b = ReadByte(cursor);
		const auto isNegative = ByteHasFlag(b, 0b10000000);

		auto value = b & 0b00111111;

		if (ByteHasFlag(b, 0b01000000)) {
			b = ReadByte(cursor);

			value |= (b & 0b01111111) << 6;

			if (ByteHasFlag(b, 0b10000000)) {
				b = ReadByte(cursor);
				value |= (b & 0b01111111) << 13;

				if (ByteHasFlag(b, 0b10000000)) {
					b = ReadByte(cursor);
					value |= (b & 0b01111111) << 20;

					if (ByteHasFlag(b, 0b10000000)) {
						b = ReadByte(cursor);
						value |= (b & 0b01111111) << 27;
					}
				}
			}
		}

		return isNegative ? -value : value;
	}

	std::uint32_t ReadVlqUint32(FileCursor cursor) {
		char b = ReadByte(cursor);

		auto value = static_cast<std::uint32_t>(b) & 0b01111111;

		if (ByteHasFlag(b, 0b10000000)) {
			b = ReadByte(cursor);

			value |= (b & 0b01111111) << 7;

			if (ByteHasFlag(b, 0b10000000)) {
				b = ReadByte(cursor);
				value |= (b & 0b01111111) << 14;

				if (ByteHasFlag(b, 0b10000000)) {
					b = ReadByte(cursor);
					value |= (b & 0b01111111) << 21;

					if (ByteHasFlag(b, 0b10000000)) {
						b = ReadByte(cursor);
						value |= (b & 0b01111111) << 28;
					}
				}
			}
		}

		return value;
	}

	std::wstring ReadLengthPrefixedString(FileCursor cursor) {
		int prefix = ReadVlqInt32(cursor);

		int length = std::abs(int(prefix));

		if (length == 0) {
			return std::wstring{};
		}

		if (prefix > 0) {
			std::wstring ret{};
			for (auto i = 0; i < length; i++) {
				ret.push_back(ReadUshort(cursor));
			}
			return ret;
		}
		else {
			std::wstring ret;
			for (auto i = 0; i < length; i++) {
				ret.push_back(ReadByte(cursor));
			}
			return ret;
		}
	}

	int64_t FindByteSequence(FileCursor cursor, int64_t size, std::string bytes) {
		auto start = reinterpret_cast<char*>(*cursor);
		auto end = start + size;

		auto it = std::search(start, end, bytes.begin(), bytes.end());
		
		if (it == end) {
			return -1ll;
		}

		return it - start;
	}
}