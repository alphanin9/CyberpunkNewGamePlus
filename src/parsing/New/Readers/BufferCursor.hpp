#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include <span>

namespace parser::reader
{
struct Cursor
{
    template<typename T>
    T& ReadPrimitive() noexcept
    {
        auto curr = reinterpret_cast<T*>(GetCurrentPosition());

        m_offset += sizeof(T);

        return *curr;
    }

    template<typename T>
    std::span<T> ReadSpan(std::uint32_t aCount) noexcept
    {
        std::span<T> ret(reinterpret_cast<T*>(GetCurrentPosition()), aCount);

        m_offset += static_cast<std::uint32_t>(ret.size_bytes());

        return ret;
    }

    int ReadPackedInt() noexcept;
    Red::CString ReadPackedIntLengthString() noexcept;
    Red::CString ReadKnownLengthString(std::uint32_t aLength) noexcept;
    Red::CName ReadPackedIntLengthName() noexcept;
    Red::CName ReadUnknownLengthName() noexcept;

    void SeekTo(std::uint32_t aNewOffset) noexcept;
    
    std::uintptr_t GetCurrentPosition() const noexcept;
    bool HasRemaining() const noexcept;

    Cursor CreateSubCursor(std::uint32_t aSubCursorSize) noexcept;
    void CopyTo(void* aBuffer, std::uint32_t aSize) noexcept;

    Cursor() = default;
    Cursor(std::uintptr_t aBase, std::uint32_t aSize) noexcept
        : m_position(aBase)
        , m_size(aSize)
        , m_offset(0u)
    {
    }

    Cursor(std::uintptr_t aBase) noexcept
        : m_position(aBase)
        , m_size(std::numeric_limits<std::uint32_t>::max())
        , m_offset(0u)
    {
    }

    std::uintptr_t m_position;
    std::uint32_t m_offset;
    std::uint32_t m_size;
};
}