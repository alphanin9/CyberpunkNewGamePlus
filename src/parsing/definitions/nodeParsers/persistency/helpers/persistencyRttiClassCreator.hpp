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

// I know this is really stupid and I should have just made a BaseRttiClassCreator to then inherit off
// But this *should* work anyway

namespace redRTTI {
	namespace persistent {
		struct CNameHasher {
			std::size_t operator()(const RED4ext::CName cname) const noexcept {
				return std::hash<std::uint64_t>{}(cname.hash);
			}
		};

		struct RedValueWrapper {
			RED4ext::CName m_typeName;
			RED4ext::ERTTIType m_typeIndex;
			std::any m_value;

			operator std::any() = delete;
			std::any operator()() const {
				return m_value;
			}
		};

		using RedClassMap = std::unordered_map<std::string, RedValueWrapper>;

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

			inline RED4ext::CName readCName(FileCursor& aCursor) {
				return RED4ext::CName{ aCursor.readUInt64() };
			}

			inline RED4ext::NodeRef readNodeRef(FileCursor& aCursor) {
				return RED4ext::NodeRef{ aCursor.readUInt64() };
			}

			// A very limited subset
			// We don't need much, we have basically all we need for the ScriptableSystemContainer in the save
			// FUCK handle reading btw
			namespace {
				RedClassMap readClassInternal(FileCursor& aCursor, RED4ext::CRTTISystem* aRtti, std::string_view aClassName);

				std::any readValue(FileCursor& aCursor, RED4ext::CRTTISystem* aRtti, RED4ext::CBaseRTTIType* aType) {
					using RED4ext::ERTTIType;
					// Cope, I'm not using a switch statement
					if (aType->GetType() == ERTTIType::Name) {
						if (aType->GetName() == RED4ext::CName{ "CName" }) {
							return readCName(aCursor);
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
						auto enumType = static_cast<RED4ext::CEnum*>(aType);
						auto enumIndexer = std::int64_t{};

						switch (enumType->actualSize) {
							case(sizeof(std::int8_t)):
								enumIndexer = aCursor.readByte();
								break;
							case(sizeof(std::int16_t)):
								enumIndexer = aCursor.readShort();
								break;
							case(sizeof(std::int32_t)):
								enumIndexer = aCursor.readInt();
								break;
							case(sizeof(std::int64_t)):
								enumIndexer = aCursor.readInt64();
								break;
							default:
								enumIndexer = aCursor.readInt();
								break;
						}
						// No alias support :(
						if (enumIndexer < enumType->hashList.size) {
							return enumType->hashList[enumIndexer];
						}
						return RED4ext::CName{ "Invalid" };
					}
					else if (aType->GetType() == ERTTIType::Array) {
						const auto elementCount = aCursor.readInt();
						auto arr = std::vector<RedValueWrapper>{};

						auto innerType = static_cast<RED4ext::CRTTIBaseArrayType*>(aType)->GetInnerType();

						const auto typeId = innerType->GetType();
						const auto typeName = innerType->GetName();

						arr.reserve(elementCount);

						for (auto i = 0; i < elementCount; i++) {
							auto wrapper = RedValueWrapper{};

							wrapper.m_value = readValue(aCursor, aRtti, innerType);
							wrapper.m_typeName = typeName;
							wrapper.m_typeIndex = typeId;

							arr.push_back(wrapper);
						}

						return arr;
					}
					else if (aType->GetType() == ERTTIType::StaticArray) {
						const auto arrayType = static_cast<RED4ext::CRTTIStaticArrayType*>(aType);
						const auto innerType = arrayType->GetInnerType();
						
						const auto elementCount = aCursor.readInt();

						auto arr = std::vector<RedValueWrapper>{};
						arr.reserve(elementCount);

						const auto typeId = innerType->GetType();
						const auto typeName = innerType->GetName();

						for (auto i = 0; i < elementCount; i++) {
							auto wrapper = RedValueWrapper{};

							wrapper.m_value = readValue(aCursor, aRtti, innerType);
							wrapper.m_typeName = typeName;
							wrapper.m_typeIndex = typeId;

							arr.push_back(wrapper);
						}

						return arr;
					}
					else if (aType->GetType() == ERTTIType::Class) {
						return readClassInternal(aCursor, aRtti, aType->GetName().ToString());
					}
					else if (aType->GetType() == ERTTIType::Handle) {
						auto handleType = static_cast<RED4ext::CRTTIHandleType*>(aType);
						auto innerType = handleType->GetInnerType();

						auto wrapper = RedValueWrapper{};

						wrapper.m_typeIndex = innerType->GetType();
						wrapper.m_typeName = innerType->GetName();
						wrapper.m_value = readValue(aCursor, aRtti, innerType);

						return std::any{ wrapper };
					}
					else if (aType->GetType() == ERTTIType::WeakHandle) {
						return aCursor.readInt();
					}

					return std::string{ "Unknown type!" };
				}

				// Hack to not get issues with outputs
				// Unimplemented for PS2 so far
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

				RedClassMap readClassInternal(FileCursor& aCursor, RED4ext::CRTTISystem* aRtti, std::string_view aClassName) {
					auto classMap = RedClassMap{};
					auto classPtr = aRtti->GetClass(RED4ext::CName{ aClassName.data() });

					while (aCursor.getRemaining()) {
						auto propNameHash = aCursor.readUInt64();

						if (propNameHash == 0) {
							break;
						}

						auto propInfo = classPtr->GetProperty(RED4ext::CName{ propNameHash });
						auto propType = RED4ext::CName{ aCursor.readUInt64() };
						assert(propInfo);

						assert(propType == propInfo->type->GetName());

						auto wrapper = RedValueWrapper{};

						wrapper.m_typeIndex = propInfo->type->GetType();
						wrapper.m_typeName = propInfo->type->GetName();
						wrapper.m_value = readValue(aCursor, aRtti, propInfo->type);

						classMap[RED4ext::CName{ propNameHash }.ToString()] = wrapper;
					}

					return classMap;
				}
			}

			inline RedClassMap readClass(FileCursor& aCursor, RED4ext::CRTTISystem* aRtti, std::string_view aClassName) {
				return readClassInternal(aCursor, aRtti, aClassName);
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
					else if (redType == ERTTIType::StaticArray) {
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
							auto tdbId = std::any_cast<RED4ext::TweakDBID>(aProperty.m_value);

							Red::CString str;
							Red::CallStatic("gamedataTDBIDHelper", "ToStringDEBUG", str, tdbId);

							outputBuffer << std::format("{:#010x}", tdbId.value) << " " << str.c_str() << "\n";
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
						auto buf = std::any_cast<RedValueWrapper>(aProperty.m_value);
						outputBuffer << "Handle: " << "\n";
						dumpProperty(outputBuffer, aValueName, buf, aIndentation + 1, false);
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
				constexpr auto shouldDumpClass = false;

				if constexpr (!shouldDumpClass) {
					return;
				}

				auto fileName = std::format("savegame_persistent_{}.txt", aDatamap.m_typeName.ToString());
				auto widened = std::wstring(fileName.begin(), fileName.end());

				auto filePath = std::filesystem::current_path() / L"ClassDumpsPersistent" / widened;

				auto saveGameDump = std::fstream(filePath, std::ios::out | std::ios::trunc);

				auto datamap = std::any_cast<RedClassMap>(aDatamap.m_value);
				dumpClassInternal(saveGameDump, datamap, 0);
			}
		}
	}	
}