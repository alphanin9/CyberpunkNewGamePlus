#pragma once
#include <unordered_map>

#include "../interfaceNodeData.hpp"
namespace modsave
{
class FactsTableNode : public NodeDataInterface
{
public:
    std::span<std::uint32_t> m_keys;
    std::span<std::uint32_t> m_values;

    static constexpr Red::CName m_nodeName = "FactsTable";
    virtual void ReadData(FileCursor& aCursor, NodeEntry& aNode) noexcept final;
};

class FactsDBNode : public NodeDataInterface
{
    std::vector<FactsTableNode*> m_factsTables{}; // Does not need to be dynamic, but no real worry
    std::size_t m_tableCount{};

public:
    static constexpr Red::CName m_nodeName = "FactsDB";
    virtual void ReadData(FileCursor& aCursor, NodeEntry& aNode) noexcept final;

    std::uint32_t GetFact(const char* aName) noexcept;
    std::uint32_t GetFact(std::uint32_t aHash) noexcept;
};
}