#pragma once
#include <any>
#include <unordered_set>
#include <string>
#include <print>

#include <nlohmann/json.hpp>
#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include "../../rttiHelpers/rttiValueReader.hpp"

namespace scriptable
{
struct RedPackageFieldHeader
{
    std::uint16_t m_nameId;
    std::uint16_t m_typeId;
    std::uint32_t m_offset;

    inline static RedPackageFieldHeader FromCursor(FileCursor& cursor)
    {
        auto ret = RedPackageFieldHeader{};

        ret.m_nameId = cursor.readUShort();
        ret.m_typeId = cursor.readUShort();
        ret.m_offset = cursor.readUInt();

        return ret;
    }
};

struct OptimizedFieldHeader
{
    Red::CName m_name;
    Red::CName m_type;
    std::uint32_t m_offset;

    inline static OptimizedFieldHeader Make(RedPackageFieldHeader aHeader, const std::vector<Red::CName>& aNames)
    {
        OptimizedFieldHeader ret{};
        ret.m_name = aNames.at(aHeader.m_nameId);
        ret.m_type = aNames.at(aHeader.m_typeId);
        ret.m_offset = aHeader.m_offset;

        return ret;
    }
};

class ScriptableReader : public redRTTI::RTTIReader
{
private:
    std::vector<Red::CName> m_names;

	virtual Red::TweakDBID ReadTweakDBID(FileCursor& aCursor)
    {
        return aCursor.readTdbId();
	}

	virtual Red::NodeRef ReadNodeRef(FileCursor& aCursor)
    {
        const auto str = aCursor.readLengthPrefixedString();
        const auto lengthNeeded =
            WideCharToMultiByte(CP_UTF8, 0, &str.at(0), str.size(), nullptr, 0, nullptr, nullptr);

        std::string ascii(lengthNeeded, 0);

        WideCharToMultiByte(CP_UTF8, 0, &str.at(0), str.size(), &ascii.at(0), lengthNeeded, nullptr, nullptr);

        return Red::NodeRef{ascii.c_str()};
	}
	
	virtual Red::CName ReadCName(FileCursor& aCursor)
    {
        auto index = aCursor.readUShort();
		
		if (index >= m_names.size())
        {
            return Red::CName{};
		}

		return m_names.at(index);
	}

	virtual Red::CName ReadEnum(FileCursor& aCursor, Red::CBaseRTTIType* aType)
    {
        return ReadCName(aCursor);
	}

	virtual redRTTI::RTTIArray ReadArray(FileCursor& aCursor, Red::CBaseRTTIType* aType)
    {
        /*
		const auto elementCount = aCursor.readInt();
					auto arr = std::vector<RedValueWrapper>{};

					auto innerType = static_cast<RED4ext::CRTTIBaseArrayType*>(aType)->GetInnerType();

					const auto typeId = innerType->GetType();
					const auto nameId = innerType->GetName();

					for (auto i = 0; i < elementCount; i++) {
						auto wrapper = RedValueWrapper{};

						wrapper.m_value = readValue(aCursor, aRtti, innerType, aNames);
						wrapper.m_typeName = nameId;
						wrapper.m_typeIndex = typeId;

						arr.push_back(wrapper);
					}

					return arr;
		*/

		auto innerType = static_cast<RED4ext::CRTTIBaseArrayType*>(aType)->GetInnerType();

		const auto elementCount = aCursor.readInt();
		
		redRTTI::RTTIArray retValue{};
		retValue.reserve(elementCount);

		for (auto i = 0; i < elementCount; i++)
        {
            retValue.push_back(ReadValue(aCursor, innerType));
		}

		return retValue;
	}

	virtual redRTTI::RTTIValue ReadHandle(FileCursor& aCursor, Red::CBaseRTTIType* aType)
    {
        redRTTI::RTTIValue ret{};

		ret.m_typeIndex = static_cast<Red::CRTTIHandleType*>(aType)->GetInnerType()->GetType();
        ret.m_typeName = static_cast<Red::CRTTIHandleType*>(aType)->GetInnerType()->GetName();
        ret.m_value = aCursor.readInt();

		return ret;
	}

	virtual redRTTI::RTTIValue ReadWeakHandle(FileCursor& aCursor, Red::CBaseRTTIType* aType)
    {
        redRTTI::RTTIValue ret{};

        ret.m_typeIndex = static_cast<Red::CRTTIWeakHandleType*>(aType)->GetInnerType()->GetType();
        ret.m_typeName = static_cast<Red::CRTTIWeakHandleType*>(aType)->GetInnerType()->GetName();
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
    // HACK, lambda needs access to m_names
    OptimizedFieldHeader MakeOptimizedHeader(RedPackageFieldHeader aHeader)
    {
        return OptimizedFieldHeader::Make(aHeader, m_names);
    }

    virtual redRTTI::RTTIClass ReadClass(FileCursor& aCursor, Red::CBaseRTTIType* aType)
    {
        const auto baseOffset = aCursor.offset;
        const auto fieldCount = aCursor.readUShort();

		auto fieldDescriptors = aCursor.ReadMultipleClasses<RedPackageFieldHeader>(fieldCount);
        std::unordered_map<Red::CName, OptimizedFieldHeader> fieldDescMap{};

        std::transform(fieldDescriptors.begin(), fieldDescriptors.end(),
                       std::inserter(fieldDescMap, fieldDescMap.end()),
                       [this](RedPackageFieldHeader aHeader)
                       { 
                            auto optimized = this->MakeOptimizedHeader(aHeader); 
                            return std::make_pair(optimized.m_name, optimized);
                       });

		auto classType = static_cast<Red::CClass*>(aType);
        Red::DynArray<Red::CProperty*> properties{};

		classType->GetProperties(properties);
		redRTTI::RTTIClass retValue{};

        try
        {
            for (auto prop : properties)
            {
                if (prop->flags.isSavable == 0 && prop->flags.isPersistent == 0)
                {
                    continue;
                }
                auto saveProp = fieldDescMap.find(prop->name);
                
                // We don't have it stored, this is normal behavior, use the default value (generally 0 is fine)
                if (saveProp == fieldDescMap.end())
                {
                    retValue[prop->name.ToString()] = GetDefaultValue(prop->type);
                    continue;
                }

                // Verify prop type
                auto storedType = PluginContext::m_rtti->GetType(saveProp->second.m_type);

                if (!storedType || prop->type != storedType)
                {
                    PluginContext::Error(
                        std::format("scriptable::ReadClass, class {}, property type mismatch - {} != {}",
                                    classType->GetName().ToString(), saveProp->second.m_type.ToString(),
                                    prop->type->GetName().ToString()));
                    retValue[prop->name.ToString()] = GetDefaultValue(prop->type);
                    continue;
                }

                aCursor.seekTo(FileCursor::SeekTo::Start, baseOffset + saveProp->second.m_offset);

                retValue[prop->name.ToString()] = ReadValue(aCursor, prop->type);
            }
        }
        catch (std::exception e)
        {
            PluginContext::Error(std::format("scriptable::ReadClass: {}", e.what()));
            return retValue;
        }

        retValue.SetFullyLoaded(true);
        return retValue;
	}

	ScriptableReader(const std::vector<Red::CName>& aNames)
    {
        m_names = aNames;
	}
};
}