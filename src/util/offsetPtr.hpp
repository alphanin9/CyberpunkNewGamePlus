#pragma once
#include <cstdint>
// Borrowed from https://github.com/psiberx/cp2077-archive-xl/blob/9653e9d2eb07831941533fdff3839fc9bef80c8d/lib/Core/Raw.hpp#L257 as I am very lazy
namespace util
{
template<uintptr_t A, typename T>
class OffsetPtr
{
public:
    using Type = std::conditional_t<std::is_void_v<std::remove_pointer_t<T>>, void*, std::remove_pointer_t<T>>;

    static constexpr uintptr_t offset = A;
    static constexpr bool indirect = std::is_pointer_v<T> && !std::is_void_v<std::remove_pointer_t<T>>;

    constexpr OffsetPtr(uintptr_t aBase)
        : address(aBase + offset)
    {
    }

    constexpr OffsetPtr(const void* aBase)
        : address(reinterpret_cast<uintptr_t>(aBase) + offset)
    {
    }

    [[nodiscard]] inline operator bool() const
    {
        if constexpr (std::is_same_v<Type, bool>)
        {
            return GetValuePtr() && *GetValuePtr();
        }
        else
        {
            return GetValuePtr();
        }
    }

    [[nodiscard]] inline operator Type&() const
    {
        return *GetValuePtr();
    }

    [[nodiscard]] inline operator Type*() const
    {
        return GetValuePtr();
    }

    [[nodiscard]] inline Type* operator->() const
    {
        return GetValuePtr();
    }

    OffsetPtr& operator=(T&& aRhs) const noexcept
    {
        *GetPtr() = aRhs;
        return *this;
    }

    const OffsetPtr& operator=(const T& aRhs) const noexcept
    {
        *GetPtr() = aRhs;
        return *this;
    }

    [[nodiscard]] inline T* GetPtr() const noexcept
    {
        return reinterpret_cast<T*>(GetAddress());
    }

    [[nodiscard]] inline Type* GetValuePtr() const noexcept
    {
        if constexpr (indirect)
        {
            return *GetPtr();
        }
        else
        {
            return GetPtr();
        }
    }

    [[nodiscard]] inline uintptr_t GetAddress() const noexcept
    {
        return address;
    }

    inline static Type* Ptr(const void* aBase)
    {
        return OffsetPtr(aBase).GetValuePtr();
    }

    inline static Type& Ref(const void* aBase)
    {
        return *OffsetPtr(aBase).GetValuePtr();
    }

    inline static uintptr_t Addr(const void* aBase)
    {
        return reinterpret_cast<uintptr_t>(OffsetPtr(aBase).GetValuePtr());
    }

    inline static void Set(const void* aBase, const Type& aValue)
    {
        *OffsetPtr(aBase).GetValuePtr() = aValue;
    }

    inline static void Set(const void* aBase, T& aValue)
    {
        *OffsetPtr(aBase).GetPtr() = aValue;
    }

    uintptr_t address;
};
} // namespace util