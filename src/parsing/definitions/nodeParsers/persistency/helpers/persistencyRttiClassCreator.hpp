#pragma once

#include <any>
#include <cassert>
#include <fstream>
#include <print>
#include <unordered_set>
#include <vector>
#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include "../../../../cursorDef.hpp"
#include "../../rttiHelpers/rttiValueReader.hpp"

namespace persistent
{
class PersistentReader : public redRTTI::RTTIReader
{
private:
    virtual Red::TweakDBID ReadTweakDBID(FileCursor& aCursor)
    {
        return aCursor.readTdbId();
	}

	virtual Red::NodeRef ReadNodeRef(FileCursor& aCursor)
    {
        return Red::NodeRef{aCursor.readUInt64()};
	}

	virtual Red::CName ReadCName(FileCursor& aCursor)
    {
        return Red::CName{aCursor.readUInt64()};
	}

	virtual Red::CName ReadEnum(FileCursor& aCursor, Red::CBaseRTTIType* aType)
    {
        auto enumType = static_cast<RED4ext::CEnum*>(aType);
        std::int64_t enumIndexer{};

		switch (enumType->actualSize)
        {
        case (sizeof(std::int8_t)):
            enumIndexer = aCursor.readByte();
            break;
        case (sizeof(std::int16_t)):
            enumIndexer = aCursor.readShort();
            break;
        case (sizeof(std::int32_t)):
            enumIndexer = aCursor.readInt();
            break;
        case (sizeof(std::int64_t)):
            enumIndexer = aCursor.readInt64();
            break;
        default:
            enumIndexer = aCursor.readInt();
            break;
        }
        // No alias support :(
        if (enumIndexer < enumType->hashList.size)
        {
            return enumType->hashList[enumIndexer];
        }

        return RED4ext::CName{"Invalid"};
	}

	virtual redRTTI::RTTIArray ReadArray(FileCursor& aCursor, Red::CBaseRTTIType* aType)
    {
        const auto elementCount = aCursor.readInt();
        redRTTI::RTTIArray retValue{};
        retValue.reserve(elementCount);

        auto innerType = static_cast<RED4ext::CRTTIBaseArrayType*>(aType)->GetInnerType();
		
		for (auto i = 0; i < elementCount; i++)
        {
            retValue.push_back(ReadValue(aCursor, innerType));
		}
		
		return retValue;
	}

    redRTTI::RTTIArray ReadStaticArray(FileCursor& aCursor, Red::CBaseRTTIType* aType)
    {
        const auto elementCount = aCursor.readInt();
        redRTTI::RTTIArray retValue{};
        retValue.reserve(elementCount);

        auto innerType = static_cast<RED4ext::CRTTIStaticArrayType*>(aType)->GetInnerType();
        
        for (auto i = 0; i < elementCount; i++)
        {
            retValue.push_back(ReadValue(aCursor, innerType));
        }

        return retValue;
    }

	virtual redRTTI::RTTIValue ReadHandle(FileCursor& aCursor, Red::CBaseRTTIType* aType)
    {
        auto handleType = static_cast<RED4ext::CRTTIHandleType*>(aType);
        auto innerType = handleType->GetInnerType();

        return ReadValue(aCursor, innerType);
	}

	virtual redRTTI::RTTIValue ReadWeakHandle(FileCursor& aCursor, Red::CBaseRTTIType* aType)
    {
        auto handleType = static_cast<RED4ext::CRTTIWeakHandleType*>(aType);
        redRTTI::RTTIValue ret{};

        ret.m_typeIndex = handleType->GetInnerType()->GetType();
        ret.m_typeName = handleType->GetInnerType()->GetName();
        ret.m_value = aCursor.readInt();

		return ret;
    }

	virtual redRTTI::RTTIValue ReadValue(FileCursor& aCursor, Red::CBaseRTTIType* aType)
    {
		using Red::ERTTIType;
        auto retValue = redRTTI::RTTIValue{};

		retValue.m_typeIndex = aType->GetType();
        retValue.m_typeName = aType->GetName();
        if (aType->GetType() == ERTTIType::Name)
        {
            if (aType->GetName() == RED4ext::CName{"CName"})
            {
                retValue.m_value = ReadCName(aCursor);
                return retValue;
            }
        }
        else if (aType->GetType() == ERTTIType::Fundamental)
        {
            if (aType->GetName() == RED4ext::CName{"Int8"})
            {
                retValue.m_value = ReadInt8(aCursor);
            }
            else if (aType->GetName() == RED4ext::CName{"Uint8"})
            {
                retValue.m_value = ReadUInt8(aCursor);
            }
            else if (aType->GetName() == RED4ext::CName{"Int16"})
            {
                retValue.m_value = ReadInt16(aCursor);
            }
            else if (aType->GetName() == RED4ext::CName{"Uint16"})
            {
                retValue.m_value = ReadUInt16(aCursor);
            }
            else if (aType->GetName() == RED4ext::CName{"Int32"})
            {
                retValue.m_value = ReadInt32(aCursor);
            }
            else if (aType->GetName() == RED4ext::CName{"Uint32"})
            {
                retValue.m_value = ReadUInt32(aCursor);
            }
            else if (aType->GetName() == RED4ext::CName{"Int64"})
            {
                retValue.m_value = ReadInt64(aCursor);
            }
            else if (aType->GetName() == RED4ext::CName{"Uint64"})
            {
                retValue.m_value = ReadUInt64(aCursor);
            }
            else if (aType->GetName() == RED4ext::CName{"Float"})
            {
                retValue.m_value = ReadFloat(aCursor);
            }
            else if (aType->GetName() == RED4ext::CName{"Double"})
            {
                retValue.m_value = ReadDouble(aCursor);
            }
            else if (aType->GetName() == RED4ext::CName{"Bool"})
            {
                retValue.m_value = ReadBool(aCursor);
            }
        }
        else if (aType->GetType() == ERTTIType::Simple)
        {
            if (aType->GetName() == RED4ext::CName{"TweakDBID"})
            {
                retValue.m_value = ReadTweakDBID(aCursor);
            }
            else if (aType->GetName() == RED4ext::CName{"NodeRef"})
            {
                retValue.m_value = ReadNodeRef(aCursor);
            }
        }
        else if (aType->GetType() == ERTTIType::Enum)
        {
            retValue.m_value = ReadEnum(aCursor, aType);
        }
        else if (aType->GetType() == ERTTIType::Array)
        {
            retValue.m_value = ReadArray(aCursor, aType);
        }
        else if (aType->GetType() == ERTTIType::StaticArray)
        {
            retValue.m_value = ReadStaticArray(aCursor, aType);
        }
        else if (aType->GetType() == ERTTIType::Class)
        {
            retValue.m_value = ReadClass(aCursor, aType);
        }
        else if (aType->GetType() == ERTTIType::Handle)
        {
            retValue.m_value = std::any{ReadHandle(aCursor, aType)};
        }
        else if (aType->GetType() == ERTTIType::WeakHandle)
        {
            retValue.m_value = std::any{ReadWeakHandle(aCursor, aType)};
        }

		if (!retValue.m_value.has_value())
        {
            throw std::runtime_error{std::format("Failed to resolve value for type {}", aType->GetName().ToString())};
        }

        return retValue;
	}

public:
	virtual redRTTI::RTTIClass ReadClass(FileCursor& aCursor, Red::CBaseRTTIType* aType)
    {
        redRTTI::RTTIClass retValue{};

		auto classType = static_cast<Red::CClass*>(aType);

        Red::DynArray<Red::CProperty*> classProps{};
		classType->GetProperties(classProps);

		std::unordered_set<Red::CName> usedProps{};

        try
        {
            while (aCursor.getRemaining())
            {
                auto propName = Red::CName{aCursor.readUInt64()};

				if (propName.IsNone())
                {
                    break;
				}

				auto propType = Red::CName{aCursor.readUInt64()};
				auto propInfo = classType->GetProperty(propName);

				if (!propInfo)
                {
					// What in the fuck? Eh, it's probably a badly installed mod?
                    throw std::runtime_error{std::format("Class {} does not have property {}", classType->GetName().ToString(), propName.ToString())};
				}

				if (propType != propInfo->type->GetName())
                {
                    throw std::runtime_error(std::format("Type mismatch in class {} for prop {}: expected {}, got {}",
                                                         classType->GetName().ToString(), propName.ToString(),
                                                         propInfo->type->GetName().ToString(), propName.ToString()));
				}

				retValue[propName.ToString()] = ReadValue(aCursor, propInfo->type);
                usedProps.insert(propName);
            }
            for (auto prop : classProps)
            {
                if (prop->flags.isSavable == 0 && prop->flags.isPersistent == 0)
                {
                    continue;
				}

				if (usedProps.contains(prop->name))
                {
                    continue;
				}

				retValue[prop->name.ToString()] = GetDefaultValue(prop->type);
			}
		}
        catch (std::exception e)
        {
            std::println("persistency::ReadClass EXCEPTION {}", e.what());
            return retValue;
		}
		
		retValue.SetFullyLoaded(true);

		return retValue;
	}
};
}