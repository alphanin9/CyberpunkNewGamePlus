#pragma once
#include <any>
#include <unordered_set>
#include <string>
#include <print>

#include <nlohmann/json.hpp>
#include <RED4ext/RED4ext.hpp>

namespace redRTTI {
	struct CNameHasher {
		std::size_t operator()(const RED4ext::CName cname) const noexcept {
			return std::hash<std::uint64_t>{}(cname.hash);
		}
	};

	struct RedValueWrapper {
		RED4ext::CName m_typeName;
		RED4ext::ERTTIType m_typeIndex;
		std::any m_value;
	};

	struct RedPackageFieldHeader {
		std::uint16_t nameId;
		std::uint16_t typeId;
		std::uint32_t offset;

		static RedPackageFieldHeader fromCursor(FileCursor& cursor) {
			auto ret = RedPackageFieldHeader{};

			ret.nameId = cursor.readUShort();
			ret.typeId = cursor.readUShort();
			ret.offset = cursor.readUInt();

			return ret;
		}
	};

	using RedClassMap = std::unordered_map<std::string, RedValueWrapper>;

	// Recursively arrive to the underlying type
	inline RED4ext::CBaseRTTIType* getRttiType(RED4ext::CBaseRTTIType* aType) {
		using RED4ext::ERTTIType;
		switch (aType->GetType()) {
		case ERTTIType::Array:
			return getRttiType(static_cast<RED4ext::CRTTIArrayType*>(aType)->GetInnerType());
		case ERTTIType::StaticArray:
			return getRttiType(static_cast<RED4ext::CRTTIStaticArrayType*>(aType)->GetInnerType());
		case ERTTIType::NativeArray:
			return getRttiType(static_cast<RED4ext::CRTTINativeArrayType*>(aType)->GetInnerType());
		case ERTTIType::Handle:
			return getRttiType(static_cast<RED4ext::CRTTIHandleType*>(aType)->GetInnerType());
		case ERTTIType::WeakHandle:
			return getRttiType(static_cast<RED4ext::CRTTIWeakHandleType*>(aType)->GetInnerType());
		default:
			return aType;
		}
	}

	namespace reader {
		template<typename T>
		inline T readPrimitive(FileCursor& aCursor) {
			return aCursor.readValue<T>();
		}

		inline RED4ext::TweakDBID readTweakDBID(FileCursor& aCursor) {
			return aCursor.readTdbId();
		}

		inline RED4ext::CName readCName(FileCursor& aCursor, std::vector<std::string>& names) {
			auto index = aCursor.readUShort();

			if (index > names.size()) {
				return RED4ext::CName{ "None" };
			}
			else {
				return RED4ext::CName{ names.at(index).c_str() };
			}
		}

		inline RED4ext::NodeRef readNodeRef(FileCursor& aCursor) {
			// Evil
			const auto str = aCursor.readLengthPrefixedString();
			const auto length_needed = WideCharToMultiByte(CP_UTF8, 0, &str.at(0), str.size(), nullptr, 0, nullptr, nullptr);

			auto ascii = std::string(length_needed, 0);

			WideCharToMultiByte(CP_UTF8, 0, &str.at(0), str.size(), &ascii.at(0), length_needed, nullptr, nullptr);

			return RED4ext::NodeRef{ ascii.c_str() };
		}

		// A very limited subset
		// We don't need much, we have basically all we need for the ScriptableSystemContainer in the save
		// FUCK handle reading btw
		namespace {
			RedClassMap readClassInternal(FileCursor& aCursor, RED4ext::CRTTISystem* aRtti, std::string_view aClassName, std::vector<std::string>& aNames);

			std::any readValue(FileCursor& aCursor, RED4ext::CRTTISystem* aRtti, RED4ext::CBaseRTTIType* aType, std::vector<std::string>& aNames) {
				using RED4ext::ERTTIType;
				// Cope, I'm not using a switch statement
				if (aType->GetType() == ERTTIType::Name) {
					if (aType->GetName() == RED4ext::CName{ "CName" }) {
						return readCName(aCursor, aNames);
					}
				}
				else if (aType->GetType() == ERTTIType::Fundamental) {
					if (aType->GetName() == RED4ext::CName{ "Int8" }) {
						return readPrimitive<std::int8_t>(aCursor);
					}
					if (aType->GetName() == RED4ext::CName{ "Uint8" }) {
						return readPrimitive<std::uint8_t>(aCursor);
					}
					if (aType->GetName() == RED4ext::CName{ "Int16" }) {
						return readPrimitive<std::int16_t>(aCursor);
					}
					if (aType->GetName() == RED4ext::CName{ "Uint16" }) {
						return readPrimitive<std::uint16_t>(aCursor);
					}
					if (aType->GetName() == RED4ext::CName{ "Int32" }) {
						return readPrimitive<std::int32_t>(aCursor);
					}
					if (aType->GetName() == RED4ext::CName{ "Uint32" }) {
						return readPrimitive<std::uint32_t>(aCursor);
					}
					if (aType->GetName() == RED4ext::CName{ "Int64" }) {
						return readPrimitive<std::int64_t>(aCursor);
					}
					if (aType->GetName() == RED4ext::CName{ "Uint64" }) {
						return readPrimitive<std::uint64_t>(aCursor);
					}
					if (aType->GetName() == RED4ext::CName{ "Float" }) {
						return readPrimitive<float>(aCursor);
					}
					if (aType->GetName() == RED4ext::CName{ "Double" }) {
						return readPrimitive<double>(aCursor);
					}
					if (aType->GetName() == RED4ext::CName{ "Bool" }) {
						return readPrimitive<bool>(aCursor);
					}
				}
				else if (aType->GetType() == ERTTIType::Simple) {
					if (aType->GetName() == RED4ext::CName{ "TweakDBID" }) {
						return readTweakDBID(aCursor);
					}
					else if (aType->GetName() == RED4ext::CName{ "NodeRef" }) {
						return readNodeRef(aCursor);
					}
				}
				else if (aType->GetType() == ERTTIType::Enum) {
					auto nameIndex = aCursor.readUShort();

					if (nameIndex >= aNames.size()) {
						return RED4ext::CName{ "UnknownEnum" };
					}
					else {
						return RED4ext::CName{ aNames.at(nameIndex).c_str() };
					}
				}
				else if (aType->GetType() == ERTTIType::Array) {
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
				}
				else if (aType->GetType() == ERTTIType::Class) {
					return readClassInternal(aCursor, aRtti, aType->GetName().ToString(), aNames);
				}
				else if (aType->GetType() == ERTTIType::Handle) {
					return aCursor.readInt();
				}
				else if (aType->GetType() == ERTTIType::WeakHandle) {
					return aCursor.readInt();
				}

				return std::string{ "Unknown type!" };
			}

			// Hack to not get issues with outputs
			std::any getDefaultValue(RED4ext::CBaseRTTIType* aType) {
				using RED4ext::ERTTIType;
				// Cope, I'm not using a switch statement
				if (aType->GetType() == ERTTIType::Name) {
					if (aType->GetName() == RED4ext::CName{ "CName" }) {
						return RED4ext::CName{};
					}
				}
				else if (aType->GetType() == ERTTIType::Fundamental) {
					if (aType->GetName() == RED4ext::CName{ "Int8" }) {
						return std::int8_t{};
					}
					if (aType->GetName() == RED4ext::CName{ "Uint8" }) {
						return std::uint8_t{};
					}
					if (aType->GetName() == RED4ext::CName{ "Int16" }) {
						return std::int16_t{};
					}
					if (aType->GetName() == RED4ext::CName{ "Uint16" }) {
						return std::uint16_t{};
					}
					if (aType->GetName() == RED4ext::CName{ "Int32" }) {
						return std::int32_t{};
					}
					if (aType->GetName() == RED4ext::CName{ "Uint32" }) {
						return std::uint32_t{};
					}
					if (aType->GetName() == RED4ext::CName{ "Int64" }) {
						return std::int64_t{};
					}
					if (aType->GetName() == RED4ext::CName{ "Uint64" }) {
						return std::uint64_t{};
					}
					if (aType->GetName() == RED4ext::CName{ "Float" }) {
						return float{};
					}
					if (aType->GetName() == RED4ext::CName{ "Double" }) {
						return double{};
					}
					if (aType->GetName() == RED4ext::CName{ "Bool" }) {
						return false;
					}
				}
				else if (aType->GetType() == ERTTIType::Simple) {
					if (aType->GetName() == RED4ext::CName{ "TweakDBID" }) {
						return RED4ext::TweakDBID{};
					}
					else if (aType->GetName() == RED4ext::CName{ "NodeRef" }) {
						return RED4ext::NodeRef{};
					}
				}
				else if (aType->GetType() == ERTTIType::Enum) {
					auto typeEnum = static_cast<RED4ext::CEnum*>(aType);
					if (typeEnum->hashList.size > 0) {
						return typeEnum->hashList[0];
					}
					else {
						return RED4ext::CName{ "Invalid" };
					}
				}
				else if (aType->GetType() == ERTTIType::Array) {
					return std::vector<RedValueWrapper>{};
				}
				else if (aType->GetType() == ERTTIType::Class) {
					return RedClassMap{};
				}
				else if (aType->GetType() == ERTTIType::Handle) {
					return int{};
				}
				else if (aType->GetType() == ERTTIType::WeakHandle) {
					return int{};
				}

				return std::string{ "Unknown type!" };
			}

			RedClassMap readClassInternal(FileCursor& aCursor, RED4ext::CRTTISystem* aRtti, std::string_view aClassName, std::vector<std::string>& aNames) {
				const auto baseOffset = aCursor.offset;
				const auto fieldCount = aCursor.readUShort();

				auto fieldDescriptors = std::vector<RedPackageFieldHeader>{};
				fieldDescriptors.reserve(fieldCount);

				for (auto i = 0; i < fieldCount; i++) {
					fieldDescriptors.push_back(RedPackageFieldHeader::fromCursor(aCursor));
				}

				auto classPtr = aRtti->GetClass(RED4ext::CName{ aClassName.data() });
				assert(classPtr);

				auto classProps = RED4ext::DynArray<RED4ext::CProperty*>{};
				classPtr->GetProperties(classProps);

				auto usedProperties = std::unordered_set<RED4ext::CName>{};

				auto classMap = RedClassMap{};

				for (auto fieldDesc : fieldDescriptors) {
					aCursor.seekTo(FileCursor::SeekTo::Start, baseOffset + fieldDesc.offset);

					auto fieldName = aNames.at(fieldDesc.nameId);
					const auto classProperty = classPtr->GetProperty(fieldName.c_str());

					assert(classProperty);

					auto redValue = RedValueWrapper{};
					redValue.m_typeName = RED4ext::CName{ aNames.at(fieldDesc.typeId).c_str() };
					redValue.m_typeIndex = classProperty->type->GetType();
					redValue.m_value = readValue(aCursor, aRtti, classProperty->type, aNames);

					usedProperties.insert(classProperty->name);

					classMap[fieldName] = redValue;
				}

				// For easier looks later on
				for (auto prop : classProps) {
					if (prop->flags.isPersistent == 0 && prop->flags.isSavable == 0) {
						continue;
					}

					if (usedProperties.contains(prop->name)) {
						continue;
					}

					auto redValue = RedValueWrapper{};
					redValue.m_typeName = prop->type->GetName();
					redValue.m_typeIndex = prop->type->GetType();
					redValue.m_value = getDefaultValue(prop->type);

					classMap[std::string{ prop->name.ToString()}] = redValue;
				}

				return classMap;
			}
		}

		inline RedClassMap readClass(FileCursor& aCursor, RED4ext::CRTTISystem* aRtti, std::string_view aClassName, std::vector<std::string>& aNames) {
			return readClassInternal(aCursor, aRtti, aClassName, aNames);
		}
	}

	namespace dumper {
		namespace {
			void dumpClassInternal(std::ostream& outputBuffer, RedClassMap& aDatamap, int aIndentation);

			void dumpProperty(std::ostream& outputBuffer, std::string_view aValueName, RedValueWrapper& aProperty, int aIndentation, bool aIsArray = false) {
				auto redType = aProperty.m_typeIndex;
				auto redTypeName = aProperty.m_typeName;

				using RED4ext::ERTTIType;

				constexpr auto MAX_INDENTATION = 8;

				if (aIndentation >= MAX_INDENTATION) {
					return;
				}

				for (auto i = 0; i < aIndentation; i++) {
					outputBuffer << "\t";
				}

				if (!aIsArray) {
					outputBuffer << redTypeName.ToString() << " " << aValueName << " = ";
				}

				if (redType == ERTTIType::Class) {
					auto classData = std::any_cast<RedClassMap>(aProperty.m_value);
					outputBuffer << "{" << "\n";
					dumpClassInternal(outputBuffer, classData, aIndentation + 1);
					for (auto i = 0; i < aIndentation; i++) {
						outputBuffer << "\t";
					}
					outputBuffer << "}" << "\n";
					return;
				}
				else if (redType == ERTTIType::Array) {
					auto arrayData = std::any_cast<std::vector<RedValueWrapper>>(aProperty.m_value);
					outputBuffer << "[" << "\n";
					for (auto& i : arrayData) {
						dumpProperty(outputBuffer, aValueName, i, aIndentation + 1, true);
					}
					for (auto i = 0; i < aIndentation; i++) {
						outputBuffer << "\t";
					}
					outputBuffer << "]" << "\n";
					return;
				}
				else if (redType == ERTTIType::Name) {
					outputBuffer << "CName " << std::any_cast<RED4ext::CName>(aProperty.m_value).ToString() << "\n";
					return;
				}
				else if (redType == ERTTIType::Fundamental) {
					// Vulnerable thanks to char fuckery, have to upcast them
					if (redTypeName == RED4ext::CName{ "Int8" }) {
						outputBuffer << static_cast<int>(std::any_cast<std::int8_t>(aProperty.m_value)) << "\n";
						return;
					}
					if (redTypeName == RED4ext::CName{ "Uint8" }) {
						outputBuffer << static_cast<int>(std::any_cast<std::uint8_t>(aProperty.m_value)) << "\n";
						return;
					}
					if (redTypeName == RED4ext::CName{ "Int16" }) {
						outputBuffer << std::any_cast<std::int16_t>(aProperty.m_value) << "\n";
						return;
					}
					if (redTypeName == RED4ext::CName{ "Uint16" }) {
						outputBuffer << std::any_cast<std::uint16_t>(aProperty.m_value) << "\n";
						return;
					}
					if (redTypeName == RED4ext::CName{ "Int32" }) {
						outputBuffer << std::any_cast<std::int32_t>(aProperty.m_value) << "\n";
						return;
					}
					if (redTypeName == RED4ext::CName{ "Uint32" }) {
						outputBuffer << std::any_cast<std::uint32_t>(aProperty.m_value) << "\n";
						return;
					}
					if (redTypeName == RED4ext::CName{ "Int64" }) {
						outputBuffer << std::any_cast<std::int64_t>(aProperty.m_value) << "\n";
						return;
					}
					if (redTypeName == RED4ext::CName{ "Uint64" }) {
						outputBuffer << std::any_cast<std::uint64_t>(aProperty.m_value) << "\n";
						return;
					}
					if (redTypeName == RED4ext::CName{ "Float" }) {
						outputBuffer << std::any_cast<float>(aProperty.m_value) << "\n";
						return;
					}
					if (redTypeName == RED4ext::CName{ "Double" }) {
						outputBuffer << std::any_cast<double>(aProperty.m_value) << "\n";
						return;
					}
					if (redTypeName == RED4ext::CName{ "Bool" }) {
						outputBuffer << std::any_cast<bool>(aProperty.m_value) << "\n";
						return;
					}
				}
				else if (redType == ERTTIType::Simple) {
					if (redTypeName == RED4ext::CName{ "TweakDBID" }) {
						outputBuffer << std::format("{:#010x}", std::any_cast<RED4ext::TweakDBID>(aProperty.m_value).value) << "\n";
						return;
					}
					else if (redTypeName == RED4ext::CName{ "NodeRef" }) {
						outputBuffer << std::any_cast<RED4ext::NodeRef>(aProperty.m_value).hash << "\n";
						return;
					}
				}
				else if (redType == ERTTIType::Enum) {
					outputBuffer << "Enum " << std::any_cast<RED4ext::CName>(aProperty.m_value).ToString() << "\n";
					return;
				}
				else if (redType == ERTTIType::Handle) {
					outputBuffer << "Handle to chunk " << std::any_cast<int>(aProperty.m_value) << "\n";
					return;
				}
				else if (redType == ERTTIType::WeakHandle) {
					outputBuffer << "Weak handle to chunk " << std::any_cast<int>(aProperty.m_value) << "\n";
					return;
				}

				outputBuffer << "We don't know :(\n";
			}

			void dumpClassInternal(std::ostream& outputBuffer, RedClassMap& aDatamap, int aIndentation) {
				for (auto& [key, value] : aDatamap) {
					dumpProperty(outputBuffer, key.c_str(), value, aIndentation);
				}
			}
		}

		inline void dumpClass(RedValueWrapper& aDatamap) {
			auto fileName = std::format("savegame_scriptable_{}.txt", aDatamap.m_typeName.ToString());
			auto widened = std::wstring(fileName.begin(), fileName.end());

			auto filePath = std::filesystem::current_path() / L"ClassDumps" / widened;

			auto saveGameDump = std::fstream(filePath, std::ios::out | std::ios::trunc);

			auto datamap = std::any_cast<RedClassMap>(aDatamap.m_value);
			dumpClassInternal(saveGameDump, datamap, 0);
		}
	}
}