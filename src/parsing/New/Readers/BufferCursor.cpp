#include "BufferCursor.hpp"

using namespace parser::reader;
using namespace Red;

void Cursor::CopyTo(void* aBuffer, std::uint32_t aSize) noexcept
{
    std::copy_n(reinterpret_cast<char*>(GetCurrentPosition()), aSize, reinterpret_cast<char*>(aBuffer));
    m_offset += aSize;
}

int Cursor::ReadPackedInt() noexcept
{
    auto b = ReadPrimitive<char>();

    auto ret = b & 0x3F;

    const auto ìsPositive = (b & 0x80) != 0x80;

    if ((b & 0x40) == 0x40)
    {
        b = ReadPrimitive<char>();

        ret |= (b & 0x7F) << 6;

        if ((b & 0x80) == 0x80)
        {
            b = ReadPrimitive<char>();

            ret |= (b & 0x7F) << 13;

            if ((b & 0x80) == 0x80)
            {
                b = ReadPrimitive<char>();

                ret |= (b & 0x7F) << 20;

                if ((b & 0x80) == 0x80)
                {
                    b = ReadPrimitive<char>();

                    ret |= (b & 0x7F) << 27;
                }
            }
        }
    }

    if (ìsPositive)
    {
        return ret;
    }

    return -ret;
}

CString Cursor::ReadPackedIntLengthString() noexcept
{
    auto prefix = ReadPackedInt();

    std::uint32_t len = std::abs(prefix);

    if (len > 0)
    {
        if (prefix > 0)
        {
            m_offset += len * sizeof(wchar_t);
            return "";
        }
        else
        {
            CString ret(reinterpret_cast<char*>(GetCurrentPosition()), len);
            m_offset += len;

            return ret;
        }
    }

    return "";
}

CString Cursor::ReadKnownLengthString(std::uint32_t aLength) noexcept
{
    CString ret(reinterpret_cast<char*>(GetCurrentPosition()), aLength);
    m_offset += aLength;

    return ret;
}

CName Cursor::ReadPackedIntLengthName() noexcept
{
    auto prefix = ReadPackedInt();

    std::uint32_t len = std::abs(prefix);

    if (len > 0)
    {
        if (prefix > 0)
        {
            m_offset += len * sizeof(wchar_t);
            return "";
        }
        else
        {
            CName ret(reinterpret_cast<char*>(GetCurrentPosition()), len);
            m_offset += len;

            return ret;
        }
    }

    return "";
}

CName Cursor::ReadUnknownLengthName() noexcept
{
    auto view = std::string_view{reinterpret_cast<char*>(GetCurrentPosition())};

    m_offset += view.size();

    return CNamePool::Add(view.data());
}

void Cursor::SeekTo(std::uint32_t aNewOffset) noexcept
{
    m_offset = aNewOffset;
}

uintptr_t Cursor::GetCurrentPosition() const noexcept
{
    return m_position + m_offset;
}

bool Cursor::HasRemaining() const noexcept
{
    if (m_size == 0u)
    {
        return true;
    }

    return m_offset < m_size;
}

Cursor Cursor::CreateSubCursor(std::uint32_t aSubCursorSize) noexcept
{
    Cursor ret(GetCurrentPosition(), aSubCursorSize);

    m_offset += aSubCursorSize;

    return ret;
}