/* Copyright (C) 2016+ AzerothCore, GNU AGPL v3. */
#include "GameObject.h"
#include "GameObjectScript.h"
#include "Player.h"

namespace
{
class go_ascension_homebound_portal final : public GameObjectScript
{
public:
    go_ascension_homebound_portal() : GameObjectScript("go_ascension_homebound_portal") { }

    bool OnGossipHello(Player* player, GameObject* portal) override
    {
        Unit* owner = portal->GetOwner();
        if (Player* homeowner = owner ? owner->ToPlayer() : nullptr)
            player->TeleportTo(homeowner->m_homebindMapId, homeowner->m_homebindX,
                homeowner->m_homebindY, homeowner->m_homebindZ, player->GetOrientation());
        return true;
    }
};
}

void AddSC_AscensionHomeboundPortal()
{
    new go_ascension_homebound_portal();
}
