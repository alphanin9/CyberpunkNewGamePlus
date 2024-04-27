#pragma once
#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include <any>
#include <unordered_map>

#include "../../../cursorDef.hpp"

namespace redRTTI
{
struct RTTIValue;

class RTTIClass
{
private:
    std::unordered_map<std::string, RTTIValue> m_innerMap;
    bool m_isFullyLoaded;
public:
    bool IsEmpty() const
    {
        return m_innerMap.empty();
    }

    bool IsFullyLoaded() const
    {
        return m_isFullyLoaded;
    }

    void SetFullyLoaded(bool state)
    {
        m_isFullyLoaded = state;
    }

    RTTIValue& operator[](const std::string& aKey)
    {
        return m_innerMap[aKey];
    }
};

using RTTIArray = std::vector<RTTIValue>;

struct RTTIValue
{
    RED4ext::ERTTIType m_typeIndex;
    RED4ext::CName m_typeName;
    std::any m_value;

    operator std::any() = delete;

    template<typename T>
    T Cast() const
    {
        return std::any_cast<T>(m_value);
    }
    
    // Can't really make this return a reference
    RTTIValue operator[](const std::string& aKey) const
    {
        return AsClass()[aKey];
    }

    template<typename T>
    bool Equals(T value)
    {
        return Cast<T>() == value;
    }

    #define RTTIVALUE_GETTER_DEFINER(aTypeName, aType) \
    aType As##aTypeName() const                                                                                         \
    {                                                                                                                   \
        return Cast<aType>();                                                                                           \
    }                                                                                                                   \

    RTTIVALUE_GETTER_DEFINER(Byte, std::int8_t);
    RTTIVALUE_GETTER_DEFINER(Short, std::int16_t);
    RTTIVALUE_GETTER_DEFINER(Int, std::int32_t);
    RTTIVALUE_GETTER_DEFINER(Int64, std::int64_t);
    RTTIVALUE_GETTER_DEFINER(Ubyte, std::uint8_t);
    RTTIVALUE_GETTER_DEFINER(Ushort, std::uint16_t);
    RTTIVALUE_GETTER_DEFINER(Uint, std::uint32_t);
    RTTIVALUE_GETTER_DEFINER(Uint64, std::uint64_t);
    RTTIVALUE_GETTER_DEFINER(Bool, bool);
    RTTIVALUE_GETTER_DEFINER(Float, float);
    RTTIVALUE_GETTER_DEFINER(Double, double);
    RTTIVALUE_GETTER_DEFINER(CName, Red::CName);
    RTTIVALUE_GETTER_DEFINER(TweakDBID, Red::TweakDBID);
    RTTIVALUE_GETTER_DEFINER(Array, RTTIArray);
    RTTIVALUE_GETTER_DEFINER(Class, RTTIClass);
};

class RTTIReader
{
protected:
    virtual std::int8_t ReadInt8(FileCursor& aCursor)
    {
        return aCursor.readByte();
    }
    virtual std::int16_t ReadInt16(FileCursor& aCursor)
    {
        return aCursor.readShort();
    }
    virtual std::int32_t ReadInt32(FileCursor& aCursor)
    {
        return aCursor.readInt();
    }
    virtual std::int64_t ReadInt64(FileCursor& aCursor)
    {
        return aCursor.readInt64();
    }

    virtual std::uint8_t ReadUInt8(FileCursor& aCursor)
    {
        return aCursor.readValue<std::uint8_t>();
    }
    virtual std::uint16_t ReadUInt16(FileCursor& aCursor)
    {
        return aCursor.readUShort();
    }
    virtual std::uint32_t ReadUInt32(FileCursor& aCursor)
    {
        return aCursor.readUInt();
    }
    virtual std::uint64_t ReadUInt64(FileCursor& aCursor)
    {
        return aCursor.readUInt64();
    }

    virtual float ReadFloat(FileCursor& aCursor)
    {
        return aCursor.readFloat();
    }
    virtual double ReadDouble(FileCursor& aCursor)
    {
        return aCursor.readDouble();
    }
    virtual bool ReadBool(FileCursor& aCursor)
    {
        return aCursor.readValue<bool>();
    }
    virtual std::wstring ReadString(FileCursor& aCursor)
    {
        return aCursor.readLengthPrefixedString();
    }

    virtual Red::TweakDBID ReadTweakDBID(FileCursor& aCursor) = 0;
    virtual Red::NodeRef ReadNodeRef(FileCursor& aCursor) = 0;
    virtual Red::CName ReadCName(FileCursor& aCursor) = 0;

    virtual Red::CName ReadEnum(FileCursor& aCursor, Red::CBaseRTTIType* aType) = 0;
    virtual RTTIArray ReadArray(FileCursor& aCursor, Red::CBaseRTTIType* aType) = 0;

    virtual RTTIValue ReadHandle(FileCursor& aCursor, Red::CBaseRTTIType* aType) = 0;
    virtual RTTIValue ReadWeakHandle(FileCursor& aCursor, Red::CBaseRTTIType* aType) = 0;

    virtual RTTIValue ReadValue(FileCursor& aCursor, Red::CBaseRTTIType* aType) = 0;
    virtual RTTIValue GetDefaultValue(Red::CBaseRTTIType* aType)
    {
        RTTIValue retValue{};

        retValue.m_typeIndex = aType->GetType();
        retValue.m_typeName = aType->GetName();

        using Red::ERTTIType;
        
        if (aType->GetType() == ERTTIType::Name)
        {
            if (aType->GetName() == Red::CName{"CName"})
            {
                retValue.m_value = Red::CName{};
            }
        }
        else if (aType->GetType() == ERTTIType::Fundamental)
        {
            if (aType->GetName() == Red::CName{"Int8"})
            {
                retValue.m_value = std::int8_t{};
            }
            else if (aType->GetName() == Red::CName{"Uint8"})
            {
                retValue.m_value = std::uint8_t{};
            }
            else if (aType->GetName() == Red::CName{"Int16"})
            {
                retValue.m_value = std::int16_t{};
            }
            else if (aType->GetName() == Red::CName{"Uint16"})
            {
                retValue.m_value = std::uint16_t{};
            }
            else if (aType->GetName() == Red::CName{"Int32"})
            {
                retValue.m_value = std::int32_t{};
            }
            else if (aType->GetName() == Red::CName{"Uint32"})
            {
                retValue.m_value = std::uint32_t{};
            }
            else if (aType->GetName() == Red::CName{"Int64"})
            {
                retValue.m_value = std::int64_t{};
            }
            else if (aType->GetName() == Red::CName{"Uint64"})
            {
                retValue.m_value = std::uint64_t{};
            }
            else if (aType->GetName() == Red::CName{"Float"})
            {
                retValue.m_value = float{};
            }
            else if (aType->GetName() == Red::CName{"Double"})
            {
                retValue.m_value = double{};
            }
            else if (aType->GetName() == Red::CName{"Bool"})
            {
                retValue.m_value = false;
            }
        }
        else if (aType->GetType() == ERTTIType::Simple)
        {
            if (aType->GetName() == Red::CName{"TweakDBID"})
            {
                retValue.m_value = Red::TweakDBID{};
            }
            else if (aType->GetName() == Red::CName{"NodeRef"})
            {
                retValue.m_value = Red::NodeRef{};
            }
        }
        else if (aType->GetType() == ERTTIType::Enum)
        {
            auto typeEnum = static_cast<Red::CEnum*>(aType);
            if (typeEnum->hashList.size > 0)
            {
                retValue.m_value = typeEnum->hashList[0];
            }
            else
            {
                retValue.m_value = Red::CName{};
            }
        }
        else if (aType->GetType() == ERTTIType::Array || aType->GetType() == ERTTIType::StaticArray)
        {
            retValue.m_value = RTTIArray{};
        }
        else if (aType->GetType() == ERTTIType::Class)
        {
            auto classType = static_cast<Red::CClass*>(aType);
            RTTIClass defaultValue{};

            Red::DynArray<Red::CProperty*> classProps{};

            classType->GetProperties(classProps);

            for (auto prop : classProps)
            {
                if (prop->flags.isPersistent == 0 && prop->flags.isSavable == 0)
                {
                    continue;
                }

                defaultValue[prop->name.ToString()] = GetDefaultValue(prop->type);
            }

            retValue.m_value = defaultValue;
        }
        else if (aType->GetType() == ERTTIType::Handle || aType->GetType() == ERTTIType::WeakHandle)
        {
            retValue.m_value = int{};
        }
        else if (aType->GetType() == ERTTIType::WeakHandle)
        {
            retValue.m_value = int{};
        }

        if (!retValue.m_value.has_value())
        {
            retValue.m_value = std::string{"Unknown value"};
        }

        return retValue;
    }

    public:
    virtual RTTIClass ReadClass(FileCursor& aCursor, Red::CBaseRTTIType* aType) = 0;
};
}