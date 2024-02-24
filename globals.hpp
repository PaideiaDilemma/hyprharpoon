#pragma once

#include <hyprland/src/plugins/PluginAPI.hpp>
#include "indicators.hpp"

inline HANDLE PHANDLE = nullptr;

struct SGlobalState {
    std::unordered_map<std::string, CWindow*>         harpoonMap;
    std::unordered_map<CWindow*, CHarpoonIndicators*> indicators;
    //

    bool indicatorsVisible = false;
};

inline std::unique_ptr<SGlobalState> g_pGlobalState;
