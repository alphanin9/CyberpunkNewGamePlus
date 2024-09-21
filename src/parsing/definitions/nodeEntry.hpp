#pragma once
#include "../cursorDef.hpp"
#include "nodeParsers/interfaceNodeData.hpp"

#include <print>
#include <string>
#include <vector>

namespace save {
	struct NodeEntry {
		std::string name;

		int id;
		int nextId;
		int childId;
		int offset;
		int size;

		int dataSize;
		int trailingSize;

		bool isChild;
		bool isFirstChild;
		bool isReadByParent;
		bool isWritingOwnTrailingSize = true;

		NodeEntry* parent = nullptr;
		NodeEntry* nextNode = nullptr;

		std::vector<NodeEntry*> nodeChildren;

		// Keeping a data buffer like this is kind of dumb IMHO
		// It would likely be best to have a unique pointer to a structure
		// std::vector<std::byte> nodeDataBuffer;

		// EVIL UGLY CODE AHEAD, WILL LIKELY LEAK MEMORY LIKE CRAZY
		std::unique_ptr<NodeDataInterface> nodeData;

		Red::CName m_hash;

		inline void addChild(NodeEntry* child) {
			child->isChild = true;
			child->parent = this;

			if (nodeChildren.empty()) {
				child->isFirstChild = true;
			}

			nodeChildren.push_back(child);
		}

		static NodeEntry FromCursor(FileCursor& fileCursor) {
			NodeEntry ret{};

			ret.name = fileCursor.OptimizedReadLengthPrefixedANSI();
			ret.nextId = fileCursor.readInt();
			ret.childId = fileCursor.readInt();
			ret.offset = fileCursor.readInt();
			ret.size = fileCursor.readInt();

			ret.m_hash = Red::CName{ret.name.c_str()};

			return ret;
		}

		inline int GetExpectedSize() const
        {
            auto size = this->size;
            if (isWritingOwnTrailingSize)
            {
                size += trailingSize;				
			}

			return size;
		}
	};
}
