#pragma once
#include <array>
#include <cstddef>
#include <vector>

struct BufferWriter {
	std::vector<std::byte> buffer;

	void writeInt(int value) {
		std::array<std::byte, sizeof(int)> container{};
		*reinterpret_cast<int*>(container.data()) = value;

		buffer.insert(buffer.end(), container.begin(), container.end());
	}

	void writeUInt(std::uint32_t value) {
		std::array<std::byte, sizeof(int)> container{};
		*reinterpret_cast<uint32_t*>(container.data()) = value;

		buffer.insert(buffer.end(), container.begin(), container.end());
	}

	void writeEmpty(size_t count) {
		buffer.insert(buffer.end(), count, std::byte{ 0 });
	}
};