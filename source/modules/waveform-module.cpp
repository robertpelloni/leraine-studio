#include "waveform-module.h"

#include "imgui.h"

bool WaveFormModule::StartUp()
{
    _ScalableWaveFormTexture.create(_WaveFormWidth, 8192);
    _WaveFormLines.setPrimitiveType(sf::Lines);
    _WaveFormLines.resize(8192 * 2 * 2);

    return true;
}

void WaveFormModule::SetWaveFormData(WaveFormData* const InWaveFormData, const Time InSongLengthMilliSeconds)
{
    _WaveFormData = InWaveFormData;
    _SongLengthMilliSeconds = InSongLengthMilliSeconds;
}

void WaveFormModule::RenderWaveForm(TimefieldRenderGraph& InOutRenderGraph, const Time InTimeBegin, const Time InTimeEnd, const int InScreenX, const float InZoomLevel, const float InWindowHeight)
{
    //TODO: Represent lowpass and highpass filters through cool shaders

    if(!_WaveFormData)
        return;

    const Time timeHeight = InTimeEnd - InTimeBegin;

    if(timeHeight <= 0)
        return;

    const Time timeStartPoint = (InTimeEnd / timeHeight) * timeHeight;
    const Time timeOffsetPosition = InTimeEnd - timeStartPoint;

    _ScalableWaveFormTexture.clear({0, 0, 0, 0});

    const float windowHeight = InWindowHeight;
    const float scale = windowHeight / float(InTimeEnd - InTimeBegin);

    const int maxLines = 8192;
    const int lineAmount = std::min(maxLines, int(timeHeight * 2 * _ResolutionMultiplier));

    for(int i = 0; i < lineAmount; ++i)
    {
        const int timeOffset = int(float(i) / (2.0f * _ResolutionMultiplier));
        const size_t index = std::min(_SongLengthMilliSeconds, std::max(0, timeStartPoint + timeHeight - timeOffset));
        const WaveFormData waveFormData = _WaveFormData[index];

        const float y = float(_ScalableWaveFormTexture.getSize().y - i);
        const int pointIndex = i * 2;

        float leftAmp = abs(waveFormData.Left * _WaveFormWidth / 2);
        float rightAmp = abs(waveFormData.Right * _WaveFormWidth / 2);

        if (_StereoMode)
        {
            if (_Inverted)
            {
                _WaveFormLines[pointIndex].position.x = _WaveFormWidth / 2 - rightAmp;
                _WaveFormLines[pointIndex].position.y = y;

                _WaveFormLines[pointIndex + 1].position.x = _WaveFormWidth / 2 + leftAmp;
                _WaveFormLines[pointIndex + 1].position.y = y;
            }
            else
            {
                _WaveFormLines[pointIndex].position.x = _WaveFormWidth / 2 - leftAmp;
                _WaveFormLines[pointIndex].position.y = y;

                _WaveFormLines[pointIndex + 1].position.x = _WaveFormWidth / 2 + rightAmp;
                _WaveFormLines[pointIndex + 1].position.y = y;
            }
        }
        else
        {
            float monoAmp = (abs(waveFormData.Left) + abs(waveFormData.Right)) * 0.5f;
            _WaveFormLines[pointIndex].position.x = _WaveFormWidth / 2 - monoAmp * (_WaveFormWidth / 2);
            _WaveFormLines[pointIndex].position.y = y;

            _WaveFormLines[pointIndex + 1].position.x = _WaveFormWidth / 2 + monoAmp * (_WaveFormWidth / 2);
            _WaveFormLines[pointIndex + 1].position.y = y;
        }

        _WaveFormLines[pointIndex].color = sf::Color(255, 255, 255, 255);
        _WaveFormLines[pointIndex + 1].color = sf::Color(255, 255, 255, 255);
    }

    _ScalableWaveFormTexture.draw(_WaveFormLines);
    _ScalableWaveFormSprite.setTexture(_ScalableWaveFormTexture.getTexture());

    InOutRenderGraph.SubmitTimefieldRenderCommand(0, InTimeEnd + timeHeight - timeOffsetPosition, [this, scale, InTimeEnd, InWindowHeight](sf::RenderTarget* const InRenderTarget, const TimefieldMetrics& InTimefieldMetrics, const int InScreenX, const int InScreenY)
    {
        float positionY = float(InScreenY);

        _ScalableWaveFormSprite.setPosition(InTimefieldMetrics.LeftSidePosition + InTimefieldMetrics.FieldWidthHalf - _WaveFormWidth / 2, positionY);
        _ScalableWaveFormSprite.setScale(1.0f, scale);
        _ScalableWaveFormSprite.setColor(_BackColor);

        InRenderTarget->draw(_ScalableWaveFormSprite);

        _ScalableWaveFormSprite.setColor(_FrontColor);
        _ScalableWaveFormSprite.setScale(0.375, scale);
        _ScalableWaveFormSprite.setPosition(InTimefieldMetrics.LeftSidePosition + InTimefieldMetrics.FieldWidthHalf - _WaveFormWidth / 8 - _WaveFormWidth / 16, positionY);

        InRenderTarget->draw(_ScalableWaveFormSprite);
    });
}
