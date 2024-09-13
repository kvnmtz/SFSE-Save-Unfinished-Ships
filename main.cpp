#include "global.h"
#include "init.h"
#include "log.h"
#include "utils.h"

#pragma region SFSE_SETUP_CODE
#define MAKE_EXE_VERSION_EX(major, minor, build, sub) ((((major) & 0xFF) << 24) | (((minor) & 0xFF) << 16) | (((build) & 0xFFF) << 4) | ((sub) & 0xF))
#define MAKE_EXE_VERSION(major, minor, build) MAKE_EXE_VERSION_EX(major, minor, build, 0)

typedef uint32_t PluginHandle;

#define RUNTIME_VERSION_1_7_36 MAKE_EXE_VERSION(1, 7, 36)

struct SFSEPluginVersionData
{
    enum
    {
        kVersion = 1,
    };

    // describe how you find your addresses
    enum
    {
        // signature matching only
        kAddressIndependence_Signatures = 1 << 0,

        // Address Library v1 (https://www.nexusmods.com/starfield/mods/3256)
        kAddressIndependence_AddressLibrary = 1 << 1,
    };

    // describe your structure compatibility
    enum
    {
        // doesn't use any game structures
        kStructureIndependence_NoStructs = 1 << 0,

        // works with the structure layout the game shipped with
        kStructureIndependence_InitialLayout = 1 << 1,

        // additional bits will be added here when breaking changes happen
    };

    std::uint32_t    dataVersion;            // set to kVersion

    std::uint32_t    pluginVersion;          // version number of your plugin
    char             name[256];              // null-terminated ASCII plugin name (please make this recognizable to users)
    char             author[256];            // null-terminated ASCII plugin author name

    // version compatibility
    std::uint32_t    addressIndependence;    // bitfield. describe how you find your addresses using the kAddressIndependence_ enums
    std::uint32_t    structureIndependence;  // bitfield. describe how you handle structure layout using the kStructureIndependence_ enums
    std::uint32_t    compatibleVersions[16]; // zero-terminated list of RUNTIME_VERSION_ defines your plugin is compatible with

    std::uint32_t    seVersionRequired;      // minimum version of the script extender required, compared against PACKED_SFSE_VERSION
    // you probably should just set this to 0 unless you know what you are doing
    std::uint32_t    reservedNonBreaking;    // bitfield. set to 0
    std::uint32_t    reservedBreaking;       // bitfield. set to 0
};

typedef struct SFSEPluginInfo_t
{
    uint32_t infoVersion;
    const char* name;
    uint32_t version;
} SFSEPluginInfo;

typedef struct SFSEInterface_t
{
    uint32_t sfseVersion;
    uint32_t runtimeVersion;
    uint32_t interfaceVersion;
    void* (*QueryInterface) (uint32_t id);
    PluginHandle(*GetPluginHandle) (void);
    SFSEPluginInfo* (*GetPluginInfo) (const char* name);
} SFSEInterface;

typedef struct SFSEMessage_t
{
    const char* sender;
    uint32_t type;
    uint32_t dataLen;
    void* data;
} SFSEMessage;

typedef void (*SFSEMessageEventCallback) (SFSEMessage* msg);

typedef struct SFSEMessagingInterface_t
{
    uint32_t interfaceVersion;
    bool (*RegisterListener) (PluginHandle listener, const char* sender, SFSEMessageEventCallback handler);
    bool (*Dispatch) (PluginHandle sender, uint32_t messageType, void* data, uint32_t dataLen, const char* receiver);
} SFSEMessagingInterface;

static void OnDelayLoad();

static DWORD ThreadProc_OnDelayLoad(LPVOID unused)
{
    (void)unused;
    Sleep(8000); // a reasonable amount of time for the game to initialize
    OnDelayLoad();
    return 0;
}

static void MessageEventCallback(SFSEMessage* msg)
{
    if (msg->type != 0 /* postload */)
        return;
    
    CreateThread(nullptr, 4096, ThreadProc_OnDelayLoad, nullptr, 0, nullptr);
}

extern "C" __declspec(dllexport) void SFSEPlugin_Preload(const SFSEInterface * sfse)
{
    const PluginHandle pluginHandle = sfse->GetPluginHandle();
    const auto msg = static_cast<SFSEMessagingInterface*>(sfse->QueryInterface(1 /* messaging interface */));
    msg->RegisterListener(pluginHandle, "SFSE", MessageEventCallback);
}
#pragma endregion SFSE Setup Code

extern "C" __declspec(dllexport) SFSEPluginVersionData SFSEPlugin_Version = {
    SFSEPluginVersionData::kVersion,

    1,
    "Save Unfinished Ships",
    "3xpl01t",

    SFSEPluginVersionData::kAddressIndependence_Signatures,
    SFSEPluginVersionData::kStructureIndependence_NoStructs,
    { RUNTIME_VERSION_1_7_36, 0 },

    0,

    0,
    0
};

void ExecuteCommand(const string& command)
{
    const auto func = reinterpret_cast<void(*)(void*, const char*)>(Global::executeCommandPtr);
    func(*Global::scaleformManagerPtr, command.c_str());
}

void DisableLimitations()
{
    for (const auto& [address, origAndPatchPair] : Global::targetAddresses)
    {
        WriteToReadOnlyMemory(address, origAndPatchPair.second);
    }

    Global::maxShipModulesPrev = *Global::maxShipModulesPtr;
    *Global::maxShipModulesPtr = INT_MAX;

    ExecuteCommand(R"(cgf "Debug.Notification" "Ship limitations disabled.")");
}

void EnableLimitations()
{
    for (const auto& [address, origAndPatchPair] : Global::targetAddresses)
    {
        WriteToReadOnlyMemory(address, origAndPatchPair.first);
    }

    *Global::maxShipModulesPtr = Global::maxShipModulesPrev;

    ExecuteCommand(R"(cgf "Debug.Notification" "Ship limitations restored.")");
}

static void OnDelayLoad()
{
    if (!SetupLog())
        return;

    Log("Module base @ 0x{:X}", reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr)));

    if (!InitCapstone())
        return;

    if (!FindPatchingTargets())
        return;

    if (!FindGlobals())
        return;

    if (!FindFunctions())
        return;

    Log("Initialization complete");

    while (true)
    {
        do
        {
            const bool isF6Pressed = GetAsyncKeyState(VK_F6);
            if (!isF6Pressed)
                break;

            if (Global::areLimitationsDisabled)
            {
                EnableLimitations();
            }
            else
            {
                DisableLimitations();
            }

            Global::areLimitationsDisabled = !Global::areLimitationsDisabled;
        } while (false);
        Sleep(100);
    }
}