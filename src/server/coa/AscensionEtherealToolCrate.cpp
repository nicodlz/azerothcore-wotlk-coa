/* Copyright (C) 2016+ AzerothCore, GNU AGPL v3. */
#include "Item.h"
#include "ItemScript.h"
#include "Player.h"
#include "ScriptMgr.h"

namespace
{
constexpr uint32 EtherealToolCrate = 8263511;
constexpr uint32 EtherealToolDispenser = 8263501;

class item_ascension_ethereal_tool_crate : public ItemScript
{
public:
    item_ascension_ethereal_tool_crate() : ItemScript("item_ascension_ethereal_tool_crate") { }

    bool OnUse(Player* player, Item* item, SpellCastTargets const&) override
    {
        if (item->GetEntry() != EtherealToolCrate)
            return false;

        player->SendEquipError(EQUIP_ERR_NONE, item, nullptr);
        if (!player->IsAlive() || player->IsInCombat())
            return true;

        ItemPosCountVec destination;
        InventoryResult const result = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, destination,
            EtherealToolDispenser, 1);
        if (result != EQUIP_ERR_OK)
        {
            player->SendEquipError(result, item, nullptr, EtherealToolDispenser);
            return true;
        }

        if (Item* dispenser = player->StoreNewItem(destination, EtherealToolDispenser, true))
        {
            uint32 count = 1;
            player->DestroyItemCount(item, count, true);
            player->SendNewItem(dispenser, 1, true, false);
        }
        return true;
    }
};
}

void AddSC_AscensionEtherealToolCrate()
{
    new item_ascension_ethereal_tool_crate();
}
