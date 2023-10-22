#pragma once

#include "includes.h"

namespace Global
{

    struct PatchingTarget
    {
        string ReferencedString;
        bool SearchBackwards;
        std::vector<string> InstructionSignature;
        byte Patch;
        uint8_t InstructionIndex;
        uint8_t PatchByteCount;
    };
    
    inline std::vector<PatchingTarget> patchingTargets = {
        { "$SB_ERRORBODY_NOT_ATTACHED", true, { "jne" }, 0xEB, 0, 1 },
        { "$SB_ERRORBODY_COUNT", true, { "jbe" }, 0xEB, 0, 1 },
        { "$SB_ERRORBODY_SIZE", true, { "jbe" }, 0xEB, 0, 1 },
        { "$SB_ERRORBODY_SIZE", true, { "ja", "vmulss", "vcomiss", "ja" }, 0x90, 3, 2 },
        { "$SB_ERRORBODY_SIZE", true, { "ja", "vmulss", "vcomiss", "ja" }, 0x90, 0, 2 },
    
        { "$SB_LIMITBODY_MAX_COCKPIT", true, { "jle" }, 0xEB, 0, 1 },
        { "$SB_LIMITBODY_MISSING_COCKPIT", true, { "jge" }, 0xEB, 0, 1 },
        { "$SB_LIMITBODY_MAX_DOCKER", true, { "jle" }, 0xEB, 0, 1 },
        { "$SB_LIMITBODY_MISSING_DOCKER", true, { "jge" }, 0xEB, 0, 1 },
        { "$SB_LIMITBODY_MIN_ENGINE", true, { "jge" }, 0xEB, 0, 1 },
        { "$SB_LIMITBODY_MAX_GRAV_DRIVE", true, { "jle" }, 0xEB, 0, 1 },
        { "$SB_LIMITBODY_MISSING_GRAV_DRIVE", true, { "jge" }, 0xEB, 0, 1 },
        { "$SB_LIMITBODY_MAX_LANDING_BAY", true, { "jle" }, 0xEB, 0, 1 },
        { "$SB_LIMITBODY_MISSING_LANDING_BAY", true, { "jge" }, 0xEB, 0, 1 },
        { "$SB_LIMITBODY_MIN_LANDING_GEAR", true, { "jne" }, 0xEB, 0, 1 },
        { "$SB_ERRORBODY_REACTOR_CLASS", true, { "jne" }, 0xEB, 0, 1 },
        { "$SB_LIMITBODY_MAX_REACTOR", true, { "jle" }, 0xEB, 0, 1 },
        { "$SB_LIMITBODY_MISSING_REACTOR", true, { "jge" }, 0xEB, 0, 1 },
        { "$SB_LIMITBODY_MAX_SHIELD", true, { "jle" }, 0xEB, 0, 1 },
        { "$SB_LIMITBODY_MISSING_SHIELD", true, { "jne" }, 0xEB, 0, 1 },
        { "$SB_LIMITBODY_MIN_FUEL_TANK", true, { "jge" }, 0xEB, 0, 1 },
        { "$SB_LIMITBODY_MAX_WEAPONS", true, { "jle" }, 0xEB, 0, 1 },
        { "$SB_LIMITBODY_MAX_WEAPONS_TYPES", true, { "jle" }, 0xEB, 0, 1 },
    
        { "$SB_LIMITBODY_EXCESS_POWER_ENGINE", true, { "jle" }, 0xEB, 0, 1 },
        { "$SB_LIMITBODY_EXCESS_POWER_WEAPON", true, { "je" }, 0xEB, 0, 1 },
    
        { "$SB_ERRORBODY_SHIP_TOO_HEAVY_TO_GRAVJUMP", true, { "jae" }, 0xEB, 0, 1 },
        { "$SB_LIMITBODY_NOT_ENOUGH_TURN_POWER", true, { "jae" }, 0xEB, 0, 1 },
    
        { "$SB_WEAPONBODY_MISSING_ASSIGNMENT", true, { "jne" }, 0xEB, 0, 1 },
        { "$SB_WEAPONBODY_WEAPONS_MUST_BE_DIFFERENT", true, { "je", "cmp" }, 0xEB, 0, 1},
        { "$SB_WEAPONBODY_WEAPONS_UNASSIGNED", true, { "jle" }, 0xEB, 0, 1 },
    
        { "$SB_ERRORBODY_DOCKER_INVALID_POSITION", true, { "jne" }, 0xEB, 0, 1 },
    
        { "$SB_ERRORBODY_LANDINGENGINE_NOT_ALIGNED_WITH_LANDINGBAY", true, { "jne" }, 0xEB, 0, 1 },
        { "$SB_ERRORBODY_MODULE_BELOW_LANDINGBAY", true, { "jne" }, 0xEB, 0, 1 },
    
        { "$SB_ERRORBODY_LANDINGBAY_CANNOT_REACH_COCKPIT", true, { "je" }, 0x90, 0, 2 },
        { "$SB_ERRORBODY_DOCKER_CANNOT_REACH_COCKPIT", true, { "je" }, 0x90, 0, 2 }
    };

    /* Address -> <Original, Patch> */
    inline std::map<byte*, std::pair<byte, byte>> targetAddresses = {};

    inline bool areLimitationsDisabled = false;

    inline void** scaleformManagerPtr;
    inline void* executeCommandPtr;

    inline int maxShipModulesPrev;
    inline int* maxShipModulesPtr;

}