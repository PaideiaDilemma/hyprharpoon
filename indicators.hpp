#pragma once

#define WLR_USE_UNSTABLE

#include <hyprland/src/render/decorations/IHyprWindowDecoration.hpp>
#include <hyprland/src/render/OpenGL.hpp>

struct SHarpoonIndicator {
    std::string identifierText = "";
    CColor      col            = CColor(0, 0, 0, 0);
    CTexture    texture;
};

class CHarpoonIndicators : public IHyprWindowDecoration {
  public:
    CHarpoonIndicators(CWindow*, const std::vector<std::string>& identifiers);
    virtual ~CHarpoonIndicators();

    virtual SDecorationPositioningInfo getPositioningInfo();

    virtual void                       onPositioningReply(const SDecorationPositioningReply& reply);

    virtual void                       draw(CMonitor*, float a);

    virtual eDecorationType            getDecorationType();

    virtual void                       updateWindow(CWindow*);

    virtual void                       damageEntire();

    virtual uint64_t                   getDecorationFlags();

    virtual eDecorationLayer           getDecorationLayer();

    virtual std::string                getDisplayName();

    void                               addIdentifier(const std::string& identifier);
    void                               removeIdentifier(const std::string& identifier);
    void                               toggleVisibility(const bool visible);

  private:
    void                           renderText(CTexture& out, const std::string& text, const CColor& color, const Vector2D& bufferSize, const float scale, const int fontSize);
    void                           renderIndicatorText(CBox* identifierBox, const float scale, const float a);

    SWindowDecorationExtents       m_seExtents;

    CWindow*                       m_pWindow = nullptr;

    std::vector<SHarpoonIndicator> m_indicators;

    eDecorationLayer               m_eLayer;

    Vector2D                       m_vLastWindowPos;
    Vector2D                       m_vLastWindowSize;
};
