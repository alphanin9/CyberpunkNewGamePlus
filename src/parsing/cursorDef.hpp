#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <print>
#include <span>
#include <string>

#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include <intrin.h>

struct FileCursor;

template<typename ReadableClass>
concept IsClassReadable = requires(ReadableClass cl, FileCursor& aCursor) {
    {
        ReadableClass::FromCursor(aCursor)
    } -> std::same_as<ReadableClass>;
};

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

    // NOTE: bad, no good, very bad... Is needed to keep default ctor for package reader
    FileCursor() = default;
    FileCursor(std::byte* baseAddress, size_t size) : baseAddress{ baseAddress }, offset{ 0 }, size{ size } {

    }

    // Constructor for cases you don't need to know the size because you'll never seek backwards
    FileCursor(std::byte* aBaseAddress)
        : baseAddress(aBaseAddress)
        , offset(0)
        , size(std::numeric_limits<std::ptrdiff_t>::max())
    {
    
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

    void seekTo(std::ptrdiff_t aSeekOffset = 0)
    {
        offset = aSeekOffset;
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
        std::wstring ret{};

        if (length > 0) {
            if (prefix > 0) {
                for (auto i = 0; i < length; i++) {
                    ret.push_back(readUShort());
                }
            }
            else {
                for (auto i = 0; i < length; i++) {
                    ret.push_back(readByte());
                }
            }
        }

        return ret;
    }

    // WILL ONLY WORK ON ANSI STRINGS!!!
    std::string OptimizedReadLengthPrefixedANSI()
    {
        int prefix = readVlqInt32();
        std::size_t length = std::abs(int(prefix));

        if (length > 0)
        {
            if (prefix > 0)
            {
                offset += length * sizeof(std::uint16_t);
                return "";
            }
            else
            {
                std::string ret(reinterpret_cast<char*>(GetCurrentPtr()), length);
                offset += length;

                return ret;
            }
        }
    }

    std::string ReadLengthPrefixedANSI()
    {
        auto buffer = readLengthPrefixedString();

        auto lengthNeeded = WideCharToMultiByte(CP_UTF8, 0, &buffer[0], buffer.size(), nullptr, 0, nullptr, nullptr);
        
        std::string ret(lengthNeeded, char{});

        WideCharToMultiByte(CP_UTF8, 0, &buffer[0], buffer.size(), &ret[0], lengthNeeded, nullptr, nullptr);
        
        return ret;
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

    std::string_view ReadStringView()
    {
        std::string_view data = reinterpret_cast<const char*>(baseAddress + offset);
        offset += data.length();
        return data;
    }

    // This does not actually own the memory, so it'll get fucked the moment the container deallocates...
    // No real problem
    std::string_view ReadStringView(std::size_t aLength)
    {
        std::string_view data{reinterpret_cast<const char*>(baseAddress + offset), aLength};
        offset += aLength;
        return data;
    }

    Red::CName ReadCName()
    {
        auto dataPtr = ReadStringView().data();

        Red::CNamePool::Add(dataPtr);

        return dataPtr;
    }

    Red::CName ReadCNameHash()
    {
        return Red::CName{readUInt64()};
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

    std::ptrdiff_t getRemaining() const {
        return size - offset;
    }

    template<typename ReadableClass>
    requires IsClassReadable<ReadableClass>
    ReadableClass ReadClass()
    {
        return ReadableClass::FromCursor(*this);
    }

    template<typename ReadableClass>
    requires IsClassReadable<ReadableClass>
    std::vector<ReadableClass> ReadMultipleClasses(std::size_t aCount)
    {
        std::vector<ReadableClass> ret{};
        ret.reserve(aCount);

        for (std::size_t i = 0u; i < aCount; i++)
        {
            ret.push_back(ReadClass<ReadableClass>());
        }

        return ret;
    }

    // For types not requiring special handling!
    // Should add a concept? Nah, std::is_pod and std::is_trivial does not work on TDBIDs and stuff
    template<typename T>
    std::vector<T> ReadMultiplePOD(std::size_t aCount)
    {
        std::vector<T> ret{};
        ret.reserve(aCount);

        for (auto i = 0u; i < aCount; i++)
        {
            ret.push_back(readValue<T>());
        }

        return ret;
    }

    // Returns a cursor pointing to current offset with size = aSize, increments size
    FileCursor CreateSubCursor(std::size_t aSize)
    {
        const FileCursor ret{baseAddress + offset, aSize};

        offset += aSize;

        return ret;
    }

    template<typename T>
    inline static FileCursor FromVec(std::vector<T>& aVec)
    {
        return FileCursor{aVec.data(), aVec.size()};
    }

    std::byte* GetCurrentPtr() const
    {
        return baseAddress + offset;
    }

    void CopyTo(void* aBuffer, std::size_t aSize)
    {
        memcpy(aBuffer, GetCurrentPtr(), aSize);
        offset += aSize;
    }

    void InsertTo(std::vector<std::byte>& aVec, std::size_t aSize)
    {
        auto begin = GetCurrentPtr();

        aVec.insert(aVec.end(), begin, begin + aSize);
        
        offset += aSize;
    }

    template<typename T>
    std::span<T> ReadSpan(std::size_t aCount)
    {
        std::span<T> ret(reinterpret_cast<T*>(GetCurrentPtr()), aCount);

        offset += ret.size_bytes();

        return ret;
    }
};