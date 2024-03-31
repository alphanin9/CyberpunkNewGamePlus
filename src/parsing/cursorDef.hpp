#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>

#include <RED4ext/RED4ext.hpp>

// Dumb and unsafe code
struct FileCursor {
    enum class SeekTo {
        Start,
        End,
        Count
    };

    std::byte* baseAddress;
    std::ptrdiff_t offset;

    // Maybe use in buffer overflow checking, but might get slow
    size_t size;

    FileCursor() = delete;
    FileCursor(std::byte* baseAddress, size_t size) : baseAddress{ baseAddress }, offset{ 0 }, size{ size } {

    }

    template<typename T>
    T readValue() {
        const auto ptr = baseAddress + offset;
        T data = *reinterpret_cast<T*>(ptr);

        offset += sizeof(data);

        return data;
    }

    char readByte() {
        return readValue<char>();
    }

    std::int16_t readShort() {
        return readValue<std::int16_t>();
    }

    std::uint16_t readUShort() {
        return readValue<std::uint16_t>();
    }

    std::int32_t readInt() {
        return readValue<std::int32_t>();
    }

    std::uint32_t readUInt() {
        return readValue<std::uint32_t>();
    }

    std::int64_t readInt64() {
        return readValue<std::int64_t>();
    }

    std::uint64_t readUInt64() {
        return readValue<std::uint64_t>();
    }

    float readFloat() {
        return readValue<float>();
    }

    double readDouble() {
        return readValue<double>();
    }

    RED4ext::TweakDBID readTdbId() {
        return RED4ext::TweakDBID{ readUInt64() };
    }

    RED4ext::CRUID readCruid() {
        auto ret = RED4ext::CRUID{};

        ret.unk00 = readUInt64();

        return ret;
    }

    std::vector<std::byte> readBytes(size_t byteCount) {
        auto ret = std::vector<std::byte>{};

        ret.resize(byteCount);

        memcpy(ret.data(), baseAddress + offset, byteCount);
        offset += byteCount;

        return ret;
    }

    void seekTo(SeekTo direction, std::ptrdiff_t seekOffset = 0) {
        if (direction == SeekTo::Start) {
            offset = seekOffset;
        }
        else if (direction == SeekTo::End) {
            offset = size + seekOffset;
        }
    }

    static bool byteHasFlag(char byte, char flag) {
        return (byte & flag) == flag;
    }

    int readVlqInt32() {
        // FUCK THIS SHIT AAAAAAAAAAAAAAAAA
        // I WISH I HAD CSHARP'S SHIT SO I COULD JUST COPYPASTE SEBEROTH'S CODE
        // INSTEAD OF REWRITING ALL THIS SHIT

        char b = readByte();
        const auto isNegative = byteHasFlag(b, 0b10000000);

        auto value = b & 0b00111111;

        if (byteHasFlag(b, 0b01000000)) {
            b = readByte();

            value |= (b & 0b01111111) << 6;

            if (byteHasFlag(b, 0b10000000)) {
                b = readByte();
                value |= (b & 0b01111111) << 13;

                if (byteHasFlag(b, 0b10000000)) {
                    b = readByte();
                    value |= (b & 0b01111111) << 20;

                    if (byteHasFlag(b, 0b10000000)) {
                        b = readByte();
                        value |= (b & 0b01111111) << 27;
                    }
                }
            }
        }

        return isNegative ? -value : value;
    }

    std::uint32_t readVlqUint32() {
        char b = readByte();

        auto value = static_cast<std::uint32_t>(b) & 0b01111111;

        if (byteHasFlag(b, 0b10000000)) {
            b = readByte();

            value |= (b & 0b01111111) << 7;

            if (byteHasFlag(b, 0b10000000)) {
                b = readByte();
                value |= (b & 0b01111111) << 14;

                if (byteHasFlag(b, 0b10000000)) {
                    b = readByte();
                    value |= (b & 0b01111111) << 21;

                    if (byteHasFlag(b, 0b10000000)) {
                        b = readByte();
                        value |= (b & 0b01111111) << 28;
                    }
                }
            }
        }

        return value;
    }

    std::wstring readLengthPrefixedString() {
        int prefix = readVlqInt32();

        int length = std::abs(int(prefix));

        if (length == 0) {
            return std::wstring{};
        }

        if (prefix > 0) {
            std::wstring ret{};
            for (auto i = 0; i < length; i++) {
                ret.push_back(readUShort());
            }
            return ret;
        }
        else {
            std::wstring ret;
            for (auto i = 0; i < length; i++) {
                ret.push_back(readByte());
            }
            return ret;
        }
    }

    // Theoretically if we don't need a copy we could just do a string_view
    // But let's be safe!
    std::string readString(size_t length) {
        const auto bytes = readBytes(length);

        auto ret = std::string{};
        ret.reserve(length);

        for (auto i : bytes) {
            ret.push_back(static_cast<char>(i));
        }

        return ret;
    }

    std::string readNullTerminatedString() {
        auto ret = std::string{};

        auto curByte = readByte();

        do {
            ret.push_back(curByte);
            curByte = readByte();
        } while (curByte != '\0');

        return ret;
    }

    int64_t findByteSequence(std::string_view bytes) const {
        auto start = reinterpret_cast<char*>(baseAddress + offset);
        auto end = start + size;

        auto it = std::search(start, end, bytes.begin(), bytes.end());

        if (it == end) {
            return -1ll;
        }

        return it - start;
    }
};