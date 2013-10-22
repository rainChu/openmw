#ifndef CSM_WOLRD_COLUMNS_H
#define CSM_WOLRD_COLUMNS_H

#include <string>
#include <vector>

namespace CSMWorld
{
    namespace Columns
    {
        enum ColumnId
        {
            ColumnId_Value = 0,
            ColumnId_Id = 1,
            ColumnId_Modification = 2,
            ColumnId_RecordType = 3,
            ColumnId_ValueType = 4,
            ColumnId_Description = 5,
            ColumnId_Specialisation = 6,
            ColumnId_Attribute = 7,
            ColumnId_Name = 8,
            ColumnId_Playable = 9,
            ColumnId_Hidden = 10,
            ColumnId_MaleWeight = 11,
            ColumnId_FemaleWeight = 12,
            ColumnId_MaleHeight = 13,
            ColumnId_FemaleHeight = 14,
            ColumnId_Volume = 15,
            ColumnId_MinRange = 16,
            ColumnId_MaxRange = 17,
            ColumnId_SoundFile = 18,
            ColumnId_MapColour = 19,
            ColumnId_SleepEncounter = 20,
            ColumnId_Texture = 21,
            ColumnId_SpellType = 22,
            ColumnId_Cost = 23,
            ColumnId_ScriptText = 24,
            ColumnId_Region = 25,
            ColumnId_Cell = 26,
            ColumnId_Scale = 27,
            ColumnId_Owner = 28,
            ColumnId_Soul = 29,
            ColumnId_Faction = 30,
            ColumnId_FactionIndex = 31,
            ColumnId_Charges = 32,
            ColumnId_Enchantment = 33,
            ColumnId_CoinValue = 34,
            ColumnId_Teleport = 35,
            ColumnId_TeleportCell = 36,
            ColumnId_LockLevel = 37,
            ColumnId_Key = 38,
            ColumnId_Trap = 39,
            ColumnId_BeastRace = 40,
            ColumnId_AutoCalc = 41,
            ColumnId_StarterSpell = 42,
            ColumnId_AlwaysSucceeds = 43,
            ColumnId_SleepForbidden = 44,
            ColumnId_InteriorWater = 45,
            ColumnId_InteriorSky = 46,
            ColumnId_Model = 47,
            ColumnId_Script = 48,
            ColumnId_Icon = 49,
            ColumnId_Weight = 50,
            ColumnId_EnchantmentPoints = 51,
            ColumnId_Quality = 52,
            ColumnId_Ai = 53,
            ColumnId_AiHello = 54,
            ColumnId_AiFlee = 55,
            ColumnId_AiFight = 56,
            ColumnId_AiAlarm = 57,
            ColumnId_BuysWeapons = 58,
            ColumnId_BuysArmor = 59,
            ColumnId_BuysClothing = 60,
            ColumnId_BuysBooks = 61,
            ColumnId_BuysIngredients = 62,
            ColumnId_BuysLockpicks = 63,
            ColumnId_BuysProbes = 64,
            ColumnId_BuysLights = 65,
            ColumnId_BuysApparati = 66,
            ColumnId_BuysRepairItems = 67,
            ColumnId_BuysMiscItems = 68,
            ColumnId_BuysPotions = 69,
            ColumnId_BuysMagicItems = 70,
            ColumnId_SellsSpells = 71,
            ColumnId_Trainer = 72,
            ColumnId_Spellmaking = 73,
            ColumnId_EnchantingService = 74,
            ColumnId_RepairService = 75,
            ColumnId_ApparatusType = 76,
            ColumnId_ArmorType = 77,
            ColumnId_Health = 78,
            ColumnId_ArmorValue = 79,
            ColumnId_Scroll = 80,
            ColumnId_ClothingType = 81,
            ColumnId_WeightCapacity = 82,
            ColumnId_OrganicContainer = 83,
            ColumnId_Respawn = 84,
            ColumnId_CreatureType = 85,
            ColumnId_SoulPoints = 86,
            ColumnId_OriginalCreature = 87,
            ColumnId_Biped = 88,
            ColumnId_HasWeapon = 89,
            ColumnId_NoMovement = 90,
            ColumnId_Swims = 91,
            ColumnId_Flies = 92,
            ColumnId_Walks = 93,
            ColumnId_Essential = 94,
            ColumnId_SkeletonBlood = 95,
            ColumnId_MetalBlood = 96,
            ColumnId_OpenSound = 97,
            ColumnId_CloseSound = 98,
            ColumnId_Duration = 99,
            ColumnId_Radius = 100,
            ColumnId_Colour = 101,
            ColumnId_Sound = 102,
            ColumnId_Dynamic = 103,
            ColumnId_Portable = 104,
            ColumnId_NegativeLight = 105,
            ColumnId_Flickering = 106,
            ColumnId_SlowFlickering = 107,
            ColumnId_Pulsing = 108,
            ColumnId_SlowPulsing = 109,
            ColumnId_Fire = 110,
            ColumnId_OffByDefault = 111,
            ColumnId_IsKey = 112,
            ColumnId_Race = 113,
            ColumnId_Class = 114,
            Columnid_Hair = 115,
            ColumnId_Head = 116,
            ColumnId_Female = 117,
            ColumnId_WeaponType = 118,
            ColumnId_WeaponSpeed = 119,
            ColumnId_WeaponReach = 120,
            ColumnId_MinChop = 121,
            ColumnId_MaxChip = 122,
            Columnid_MinSlash = 123,
            ColumnId_MaxSlash = 124,
            ColumnId_MinThrust = 125,
            ColumnId_MaxThrust = 126,
            ColumnId_Magical = 127,
            ColumnId_Silver = 128,
            ColumnId_Filter = 129,
            ColumnId_PositionXPos = 130,
            ColumnId_PositionYPos = 131,
            ColumnId_PositionZPos = 132,
            ColumnId_PositionXRot = 133,
            ColumnId_PositionYRot = 134,
            ColumnId_PositionZRot = 135,
            ColumnId_DoorPositionXPos = 136,
            ColumnId_DoorPositionYPos = 137,
            ColumnId_DoorPositionZPos = 138,
            ColumnId_DoorPositionXRot = 139,
            ColumnId_DoorPositionYRot = 140,
            ColumnId_DoorPositionZRot = 141,

            // Allocated to a separate value range, so we don't get a collision should we ever need
            // to extend the number of use values.
            ColumnId_UseValue1 = 0x10000,
            ColumnId_UseValue2 = 0x10001,
            ColumnId_UseValue3 = 0x10002,
            ColumnId_UseValue4 = 0x10003,

            // Allocated to a separate value range, so we don't get a collision should we ever need
            // to extend the number of attributes. Note that this is not the number of different
            // attributes, but the number of attributes that can be references from a record.
            ColumnId_Attribute1 = 0x20000,
            ColumnId_Attribute2 = 0x20001,

            // Allocated to a separate value range, so we don't get a collision should we ever need
            // to extend the number of skills. Note that this is not the number of different
            // skills, but the number of skills that can be references from a record.
            ColumnId_MajorSkill1 = 0x30000,
            ColumnId_MajorSkill2 = 0x30001,
            ColumnId_MajorSkill3 = 0x30002,
            ColumnId_MajorSkill4 = 0x30003,
            ColumnId_MajorSkill5 = 0x30004,

            ColumnId_MinorSkill1 = 0x40000,
            ColumnId_MinorSkill2 = 0x40001,
            ColumnId_MinorSkill3 = 0x40002,
            ColumnId_MinorSkill4 = 0x40003,
            ColumnId_MinorSkill5 = 0x40004,

            ColumnId_Skill1 = 0x50000,
            ColumnId_Skill2 = 0x50001,
            ColumnId_Skill3 = 0x50002,
            ColumnId_Skill4 = 0x50003,
            ColumnId_Skill5 = 0x50004,
            ColumnId_Skill6 = 0x50005
        };

        std::string getName (ColumnId column);

        int getId (const std::string& name);
        ///< Will return -1 for an invalid name.

        bool hasEnums (ColumnId column);

        std::vector<std::string> getEnums (ColumnId column);
        ///< Returns an empty vector, if \æ column isn't an enum type column.
    }
}

#endif
