#pragma once

#include "includes.h"

namespace Global
{
    inline std::map<string, byte> jmpPatchingTargets = {
        { "$SB_ERRORBODY_NOT_ATTACHED", 0x75 },
        { "$SB_ERRORBODY_COUNT", 0x76 },
        { "$SB_ERRORBODY_SIZE", 0x76 },

        { "$SB_LIMITBODY_MAX_COCKPIT", 0x7E },
        { "$SB_LIMITBODY_MISSING_COCKPIT", 0x7D },
        { "$SB_LIMITBODY_MAX_DOCKER", 0x7E },
        { "$SB_LIMITBODY_MISSING_DOCKER", 0x7D },
        { "$SB_LIMITBODY_MIN_ENGINE", 0x7D },
        { "$SB_LIMITBODY_MAX_GRAV_DRIVE", 0x7E },
        { "$SB_LIMITBODY_MISSING_GRAV_DRIVE", 0x7D },
        { "$SB_LIMITBODY_MAX_LANDING_BAY", 0x7E },
        { "$SB_LIMITBODY_MISSING_LANDING_BAY", 0x7D },
        { "$SB_LIMITBODY_MIN_LANDING_GEAR", 0x75 },
        { "$SB_ERRORBODY_REACTOR_CLASS", 0x75 },
        { "$SB_LIMITBODY_MAX_REACTOR", 0x7E },
        { "$SB_LIMITBODY_MISSING_REACTOR", 0x7D },
        { "$SB_LIMITBODY_MAX_SHIELD", 0x7E },
        { "$SB_LIMITBODY_MISSING_SHIELD", 0x75 },
        { "$SB_LIMITBODY_MIN_FUEL_TANK", 0x7D },
        { "$SB_LIMITBODY_MAX_WEAPONS", 0x7E },
        { "$SB_LIMITBODY_MAX_WEAPONS_TYPES", 0x7E },

        { "$SB_LIMITBODY_EXCESS_POWER_ENGINE", 0x7E },
        { "$SB_LIMITBODY_EXCESS_POWER_WEAPON", 0x74 },

        { "$SB_ERRORBODY_SHIP_TOO_HEAVY_TO_GRAVJUMP", 0x73 },
        { "$SB_LIMITBODY_NOT_ENOUGH_TURN_POWER", 0x73 },

        { "$SB_WEAPONBODY_MISSING_ASSIGNMENT", 0x75 },
        { "$SB_WEAPONBODY_WEAPONS_MUST_BE_DIFFERENT", 0x74 },
        { "$SB_WEAPONBODY_WEAPONS_UNASSIGNED", 0x7E },

        { "$SB_ERRORBODY_DOCKER_INVALID_POSITION", 0x75 },

        { "$SB_ERRORBODY_LANDINGENGINE_NOT_ALIGNED_WITH_LANDINGBAY", 0x75 },
        { "$SB_ERRORBODY_MODULE_BELOW_LANDINGBAY", 0x75 }
    };

    inline std::map<string, string> nopPatchingTargets = {
        { "$SB_ERRORBODY_LANDINGBAY_CANNOT_REACH_COCKPIT", "je" },
        { "$SB_ERRORBODY_DOCKER_CANNOT_REACH_COCKPIT", "je" }
    };

    /* Address -> <Original, Patch> */
    inline std::map<byte*, std::pair<byte, byte>> targetAddresses = {};

    inline bool areLimitationsDisabled = false;

    inline void** scaleformManagerPtr;
    inline void* executeCommandPtr;

}