/* Copyright (C) 2016+ AzerothCore, GNU AGPL v3. */
#include "DBCStores.h"
#include "Item.h"
#include "ItemScript.h"
#include "Player.h"
#include "ScriptMgr.h"

namespace
{
uint32 TitleForCharter(uint32 item)
{
    switch (item)
    {
        case 134988:
        case 977020:
            return 210;
        case 977220:
            return 230;
        case 1777010:
            return 180;
        default:
            return 0;
    }
}

class item_ascension_title_charter : public ItemScript
{
public:
    item_ascension_title_charter() : ItemScript("item_ascension_title_charter") { }

    bool OnUse(Player* player, Item* item, SpellCastTargets const&) override
    {
        uint32 const titleId = TitleForCharter(item->GetEntry());
        if (!titleId)
            return false;

        player->SendEquipError(EQUIP_ERR_NONE, item, nullptr);
        CharTitlesEntry const* title = sCharTitlesStore.LookupEntry(titleId);
        if (!title || !player->IsAlive() || player->IsInCombat() || player->HasTitle(title))
            return true;

        player->SetTitle(title);
        uint32 count = 1;
        player->DestroyItemCount(item, count, true);
        return true;
    }
};
}

void AddSC_AscensionTitleCharters()
{
    new item_ascension_title_charter();
}
