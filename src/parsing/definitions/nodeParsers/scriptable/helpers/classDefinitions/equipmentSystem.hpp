#pragma once
#include <RED4ext/RED4ext.hpp>
#include <RedLib.hpp>

#include <RED4ext/Scripting/Natives/Generated/game/SLoadout.hpp>

namespace scriptable::native::EquipmentSystem
{

struct EquipmentSystemPlayerData
{
    inline static Red::CClass* s_type;
    Red::ScriptInstance m_instance;
    
    EquipmentSystemPlayerData(Red::ScriptInstance aInstance)
        : m_instance(aInstance)
    {
        if (!s_type)
        {
            s_type = Red::GetClass<EquipmentSystemPlayerData>();
        }
    }

    Red::SLoadout* GetLoadout()
    {
        return s_type->GetProperty("equipment")->GetValuePtr<Red::SLoadout>(m_instance);
    }
};
}