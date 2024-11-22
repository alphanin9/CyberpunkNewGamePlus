#pragma once
#include <RED4ext/RED4ext.hpp>
#include <RED4ext/Package.hpp>
#include <RedLib.hpp>

#include "../defaultNodeData.hpp"

#include "../../package/packageReader.hpp"

namespace modsave
{
    // Improved version of scriptable reader, based on common package loader
	class ScriptableSystemsContainerNodeV2 : public NodeDataInterface {
    private:
        package::Package m_package;
    public:
		static constexpr Red::CName m_nodeName = "ScriptableSystemsContainer";

        virtual void ReadData(FileCursor& aCursor, NodeEntry& aNode) noexcept final;

        Red::Handle<Red::ISerializable>* GetScriptableSystem(Red::CName aName) noexcept;
	};
}