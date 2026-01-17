#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <stdio.h>
#include <string>
#include <vector>
#include <fstream>
#include <chrono>
#include <ctime>
#include <stdexcept>

uintptr_t OFFSET_GWORLD = 0;
uintptr_t OFFSET_GNAMES = 0;

uintptr_t OFFSET_GWORLD_DEFAULT = 0x3EAF350;
uintptr_t OFFSET_GNAMES_DEFAULT = 0x3DAE900;

std::string IN_TITLE = "PL_Advertise";
std::string IN_MENU = "PL_Menu";
std::string IN_BOOT = "PL_Boot";
std::string IN_RACE = "PL_Course";

int PORT_DEFAULT = 5555;

SOCKET clientSocket;
sockaddr_in serverAddr;

void InitializeConsole() {
    AllocConsole();

    freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
    freopen_s((FILE**)stdin, "CONIN$", "r", stdin);
    freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);

}

void InitSocket(int port) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    clientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
}

void SendUpdate(std::string msg) {
    sendto(clientSocket, msg.c_str(), (int)msg.length(), 0, (sockaddr*)&serverAddr, sizeof(serverAddr));
}

std::string configPath(){
    HMODULE hModule = NULL;
    char dllPath[MAX_PATH];
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | 
                       GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       (LPCSTR)&configPath, &hModule);

    GetModuleFileNameA(hModule, dllPath, MAX_PATH);

    std::string pathStr(dllPath);
    size_t lastSlash = pathStr.find_last_of("\\/");
    std::string finalConfigPath = pathStr.substr(0, lastSlash) + "\\config.ini";
    return finalConfigPath;
}

int ReadPortFromConfig() {
    char buffer[255];
    std::string configFile = configPath();
    
    DWORD result = GetPrivateProfileStringA(
        "socket", 
        "port", 
        std::to_string(PORT_DEFAULT).c_str(), 
        buffer, 
        sizeof(buffer), 
        configFile.c_str()
    ); 
    try {
        return std::stoi(std::string(buffer));
    } 
    catch (const std::invalid_argument& e) {
        return PORT_DEFAULT;
    } 
    catch (const std::out_of_range& e) {
        return PORT_DEFAULT;
    }
}

uintptr_t ReadHexFromIni(const std::string& section, const std::string& key, uintptr_t defaultValue) {
    char buffer[255];
    std::string configFile = configPath();

    DWORD result = GetPrivateProfileStringA(
        section.c_str(), 
        key.c_str(), 
        "0", 
        buffer, 
        sizeof(buffer), 
        configFile.c_str()
    );

    if (result == 0 || std::string(buffer) == "0") {
        return defaultValue;
    }

    try {
        return (uintptr_t)std::stoull(buffer, nullptr, 16);
    } 
    catch (const std::exception&) {
        return defaultValue;
    }
}

struct FNameEntryHeader {
    uint16_t bIsWide : 1;
    uint16_t LowercaseProbeHash : 5;
    uint16_t Len : 10;
};

struct FNameEntry {
    FNameEntryHeader Header;
    union {
        char AnsiName[1024];
        wchar_t WideName[1024];
    };

    std::string ToString() {
        if (Header.bIsWide) return "WideName_Unsupported";
        return std::string(AnsiName, Header.Len);
    }
};

struct TArray { 
    uintptr_t Data; 
    int32_t Count; 
    int32_t Max; 
};

std::string GetNameFromPool(uint32_t id) {
    uintptr_t moduleBase = (uintptr_t)GetModuleHandle(NULL);
    uintptr_t GNamesPool = moduleBase + OFFSET_GNAMES;

    // split ID into block and offset
    uint32_t block = id >> 16;
    uint32_t offset = id & 65535;

    uintptr_t blockPtr = *(uintptr_t*)(GNamesPool + 0x10 + (block * 8));
    if (!blockPtr) return "ERR_BLOCK";

    uintptr_t entryAddr = blockPtr + (offset * 2);
    FNameEntry* entry = (FNameEntry*)entryAddr;
    
    return entry->ToString();
}

#pragma pack(push, 1)
struct FRaceDataBlock {
    unsigned char CourseID;   // +0x00
    unsigned char Direction;  // +0x01
    unsigned char Day;        // +0x02
    unsigned char Weather;    // +0x03
    unsigned char GameMode;   // +0x04
    unsigned char RaceState;  // +0x05
};
#pragma pack(pop)

bool IsValid(uintptr_t ptr) {
    return (ptr >= 0x10000 && ptr < 0x00007FFFFFFFFFFF);
}

template <typename T>
bool SafeRead(uintptr_t remote_address, T& out_value) {
    if (!IsValid(remote_address)) return false;
    SIZE_T bytesRead;
    if (ReadProcessMemory(GetCurrentProcess(), (LPCVOID)remote_address, &out_value, sizeof(T), &bytesRead)) {
        return bytesRead == sizeof(T);
    }
    return false;
}

std::string GetRootLevelName(uintptr_t gWorld) {
    // getting root level name
    // need for checking if we in race now or not
    // if PL_Course - we in race, otherwise - we are not

    uintptr_t persistentLevel = 0;
    if (!SafeRead(gWorld + 0x30, persistentLevel) || !IsValid(persistentLevel)) return "ERR";

    uintptr_t levelOuter = 0;
    if (!SafeRead(persistentLevel + 0x20, levelOuter) || !IsValid(levelOuter)) return "ERR";

    uint32_t nameID = 0;
    if (!SafeRead(levelOuter + 0x18, nameID)) return "ERR";

    return GetNameFromPool(nameID);
}

std::string GetModeFromEnum(uintptr_t uWorld) {
    // gWorld -> gameInstance -> loadingWidget -> bpvMode
    // enum, representing game mode
    // aquiring it via loadingWidget kinda messy, better find another way

    if (!IsValid(uWorld)) return "0";

    uintptr_t gameInstance = *(uintptr_t*)(uWorld + 0x170);
    if (!IsValid(gameInstance)) return "0";

    uintptr_t loadingWidget = *(uintptr_t*)(gameInstance + 0x318);
    if (!IsValid(loadingWidget)) return "0"; 

    unsigned char modeVal = *(unsigned char*)(loadingWidget + 0x3E0);

    return std::to_string((int)modeVal);
}

std::string GetRaceSettingsRaw(uintptr_t uWorld) {
    // gWorld -> gameMode -> raceSystem -> raceSetting
    // raceSetting is incapsulated, reading directly from memory
    // 0x00 - 0x30 - uObject headers
    // 0x30 - 0x50 - five references to class values

    const std::string default_val = "0,0,0,0,0,0";

    if (!IsValid(uWorld)) {
        return default_val;
    }

    uintptr_t gameMode = 0;
    if (!SafeRead(uWorld + 0x128, gameMode) || !IsValid(gameMode)) {
        return default_val;
    }

    uintptr_t raceSystem = 0;
    if (!SafeRead(gameMode + 0x330, raceSystem) || !IsValid(raceSystem)) {
        return default_val;
    }

    uintptr_t raceSettingObj = 0;
    if (!SafeRead(raceSystem + 0x3E8, raceSettingObj) || !IsValid(raceSettingObj)) {
        return default_val;
    }

    uintptr_t dataBlockPtr = 0;
    if (!SafeRead(raceSettingObj + 0x30, dataBlockPtr) || !IsValid(dataBlockPtr)) {
        return default_val;
    }

    FRaceDataBlock localData = { 0 };
    if (!SafeRead(dataBlockPtr, localData)) {
        return default_val;
    }

    char buf[128];
    snprintf(buf, sizeof(buf), "%u,%u,%u,%u,%u,%u", 
        (unsigned int)localData.CourseID, 
        (unsigned int)localData.Direction, 
        (unsigned int)localData.Day, 
        (unsigned int)localData.Weather, 
        (unsigned int)localData.GameMode, 
        (unsigned int)localData.RaceState);

    return std::string(buf);
}

DWORD WINAPI MainThread(LPVOID lpReserved) {
    int port = ReadPortFromConfig();
    OFFSET_GWORLD = ReadHexFromIni("offsets", "GWorld", OFFSET_GWORLD_DEFAULT);
    OFFSET_GNAMES = ReadHexFromIni("offsets", "GNames", OFFSET_GNAMES_DEFAULT);

    InitSocket(port);

    uintptr_t moduleBase = (uintptr_t)GetModuleHandle(NULL);
    std::string lastSent = "";

    while (true) {
        uintptr_t* GWorldPtr = (uintptr_t*)(moduleBase + OFFSET_GWORLD);
        if (GWorldPtr && *GWorldPtr) {
            uintptr_t uWorld = *GWorldPtr;
            
            std::string rootLevelName = GetRootLevelName(uWorld);
            int stateID = -1;

            if (rootLevelName == IN_BOOT)      stateID = 0;
            else if (rootLevelName == IN_TITLE) stateID = 1;
            else if (rootLevelName == IN_MENU)  stateID = 3;
            else if (rootLevelName == IN_RACE)  stateID = 4;

            std::string raceSettings = GetRaceSettingsRaw(uWorld);
            std::string gameMode = GetModeFromEnum(uWorld);
            
            std::string combined = std::to_string(stateID) + "," + gameMode + "," + raceSettings;

            if (combined != lastSent) {
                SendUpdate(combined);
                lastSent = combined;
            }
        }

        if (GetAsyncKeyState(VK_END) & 1) break;
        Sleep(2000);
    }

    closesocket(clientSocket);
    WSACleanup();
    FreeLibraryAndExitThread((HMODULE)lpReserved, 0);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID res) {
    if (reason == DLL_PROCESS_ATTACH) CreateThread(NULL, 0, MainThread, hModule, 0, NULL);
    return TRUE;
}