#define WLR_USE_UNSTABLE

#include <algorithm>
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/desktop/Window.hpp>
#include <hyprland/src/helpers/Color.hpp>
#include <hyprland/src/render/OpenGL.hpp>
#include <hyprland/src/render/decorations/IHyprWindowDecoration.hpp>
#include <pango/pangocairo.h>

#include "indicators.hpp"
#include "globals.hpp"

CHarpoonIndicators::CHarpoonIndicators(CWindow* pWindow, const std::vector<std::string>& identifiers) :
    IHyprWindowDecoration(pWindow), m_pWindow(pWindow), m_eLayer(DECORATION_LAYER_UNDER) {
    m_vLastWindowPos  = pWindow->m_vRealPosition.value();
    m_vLastWindowSize = pWindow->m_vRealSize.value();

    for (const auto& identifier : identifiers) {
        m_indicators.push_back(SHarpoonIndicator{identifier});
    }
}

CHarpoonIndicators::~CHarpoonIndicators() {
    damageEntire();
}

SDecorationPositioningInfo CHarpoonIndicators::getPositioningInfo() {
    static auto* const         PHEIGHT = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:harpoon:indicators:height")->getDataStaticPtr();
    const auto                 height  = static_cast<double>(**PHEIGHT);

    SDecorationPositioningInfo info;
    info.edges    = DECORATION_EDGE_BOTTOM;
    info.policy   = DECORATION_POSITION_STICKY;
    info.reserved = false;

    info.desiredExtents = {{0, 0}, {0, height}};

    return info;
}

void CHarpoonIndicators::onPositioningReply(const SDecorationPositioningReply& reply) {
    ; // ignored
}

uint64_t CHarpoonIndicators::getDecorationFlags() {
    return 0;
}

eDecorationLayer CHarpoonIndicators::getDecorationLayer() {
    return m_eLayer;
}

std::string CHarpoonIndicators::getDisplayName() {
    return "Harpoon Indicator";
}

void CHarpoonIndicators::draw(CMonitor* pMonitor, float a, const Vector2D& offset) {
    if (!g_pCompositor->windowValidMapped(m_pWindow))
        return;

    if (!m_pWindow->m_sSpecialRenderData.decorate)
        return;

    if (m_indicators.empty())
        return;

    static auto* const PROUNDING   = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(PHANDLE, "decoration:rounding")->getDataStaticPtr();
    static auto* const PBORDERSIZE = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(PHANDLE, "general:border_size")->getDataStaticPtr();

    static auto* const PWIDTH    = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:harpoon:indicators:width")->getDataStaticPtr();
    static auto* const PHEIGHT   = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:harpoon:indicators:height")->getDataStaticPtr();
    static auto* const PMARGIN   = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:harpoon:indicators:margin")->getDataStaticPtr();
    static auto* const PROUNDIND = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:harpoon:indicators:rounding")->getDataStaticPtr();

    /*static std::vector<Hyprlang::INT* const*> PCOLORS;
    for (size_t i = 0; i < 9; ++i) {
        PCOLORS.push_back((Hyprlang::INT* const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:harpoon:col.indicator_" + std::to_string(i + 1))->getDataStaticPtr());
    }*/

    const auto scale = pMonitor->scale;

    const auto width        = static_cast<double>(**PWIDTH);
    const auto height       = static_cast<double>(**PHEIGHT);
    const auto scaledWidth  = width * scale;
    const auto scaledHeight = height * scale;

    const auto scaledMargin = **PMARGIN * scale;

    const auto PWORKSPACE      = g_pCompositor->getWorkspaceByID(m_pWindow->m_iWorkspaceID);
    const auto WORKSPACEOFFSET = PWORKSPACE && !m_pWindow->m_bPinned ? PWORKSPACE->m_vRenderOffset.value() : Vector2D();

    const auto rounding = **PROUNDIND == 0 ? 0 : **PROUNDIND * scale;

    // We want to loop over all indicators associated with this window and draw small labels in our decoration area.
    const auto fullDecoWidth = scaledWidth * m_indicators.size() + scaledMargin * m_indicators.size();
    double     x             = m_vLastWindowPos.x + m_vLastWindowSize.x / 2 - fullDecoWidth / 2;

    auto       drawIndicator = [&](SHarpoonIndicator& indicator) -> void {
        // Too many labels to fit. TODO: think about how to handle this
        if (x + scaledWidth > m_vLastWindowPos.x + m_vLastWindowSize.x)
            return;

        CBox identifierBox = {x, m_vLastWindowPos.y, width, height};
        identifierBox.scale(scale);

        identifierBox.translate({0, m_vLastWindowSize.y - scaledHeight / 1.3}); // TODO: configurable ratio or pixels

        identifierBox.translate(offset - pMonitor->vecPosition + WORKSPACEOFFSET);

        x += scaledWidth + scaledMargin;

        if (identifierBox.width < 1 || identifierBox.height < 1)
            return;

        // TODO: color, round, blur (use renderRectWithBlur)
        g_pHyprOpenGL->renderRect(&identifierBox, CColor{255.0 / 255.0, 204 / 255.0, 102 / 255.0, 1.0}, rounding);

        if (indicator.texture.m_iTexID == 0 /* texture is not rendered */) {
            const Vector2D BUFSIZE = {scaledWidth, scaledHeight};
            //TODO: COLORS, PROPER FONT SIZE, that scaled with the available space
            const CColor textColor{0, 0, 0, 1};
            renderText(indicator.texture, indicator.identifierText, textColor, BUFSIZE, scale, width * 0.5);
        }

        CBox textPos{identifierBox};
        textPos.translate({0, -scaledHeight * (1 - 1 / 1.3)}); // Make sure the text is within the window

        g_pHyprOpenGL->renderTexture(indicator.texture, &textPos, a);
    };

    g_pHyprOpenGL->scissor((CBox*)nullptr);

    for (auto& indicator : m_indicators) {
        drawIndicator(indicator);
    }

    g_pHyprOpenGL->scissor((CBox*)nullptr);

    /*CBox       fullBox       = {m_vLastWindowPos.x, m_vLastWindowPos.y, m_vLastWindowSize.x, height};

    fullBox.translate(offset - pMonitor->vecPosition + WORKSPACEOFFSET).scale(pMonitor->scale);

    fullBox.x -= **PBORDERSIZE * pMonitor->scale;
    fullBox.y -= **PBORDERSIZE * pMonitor->scale;
    fullBox.width += **PBORDERSIZE * 2 * pMonitor->scale;
    fullBox.height += **PBORDERSIZE * 2 * pMonitor->scale;

    for (size_t i = 0; i < **PBORDERS; ++i) {
        const int PREVBORDERSIZESCALED = i == 0 ? 0 : (**PSIZES[i - 1] == -1 ? **PBORDERSIZE : **(PSIZES[i - 1])) * pMonitor->scale;
        const int THISBORDERSIZE       = **(PSIZES[i]) == -1 ? **PBORDERSIZE : (**PSIZES[i]);

        if (i != 0) {
            rounding += rounding == 0 ? 0 : PREVBORDERSIZESCALED / pMonitor->scale;
            fullBox.x -= PREVBORDERSIZESCALED;
            fullBox.y -= PREVBORDERSIZESCALED;
            fullBox.width += PREVBORDERSIZESCALED * 2;
            fullBox.height += PREVBORDERSIZESCALED * 2;
        }

        if (fullBox.width < 1 || fullBox.height < 1)
            break;

        g_pHyprOpenGL->scissor((CBox*)nullptr);

        g_pHyprOpenGL->renderBorder(&fullBox, CColor{(uint64_t) * *PCOLORS[i]}, **PNATURALROUND ? ORIGINALROUND : rounding, THISBORDERSIZE, a,
                                    **PNATURALROUND ? ORIGINALROUND : -1);
    }*/

    m_seExtents = {{0, 0}, {0, height}};

    // TODO: Check if we need to reposition;
    // g_pDecorationPositioner->repositionDeco(this);
}

eDecorationType CHarpoonIndicators::getDecorationType() {
    return DECORATION_CUSTOM;
}

void CHarpoonIndicators::updateWindow(CWindow* pWindow) {
    m_vLastWindowPos  = pWindow->m_vRealPosition.value();
    m_vLastWindowSize = pWindow->m_vRealSize.value();

    damageEntire();
}

void CHarpoonIndicators::damageEntire() {
    static auto* const PHEIGHT = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:harpoon:indicators:height")->getDataStaticPtr();
    CBox               dm      = {m_vLastWindowPos.x, m_vLastWindowPos.y, m_vLastWindowSize.x, m_seExtents.bottomRight.y};
    dm.translate({0, m_vLastWindowSize.y - **PHEIGHT / 1.3});
    g_pHyprRenderer->damageBox(&dm);
}

void CHarpoonIndicators::addIdentifier(const std::string& identifier) {
    // Don't allow duplicates
    if (std::find_if(m_indicators.begin(), m_indicators.end(), [&](const SHarpoonIndicator& i) { return i.identifierText == identifier; }) != m_indicators.end())
        return;

    m_indicators.push_back(SHarpoonIndicator{identifier});
    damageEntire();
}

void CHarpoonIndicators::removeIdentifier(const std::string& identifier) {
    m_indicators.erase(std::remove_if(m_indicators.begin(), m_indicators.end(), [&](const SHarpoonIndicator& i) { return i.identifierText == identifier; }), m_indicators.end());

    damageEntire();
}

void CHarpoonIndicators::toggleVisibility(const bool visible) {
    m_eLayer = visible ? DECORATION_LAYER_OVER : DECORATION_LAYER_UNDER;

    damageEntire();
}

void CHarpoonIndicators::renderText(CTexture& out, const std::string& text, const CColor& color, const Vector2D& bufferSize, const float scale, const int fontSize) {
    const auto CAIROSURFACE = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, bufferSize.x, bufferSize.y);
    const auto CAIRO        = cairo_create(CAIROSURFACE);

    // clear the pixmap
    cairo_save(CAIRO);
    cairo_set_operator(CAIRO, CAIRO_OPERATOR_CLEAR);
    cairo_paint(CAIRO);
    cairo_restore(CAIRO);

    // draw title using Pango
    PangoLayout* layout = pango_cairo_create_layout(CAIRO);
    pango_layout_set_text(layout, text.c_str(), -1);

    PangoFontDescription* fontDesc = pango_font_description_from_string("sans");
    pango_font_description_set_size(fontDesc, fontSize * scale * PANGO_SCALE);
    pango_layout_set_font_description(layout, fontDesc);
    pango_font_description_free(fontDesc);

    const int maxWidth = bufferSize.x;

    pango_layout_set_width(layout, maxWidth * PANGO_SCALE);
    pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_NONE);

    cairo_set_source_rgba(CAIRO, color.r, color.g, color.b, color.a);

    int layoutWidth, layoutHeight;
    pango_layout_get_size(layout, &layoutWidth, &layoutHeight);
    const double xOffset = (bufferSize.x / 2.0 - layoutWidth / PANGO_SCALE / 2.0);
    const double yOffset = (bufferSize.y / 2.0 - layoutHeight / PANGO_SCALE / 2.0);

    cairo_move_to(CAIRO, xOffset, yOffset);
    pango_cairo_show_layout(CAIRO, layout);

    g_object_unref(layout);

    cairo_surface_flush(CAIROSURFACE);

    // copy the data to an OpenGL texture we have
    const auto DATA = cairo_image_surface_get_data(CAIROSURFACE);
    out.allocate();
    glBindTexture(GL_TEXTURE_2D, out.m_iTexID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

#ifndef GLES2
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_BLUE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
#endif

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bufferSize.x, bufferSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, DATA);

    // delete cairo
    cairo_destroy(CAIRO);
    cairo_surface_destroy(CAIROSURFACE);
}
