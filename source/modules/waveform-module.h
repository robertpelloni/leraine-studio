#pragma once

#include "base/module.h"

#include <SFML/Graphics.hpp>
#include <map>

class WaveFormModule : public Module
{
public:

    virtual bool StartUp() override;

public:

    void SetWaveFormData(WaveFormData* const InWaveFormData, const Time InSongLengthMilliSeconds);
    void RenderWaveForm(TimefieldRenderGraph& InOutRenderGraph, const Time InTimeBegin, const Time InTimeEnd, const int InScreenX, const float InZoomLevel, const float InWindowHeight);

    bool HasWaveFormData() const { return _WaveFormData != nullptr; }
    Time GetSongLength() const { return _SongLengthMilliSeconds; }

    bool IsStereoMode() const { return _StereoMode; }
    void SetStereoMode(bool InStereo) { _StereoMode = InStereo; }

    sf::Color GetFrontColor() const { return _FrontColor; }
    void SetFrontColor(const sf::Color& InColor) { _FrontColor = InColor; }

    sf::Color GetBackColor() const { return _BackColor; }
    void SetBackColor(const sf::Color& InColor) { _BackColor = InColor; }

    uint8_t GetAlpha() const { return _FrontColor.a; }
    void SetAlpha(uint8_t InAlpha)
    {
        _FrontColor.a = InAlpha;
        _BackColor.a = std::min(255, int(InAlpha * 0.75f));
    }

private:

    const int _WaveFormWidth = 256;
    sf::VertexArray _WaveFormLines;

    sf::RenderTexture _ScalableWaveFormTexture;
    sf::Sprite _ScalableWaveFormSprite;

    WaveFormData* _WaveFormData = nullptr;

    Time _SongLengthMilliSeconds = 0;
    bool _StereoMode = true;

    sf::Color _FrontColor = sf::Color(0, 255, 255, 128);
    sf::Color _BackColor  = sf::Color(255, 255, 0, 96);
};
