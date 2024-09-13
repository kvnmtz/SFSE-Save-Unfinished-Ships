#pragma once

#include "includes.h"

namespace Global
{

    inline std::set<string> patchingTargets = {
        { "$SB_LIMITHEADER_MAX_COCKPIT" },
        { "$SB_LIMITHEADER_MISSING_DOCKER" },
        { "$SB_LIMITHEADER_MISSING_COCKPIT" },
        { "$SB_LIMITHEADER_MAX_DOCKER" },
        { "$SB_LIMITHEADER_MIN_ENGINE" },
        { "$SB_LIMITHEADER_MISSING_GRAV_DRIVE" },
        { "$SB_LIMITHEADER_MAX_GRAV_DRIVE" },
        { "$SB_LIMITHEADER_MISSING_LANDING_BAY" },
        { "$SB_LIMITHEADER_MAX_LANDING_BAY" },
        { "$SB_LIMITHEADER_MIN_LANDING_GEAR" },
        { "$SB_LIMITHEADER_MISSING_REACTOR" },
        { "$SB_LIMITHEADER_MAX_REACTOR" },
        { "$SB_ERRORHEADER_REACTOR_CLASS" },
        { "$SB_LIMITHEADER_MAX_SHIELD" },
        { "$SB_LIMITHEADER_MIN_FUEL_TANK" },
        { "$SB_LIMITBODY_MAX_WEAPONS" },
        { "$SB_LIMITBODY_MAX_WEAPONS_TYPES" },
        { "$SB_LIMITHEADER_MAX_VEHICLE" },
        { "$SB_ERRORHEADER_NOT_ATTACHED" },
        { "$SB_ERRORHEADER_COUNT" },
        { "$SB_ERRORHEADER_SIZE" },
        { "$SB_LIMITHEADER_EXCESS_POWER_ENGINE" },
        { "$SB_LIMITHEADER_EXCESS_POWER_WEAPON" },
        { "$SB_ERRORHEADER_SHIP_TOO_HEAVY_TO_GRAVJUMP" },
        { "$SB_WEAPONHEADER_WEAPONS_UNASSIGNED" },
        { "$SB_ERRORHEADER_DOCKER_INVALID_POSITION" },
        { "$SB_ERRORHEADER_LANDINGBAY_CANNOT_REACH_COCKPIT" },
        { "$SB_ERRORHEADER_DOCKER_CANNOT_REACH_COCKPIT" },
        { "$SB_ERRORHEADER_LANDINGENGINE_NOT_ALIGNED_WITH_LANDINGBAY" },
        { "$SB_ERRORHEADER_MODULE_BELOW_LANDINGBAY" },
    };

    /* Address -> <Original, Patch> */
    inline std::unordered_map<byte*, std::pair<byte, byte>> targetAddresses = {};

    inline bool areLimitationsDisabled = false;

    inline void** scaleformManagerPtr;
    inline void* executeCommandPtr;

    inline int maxShipModulesPrev;
    inline int* maxShipModulesPtr;

}