#define WLR_USE_UNSTABLE

#include <cctype>
#include <ranges>
#include <sstream>
#include <string_view>
#include <unordered_map>

#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/SharedDefs.hpp>
#include <hyprland/src/debug/HyprCtl.hpp>
#include <hyprland/src/helpers/MiscFunctions.hpp>
#include <hyprland/src/managers/XWaylandManager.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>

#include "globals.hpp"

// Do NOT change this function.
APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}

size_t getFirstFreeNumber() {
    size_t i = 1;
    while (g_pGlobalState->harpoonMap.find(std::to_string(i)) != g_pGlobalState->harpoonMap.end()) {
        i++;
    }
    return i;
}

void removeByWindow(CWindow* pWindow) {
    static auto* const PINDICATORSENABLE = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:harpoon:indicators:enable")->getDataStaticPtr();

    for (auto it = g_pGlobalState->harpoonMap.begin(); it != g_pGlobalState->harpoonMap.end();) {
        if (it->second == pWindow)
            it = g_pGlobalState->harpoonMap.erase(it);
        else
            it++;
    }
    if (**PINDICATORSENABLE == 0)
        return;

    if (g_pGlobalState->indicators.find(pWindow) != g_pGlobalState->indicators.end()) {
        auto* pDecoration = g_pGlobalState->indicators[pWindow];
        HyprlandAPI::removeWindowDecoration(PHANDLE, pDecoration);

        g_pGlobalState->indicators.erase(pWindow);
    }
}

void onCloseWindow(void* self, std::any data) {
    auto* const PWINDOW = std::any_cast<CWindow*>(data);
    removeByWindow(PWINDOW);
}

void onSubmapChange(void* self, std::any data) {
    const auto& sSubmap = std::any_cast<const std::string&>(data);

    if (sSubmap == "harpoon_select") {
        for (const auto& [_, indicatorDecoration] : g_pGlobalState->indicators) {
            indicatorDecoration->toggleVisibility(true);
        }

        g_pGlobalState->indicatorsVisible = 1;

    } else if (sSubmap.empty() && g_pGlobalState->indicatorsVisible == 1) {
        for (const auto& [_, indicatorDecoration] : g_pGlobalState->indicators) {
            indicatorDecoration->toggleVisibility(false);
        }

        g_pGlobalState->indicatorsVisible = 0;
    }
}

std::tuple<std::string, std::string> splitDispatcherParams(const std::string str) {
    std::string op;
    std::string params;

    auto        pos = str.find(' ');
    if (pos != std::string::npos) {
        op     = str.substr(0, pos);
        params = str.substr(pos + 1);
    } else {
        op = str;
    }

    return {op, params};
}

void harpoonDispatcher(std::string params) {
    static auto* const PINDICATORSENABLE = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:harpoon:indicators:enable")->getDataStaticPtr();

    auto [op, identifier] = splitDispatcherParams(params);

    if (g_pCompositor->m_bUnsafeState)
        return;

    if (op == "add") {
        if (identifier.empty())
            identifier = std::to_string(getFirstFreeNumber());

        if (g_pCompositor->m_vWindowFocusHistory.empty())
            return;

        CWindow* pLastWindow = g_pCompositor->m_vWindowFocusHistory[0];

        // Already added
        if (g_pGlobalState->harpoonMap.find(identifier) != g_pGlobalState->harpoonMap.end()) {
            if (g_pGlobalState->harpoonMap[identifier] == pLastWindow)
                return;

            CWindow* pOldWindow = g_pGlobalState->harpoonMap[identifier];

            if (**PINDICATORSENABLE == 1) {
                // Remove old indicators
                if (g_pGlobalState->indicators.find(pOldWindow) != g_pGlobalState->indicators.end())
                    g_pGlobalState->indicators[pOldWindow]->removeIdentifier(identifier);
            }
        }

        g_pGlobalState->harpoonMap[identifier] = pLastWindow;

        if (**PINDICATORSENABLE == 0)
            return;

        if (g_pGlobalState->indicators.find(pLastWindow) == g_pGlobalState->indicators.end()) {
            std::vector<std::string> vIdentifiers   = {identifier};
            auto                     pIndicator     = std::make_unique<CHarpoonIndicators>(pLastWindow, vIdentifiers);
            g_pGlobalState->indicators[pLastWindow] = pIndicator.get();

            HyprlandAPI::addWindowDecoration(PHANDLE, pLastWindow, std::move(pIndicator));
        } else {
            // This is a bit scary, because the lifetime of the indicator is not guaranteed
            g_pGlobalState->indicators[pLastWindow]->addIdentifier(identifier);
        }

    } else if (op == "select") {
        if (identifier.empty())
            return;

        if (g_pGlobalState->harpoonMap.find(identifier) == g_pGlobalState->harpoonMap.end())
            return;

        CWindow* pWindow = g_pGlobalState->harpoonMap[identifier];

        if (!g_pCompositor->windowExists(pWindow)) {
            // awkward better remove it
            removeByWindow(pWindow);
            return;
        }

        if (!g_pCompositor->windowValidMapped(pWindow))
            return;

        g_pCompositor->focusWindow(pWindow);

    } else if (op == "remove") {
        if (identifier.empty())
            return;

        CWindow* pWindow = g_pGlobalState->harpoonMap[identifier];

        if (**PINDICATORSENABLE == 1) {
            if (g_pGlobalState->indicators.find(pWindow) != g_pGlobalState->indicators.end()) {
                g_pGlobalState->indicators[pWindow]->removeIdentifier(identifier);
            }
        }

        g_pGlobalState->harpoonMap.erase(identifier);
    }
}

std::string doListCommand(eHyprCtlOutputFormat format, std::string _) {
    if (format == eHyprCtlOutputFormat::FORMAT_NORMAL) {
        std::stringstream ss;
        for (auto& [identifier, pWindow] : g_pGlobalState->harpoonMap) {
            if (pWindow == nullptr || !g_pCompositor->windowExists(pWindow))
                continue;

            ss << identifier << " " << g_pXWaylandManager->getTitle(pWindow) << "\n";
        }
        return ss.str();
    } else if (format == eHyprCtlOutputFormat::FORMAT_JSON) {
        return "Not implemented yet!";
    } else {
        return "Invalid format!";
    }
}

static inline std::string invokeHyprctlCommandChecked(const std::string& call, const std::string& args) {
    std::string sResult = HyprlandAPI::invokeHyprctlCommand(call, args);
    if (sResult != "ok") {
        HyprlandAPI::addNotification(PHANDLE, "[Harpoon] Hyprctl command failed: " + call + " " + args, CColor(0), 5000);
    }
    return sResult;
}

void onConfigReloaded() {
    static auto* const PKEYS         = (Hyprlang::STRING const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:harpoon:keys")->getDataStaticPtr();
    static auto* const PKEY_SEL      = (Hyprlang::STRING const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:harpoon:select_trigger")->getDataStaticPtr();
    static auto* const PKEY_ADD      = (Hyprlang::STRING const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:harpoon:add_trigger")->getDataStaticPtr();
    static auto* const PKEY_QUICKADD = (Hyprlang::STRING const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:harpoon:quickadd")->getDataStaticPtr();

    const std::string  sKeys{*PKEYS};
    const std::string  sSel{*PKEY_SEL};
    const std::string  sAdd{*PKEY_ADD};
    const std::string  sQuickAdd{*PKEY_QUICKADD};

    if (!sQuickAdd.empty()) {
        invokeHyprctlCommandChecked("keyword", "unbind " + sQuickAdd);
        invokeHyprctlCommandChecked("keyword", "bind " + sQuickAdd + ",harpoon,add");
    }

    invokeHyprctlCommandChecked("keyword", "unbind " + sAdd);
    invokeHyprctlCommandChecked("keyword", "bind " + sAdd + ",submap,harpoon_add");
    invokeHyprctlCommandChecked("keyword", "submap harpoon_add");

    invokeHyprctlCommandChecked("keyword", "bind " + sAdd + ",submap,reset");
    invokeHyprctlCommandChecked("keyword", "bind ,Escape,submap,reset");

    for (const char cKey : sKeys) {
        std::stringstream ssBindKey;
        std::stringstream ssBindReset;

        ssBindKey << "bind ," << cKey << ",harpoon,add " << cKey;
        ssBindReset << "bind ," << cKey << ",submap,reset";

        invokeHyprctlCommandChecked("keyword", ssBindKey.str());
        invokeHyprctlCommandChecked("keyword", ssBindReset.str());
    }

    invokeHyprctlCommandChecked("keyword", "submap reset");

    invokeHyprctlCommandChecked("keyword", "unbind " + sSel);
    invokeHyprctlCommandChecked("keyword", "bind " + sSel + ",submap,harpoon_select");
    invokeHyprctlCommandChecked("keyword", "submap harpoon_select");

    invokeHyprctlCommandChecked("keyword", "bind " + sSel + ",submap,reset");
    invokeHyprctlCommandChecked("keyword", "bind ,Escape,submap,reset");

    for (const char cKey : sKeys) {
        std::stringstream ssBindKey;
        std::stringstream ssBindReset;

        ssBindKey << "bind ," << cKey << ",harpoon,select " << cKey;
        ssBindReset << "bind ," << cKey << ",submap,reset";

        invokeHyprctlCommandChecked("keyword", ssBindKey.str());
        invokeHyprctlCommandChecked("keyword", ssBindReset.str());
    }

    invokeHyprctlCommandChecked("keyword", "submap reset");
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    PHANDLE = handle;

    g_pGlobalState = std::make_unique<SGlobalState>();

    HyprlandAPI::addConfigValue(PHANDLE, "plugin:harpoon:keys", Hyprlang::STRING{"123456789qwert"});
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:harpoon:select_trigger", Hyprlang::STRING{"SHIFT,Space"});
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:harpoon:add_trigger", Hyprlang::STRING{"CONTROL_SHIFT,Space"});
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:harpoon:quickadd", Hyprlang::STRING{""});

    HyprlandAPI::addConfigValue(PHANDLE, "plugin:harpoon:indicators:enable", Hyprlang::INT{1});
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:harpoon:indicators:height", Hyprlang::INT{24});
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:harpoon:indicators:width", Hyprlang::INT{20});
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:harpoon:indicators:margin", Hyprlang::INT{4});
    HyprlandAPI::addConfigValue(PHANDLE, "plugin:harpoon:indicators:rounding", Hyprlang::INT{2});

    HyprlandAPI::addDispatcher(PHANDLE, "harpoon", &harpoonDispatcher);

    HyprlandAPI::reloadConfig();

    HyprlandAPI::registerCallbackDynamic(PHANDLE, "closeWindow", [&](void* self, SCallbackInfo& info, std::any data) { onCloseWindow(self, data); });
    HyprlandAPI::registerCallbackDynamic(PHANDLE, "configReloaded", [&](void* self, SCallbackInfo& info, std::any data) { onConfigReloaded(); });
    HyprlandAPI::registerCallbackDynamic(PHANDLE, "submap", [&](void* self, SCallbackInfo& info, std::any data) { onSubmapChange(self, data); });

    SHyprCtlCommand harpoonListCommand = {"harpoonlist", true, &doListCommand};
    HyprlandAPI::registerHyprCtlCommand(PHANDLE, harpoonListCommand);

    HyprlandAPI::addNotification(PHANDLE, "[Harpoon] Loaded successfully!", CColor(0), 5000);

    return {"Hyprland Harpoon", "(ph/f)ishing for windows", "PaideiaDilemma", "0.1"};
}

APICALL EXPORT void PLUGIN_EXIT() {
    HyprlandAPI::addNotification(PHANDLE, "[Harpoon] Unloaded successfully!", CColor(0), 5000);
}
