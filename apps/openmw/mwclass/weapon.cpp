
#include "weapon.hpp"

#include <components/esm/loadweap.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/actiontake.hpp"
#include "../mwworld/actionequip.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/nullaction.hpp"

#include "../mwgui/tooltips.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

namespace MWClass
{
    std::string Weapon::getId (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Weapon> *ref = ptr.get<ESM::Weapon>();

        return ref->mBase->mId;
    }

    void Weapon::insertObjectRendering (const MWWorld::Ptr& ptr, MWRender::RenderingInterface& renderingInterface) const
    {
        const std::string model = getModel(ptr);
        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model);
        }
    }

    void Weapon::insertObject(const MWWorld::Ptr& ptr, MWWorld::PhysicsSystem& physics) const
    {
        const std::string model = getModel(ptr);
        if(!model.empty())
            physics.addObject(ptr,true);
    }

    std::string Weapon::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::Weapon> *ref =
            ptr.get<ESM::Weapon>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string Weapon::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Weapon> *ref =
            ptr.get<ESM::Weapon>();

        return ref->mBase->mName;
    }

    boost::shared_ptr<MWWorld::Action> Weapon::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor) const
    {
        return defaultItemActivate(ptr, actor);
    }

    bool Weapon::hasItemHealth (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Weapon> *ref =
            ptr.get<ESM::Weapon>();

        return (ref->mBase->mData.mType < 11); // thrown weapons and arrows/bolts don't have health, only quantity
    }

    int Weapon::getItemMaxHealth (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Weapon> *ref =
            ptr.get<ESM::Weapon>();

        return ref->mBase->mData.mHealth;
    }

    std::string Weapon::getScript (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Weapon> *ref =
            ptr.get<ESM::Weapon>();

        return ref->mBase->mScript;
    }

    std::pair<std::vector<int>, bool> Weapon::getEquipmentSlots (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Weapon> *ref =
            ptr.get<ESM::Weapon>();

        std::vector<int> slots_;
        bool stack = false;

        if (ref->mBase->mData.mType==ESM::Weapon::Arrow || ref->mBase->mData.mType==ESM::Weapon::Bolt)
        {
            slots_.push_back (int (MWWorld::InventoryStore::Slot_Ammunition));
            stack = true;
        }
        else if (ref->mBase->mData.mType==ESM::Weapon::MarksmanThrown)
        {
            slots_.push_back (int (MWWorld::InventoryStore::Slot_CarriedRight));
            stack = true;
        }
        else
            slots_.push_back (int (MWWorld::InventoryStore::Slot_CarriedRight));

        return std::make_pair (slots_, stack);
    }

    int Weapon::getEquipmentSkill (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Weapon> *ref =
            ptr.get<ESM::Weapon>();

        const int size = 12;

        static const int sMapping[size][2] =
        {
            { ESM::Weapon::ShortBladeOneHand, ESM::Skill::ShortBlade },
            { ESM::Weapon::LongBladeOneHand, ESM::Skill::LongBlade },
            { ESM::Weapon::LongBladeTwoHand, ESM::Skill::LongBlade },
            { ESM::Weapon::BluntOneHand, ESM::Skill::BluntWeapon },
            { ESM::Weapon::BluntTwoClose, ESM::Skill::BluntWeapon },
            { ESM::Weapon::BluntTwoWide, ESM::Skill::BluntWeapon },
            { ESM::Weapon::SpearTwoWide, ESM::Skill::Spear },
            { ESM::Weapon::AxeOneHand, ESM::Skill::Axe },
            { ESM::Weapon::AxeTwoHand, ESM::Skill::Axe },
            { ESM::Weapon::MarksmanBow, ESM::Skill::Marksman },
            { ESM::Weapon::MarksmanCrossbow, ESM::Skill::Marksman },
            { ESM::Weapon::MarksmanThrown, ESM::Skill::Marksman }
        };

        for (int i=0; i<size; ++i)
            if (sMapping[i][0]==ref->mBase->mData.mType)
                return sMapping[i][1];

        return -1;
    }

    int Weapon::getValue (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Weapon> *ref =
            ptr.get<ESM::Weapon>();

        if (ptr.getCellRef().mCharge == -1)
            return ref->mBase->mData.mValue;
        else
            return ref->mBase->mData.mValue * (static_cast<float>(ptr.getCellRef().mCharge) / getItemMaxHealth(ptr));
    }

    void Weapon::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Weapon);

        registerClass (typeid (ESM::Weapon).name(), instance);
    }

    std::string Weapon::getUpSoundId (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Weapon> *ref =
            ptr.get<ESM::Weapon>();

        int type = ref->mBase->mData.mType;
        // Ammo
        if (type == 12 || type == 13)
        {
            return std::string("Item Ammo Up");
        }
        // Bow
        if (type == 9)
        {
            return std::string("Item Weapon Bow Up");
        }
        // Crossbow
        if (type == 10)
        {
            return std::string("Item Weapon Crossbow Up");
        }
        // Longblades, One hand and Two
        if (type == 1 || type == 2)
        {
            return std::string("Item Weapon Longblade Up");
        }
        // Shortblade and thrown weapons
        // thrown weapons may not be entirely correct
        if (type == 0 || type == 11)
        {
            return std::string("Item Weapon Shortblade Up");
        }
        // Spear
        if (type == 6)
        {
            return std::string("Item Weapon Spear Up");
        }
        // Blunts and Axes
        if (type == 3 || type == 4 || type == 5 || type == 7 || type == 8)
        {
            return std::string("Item Weapon Blunt Up");
        }

        return std::string("Item Misc Up");
    }

    std::string Weapon::getDownSoundId (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Weapon> *ref =
            ptr.get<ESM::Weapon>();

        int type = ref->mBase->mData.mType;
        // Ammo
        if (type == 12 || type == 13)
        {
            return std::string("Item Ammo Down");
        }
        // Bow
        if (type == 9)
        {
            return std::string("Item Weapon Bow Down");
        }
        // Crossbow
        if (type == 10)
        {
            return std::string("Item Weapon Crossbow Down");
        }
        // Longblades, One hand and Two
        if (type == 1 || type == 2)
        {
            return std::string("Item Weapon Longblade Down");
        }
        // Shortblade and thrown weapons
        // thrown weapons may not be entirely correct
        if (type == 0 || type == 11)
        {
            return std::string("Item Weapon Shortblade Down");
        }
        // Spear
        if (type == 6)
        {
            return std::string("Item Weapon Spear Down");
        }
        // Blunts and Axes
        if (type == 3 || type == 4 || type == 5 || type == 7 || type == 8)
        {
            return std::string("Item Weapon Blunt Down");
        }

        return std::string("Item Misc Down");
    }

    std::string Weapon::getInventoryIcon (const MWWorld::Ptr& ptr) const
    {
          MWWorld::LiveCellRef<ESM::Weapon> *ref =
            ptr.get<ESM::Weapon>();

        return ref->mBase->mIcon;
    }

    bool Weapon::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Weapon> *ref =
            ptr.get<ESM::Weapon>();

        return (ref->mBase->mName != "");
    }

    MWGui::ToolTipInfo Weapon::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Weapon> *ref =
            ptr.get<ESM::Weapon>();

        MWGui::ToolTipInfo info;
        info.caption = ref->mBase->mName + MWGui::ToolTips::getCountString(ptr.getRefData().getCount());
        info.icon = ref->mBase->mIcon;

        const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();

        std::string text;

        // weapon type & damage. arrows / bolts don't have his info.
        if (ref->mBase->mData.mType < 12)
        {
            text += "\n#{sType} ";

            std::map <int, std::pair <std::string, std::string> > mapping;
            mapping[ESM::Weapon::ShortBladeOneHand] = std::make_pair("sSkillShortblade", "sOneHanded");
            mapping[ESM::Weapon::LongBladeOneHand] = std::make_pair("sSkillLongblade", "sOneHanded");
            mapping[ESM::Weapon::LongBladeTwoHand] = std::make_pair("sSkillLongblade", "sTwoHanded");
            mapping[ESM::Weapon::BluntOneHand] = std::make_pair("sSkillBluntweapon", "sOneHanded");
            mapping[ESM::Weapon::BluntTwoClose] = std::make_pair("sSkillBluntweapon", "sTwoHanded");
            mapping[ESM::Weapon::BluntTwoWide] = std::make_pair("sSkillBluntweapon", "sTwoHanded");
            mapping[ESM::Weapon::SpearTwoWide] = std::make_pair("sSkillSpear", "sTwoHanded");
            mapping[ESM::Weapon::AxeOneHand] = std::make_pair("sSkillAxe", "sOneHanded");
            mapping[ESM::Weapon::AxeTwoHand] = std::make_pair("sSkillAxe", "sTwoHanded");
            mapping[ESM::Weapon::MarksmanBow] = std::make_pair("sSkillMarksman", "");
            mapping[ESM::Weapon::MarksmanCrossbow] = std::make_pair("sSkillMarksman", "");
            mapping[ESM::Weapon::MarksmanThrown] = std::make_pair("sSkillMarksman", "");

            std::string type = mapping[ref->mBase->mData.mType].first;
            std::string oneOrTwoHanded = mapping[ref->mBase->mData.mType].second;

            text += store.get<ESM::GameSetting>().find(type)->getString() +
                ((oneOrTwoHanded != "") ? ", " + store.get<ESM::GameSetting>().find(oneOrTwoHanded)->getString() : "");

            // weapon damage
            if (ref->mBase->mData.mType >= 9)
            {
                // marksman
                text += "\n#{sAttack}: "
                    + MWGui::ToolTips::toString(static_cast<int>(ref->mBase->mData.mChop[0]))
                    + " - " + MWGui::ToolTips::toString(static_cast<int>(ref->mBase->mData.mChop[1]));
            }
            else
            {
                // Chop
                text += "\n#{sChop}: "
                    + MWGui::ToolTips::toString(static_cast<int>(ref->mBase->mData.mChop[0]))
                    + " - " + MWGui::ToolTips::toString(static_cast<int>(ref->mBase->mData.mChop[1]));
                // Slash
                text += "\n#{sSlash}: "
                    + MWGui::ToolTips::toString(static_cast<int>(ref->mBase->mData.mSlash[0]))
                    + " - " + MWGui::ToolTips::toString(static_cast<int>(ref->mBase->mData.mSlash[1]));
                // Thrust
                text += "\n#{sThrust}: "
                    + MWGui::ToolTips::toString(static_cast<int>(ref->mBase->mData.mThrust[0]))
                    + " - " + MWGui::ToolTips::toString(static_cast<int>(ref->mBase->mData.mThrust[1]));
            }
        }

        if (ref->mBase->mData.mType < 11) // thrown weapons and arrows/bolts don't have health, only quantity
        {
            int remainingHealth = (ptr.getCellRef().mCharge != -1) ? ptr.getCellRef().mCharge : ref->mBase->mData.mHealth;
            text += "\n#{sCondition}: " + MWGui::ToolTips::toString(remainingHealth) + "/"
                    + MWGui::ToolTips::toString(ref->mBase->mData.mHealth);
        }

        text += "\n#{sWeight}: " + MWGui::ToolTips::toString(ref->mBase->mData.mWeight);
        text += MWGui::ToolTips::getValueString(getValue(ptr), "#{sValue}");

        info.enchant = ref->mBase->mEnchant;

        if (!info.enchant.empty())
            info.remainingEnchantCharge = ptr.getCellRef().mEnchantmentCharge;

        if (MWBase::Environment::get().getWindowManager()->getFullHelp()) {
            text += MWGui::ToolTips::getMiscString(ref->mRef.mOwner, "Owner");
            text += MWGui::ToolTips::getMiscString(ref->mBase->mScript, "Script");
        }

        info.text = text;

        return info;
    }

    std::string Weapon::getEnchantment (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Weapon> *ref =
            ptr.get<ESM::Weapon>();

        return ref->mBase->mEnchant;
    }

    void Weapon::applyEnchantment(const MWWorld::Ptr &ptr, const std::string& enchId, int enchCharge, const std::string& newName) const
    {
        MWWorld::LiveCellRef<ESM::Weapon> *ref =
        ptr.get<ESM::Weapon>();

        ESM::Weapon newItem = *ref->mBase;
        newItem.mId="";
        newItem.mName=newName;
        newItem.mData.mEnchant=enchCharge;
        newItem.mEnchant=enchId;
        const ESM::Weapon *record = MWBase::Environment::get().getWorld()->createRecord (newItem);
        ref->mBase = record;
        ref->mRef.mRefID = record->mId;
    }

    std::pair<int, std::string> Weapon::canBeEquipped(const MWWorld::Ptr &ptr, const MWWorld::Ptr &npc) const
    {
        std::pair<std::vector<int>, bool> slots_ = MWWorld::Class::get(ptr).getEquipmentSlots(ptr);

        // equip the item in the first free slot
        for (std::vector<int>::const_iterator slot=slots_.first.begin();
            slot!=slots_.first.end(); ++slot)
        {
            if(*slot == MWWorld::InventoryStore::Slot_CarriedRight)
            {
                if(ptr.get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::LongBladeTwoHand ||
                ptr.get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::BluntTwoClose || 
                ptr.get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::BluntTwoWide || 
                ptr.get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::SpearTwoWide ||
                ptr.get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::AxeTwoHand || 
                ptr.get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::MarksmanBow || 
                ptr.get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::MarksmanCrossbow)
                {
                    return std::make_pair (2, "");
                }
            }
            return std::make_pair(1, "");
        }
        return std::make_pair (0, "");
    }

    boost::shared_ptr<MWWorld::Action> Weapon::use (const MWWorld::Ptr& ptr) const
    {
        boost::shared_ptr<MWWorld::Action> action(new MWWorld::ActionEquip(ptr));

        action->setSound(getUpSoundId(ptr));

        return action;
    }

    MWWorld::Ptr
    Weapon::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM::Weapon> *ref =
            ptr.get<ESM::Weapon>();

        return MWWorld::Ptr(&cell.mWeapons.insert(*ref), &cell);
    }

    float Weapon::getEnchantmentPoints (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM::Weapon> *ref =
                ptr.get<ESM::Weapon>();

        return ref->mBase->mData.mEnchant/10.f;
    }

    bool Weapon::canSell (const MWWorld::Ptr& item, int npcServices) const
    {
        return npcServices & ESM::NPC::Weapon;
    }

    float Weapon::getWeight(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM::Weapon> *ref =
            ptr.get<ESM::Weapon>();
        return ref->mBase->mData.mWeight;
    }
}
