#include "audio-module.h"

#include <algorithm>

bool AudioModule::Tick(const float& InDeltaTime)
{
	BASS_Update(_StreamHandle);
	_CurrentTime = GetTimeSeconds();

    if (_PlayEndTime >= 0.0 && _CurrentTime >= _PlayEndTime && !_Paused)
    {
        SetPause(true);
        _PlayEndTime = -1.0;
    }

	return true;
}

void AudioModule::LoadAudio(const std::filesystem::path& InPath)
{
	BASS_Free();
	BASS_Init(_Device, _Freq, 0, 0, NULL);

	_CurrentAudioPath = InPath;

	_StreamHandle = BASS_FX_TempoCreate(BASS_StreamCreateFile(FALSE, InPath.string().c_str(), 0, 0, BASS_STREAM_DECODE | BASS_STREAM_PRESCAN), BASS_FX_FREESOURCE);

	auto error = BASS_ErrorGetCode();
	if (error != 0)
	{
		std::cout << BASS_ErrorGetCode() << std::endl;
	}

	BASS_ChannelPlay(_StreamHandle, FALSE);
	BASS_ChannelPause(_StreamHandle);

	BASS_ChannelSetAttribute(_StreamHandle, BASS_ATTRIB_TEMPO_OPTION_SEQUENCE_MS, 32);
	BASS_ChannelSetAttribute(_StreamHandle, BASS_ATTRIB_TEMPO_OPTION_SEEKWINDOW_MS, 4);

	ResetSpeed();

	_Paused = true;
}

void AudioModule::TogglePause()
{
	_Paused = !_Paused;

	SetPause(_Paused);
}

void AudioModule::SetPause(bool InPause)
{
	_Paused = InPause;

    if (_Paused)
    {
        BASS_ChannelPause(_StreamHandle);
        _PlayEndTime = -1.0; // Clear play range if manually paused
    }
	else
		BASS_ChannelPlay(_StreamHandle, FALSE);

	_CurrentTime = GetTimeSeconds();
}

void AudioModule::PlayRange(Time Start, Time End)
{
    SetTimeMilliSeconds(Start);
    _PlayEndTime = double(End) / 1000.0;
    SetPause(false);
}

void AudioModule::ResetSpeed()
{
	_Speed = 1.f;

	BASS_CHANNELINFO info;
	BASS_ChannelGetInfo(_StreamHandle, &info);

	if (UsePitch)
	{
		BASS_ChannelSetAttribute(_StreamHandle, BASS_ATTRIB_FREQ, float(info.freq) * _Speed);
		BASS_ChannelSetAttribute(_StreamHandle, BASS_ATTRIB_TEMPO, 0);
	}
	else
	{
		BASS_ChannelSetAttribute(_StreamHandle, BASS_ATTRIB_TEMPO, (_Speed - 1.f) * 100.f);
	}
}

void AudioModule::SetTimeMilliSeconds(const Time InTime)
{
	_CurrentTime = double(InTime) / 1000.0;

	BASS_ChannelSetPosition(_StreamHandle, BASS_ChannelSeconds2Bytes(_StreamHandle, _CurrentTime), BASS_POS_BYTE);
}

void AudioModule::MoveDelta(const int InDeltaMilliSeconds)
{
	_CurrentTime += double(InDeltaMilliSeconds) / 1000.0;

	BASS_ChannelSetPosition(_StreamHandle, BASS_ChannelSeconds2Bytes(_StreamHandle, _CurrentTime), BASS_POS_BYTE);
}

void AudioModule::ChangeSpeed(const float InDeltaSpeed)
{
	_Speed += InDeltaSpeed;

	if (_Speed < 0.05f)
		_Speed = 0.05f;

	if (_Speed > 2.f)
		_Speed = 2.f;

	BASS_CHANNELINFO info;
	BASS_ChannelGetInfo(_StreamHandle, &info);

	if (UsePitch)
	{
		BASS_ChannelSetAttribute(_StreamHandle, BASS_ATTRIB_FREQ, float(info.freq) * _Speed);
		BASS_ChannelSetAttribute(_StreamHandle, BASS_ATTRIB_TEMPO, 0);
	}
	else
	{
		BASS_ChannelSetAttribute(_StreamHandle, BASS_ATTRIB_TEMPO, (_Speed - 1.f) * 100.f);
	}
}

double AudioModule::GetTimeSeconds()
{
	return BASS_ChannelBytes2Seconds(_StreamHandle, BASS_ChannelGetPosition(_StreamHandle, BASS_POS_BYTE));
}

Time AudioModule::GetTimeMilliSeconds()
{
	return _CurrentTime * 1000;
}

Time AudioModule::GetSongLengthMilliSeconds()
{
	return BASS_ChannelBytes2Seconds(_StreamHandle, BASS_ChannelGetLength(_StreamHandle, BASS_POS_BYTE)) * 1000;
}

float AudioModule::GetPlaybackSpeed()
{
	return _Speed;
}

float AudioModule::EstimateBPM(Time Start, Time End)
{
    if (_CurrentAudioPath.empty())
        return 0.0f;

    HSTREAM decodeStream = BASS_StreamCreateFile(FALSE, _CurrentAudioPath.string().c_str(), 0, 0, BASS_STREAM_DECODE);
    if (!decodeStream)
        return 0.0f;

    double startSec = double(Start) / 1000.0;
    double endSec = double(End) / 1000.0;

    // Use default min/max hints (e.g. 45 to 230)
    // MAKELONG(low, high)
    DWORD minMaxBpm = MAKELONG(45, 230);

    float bpm = BASS_FX_BPM_DecodeGet(decodeStream, startSec, endSec, minMaxBpm, BASS_FX_BPM_BKGRND | BASS_FX_BPM_MULT2, NULL, NULL);

    BASS_StreamFree(decodeStream);

    return bpm;
}

Time AudioModule::EstimateOffset(double BPM, Time Start, Time End)
{
    if (!_ReadableWaveFormData || BPM <= 0.0)
        return Start;

    double beatInterval = 60000.0 / BPM;

    // We only need to check offsets within one beat interval [0, beatInterval)
    // Actually, we want to find the offset *relative to Start* that aligns with peaks.
    // We search for phase phi in [0, beatInterval).

    // Algorithm:
    // Iterate phi from 0 to beatInterval with step (e.g. 10ms or 5ms).
    // For each phi, sum the energy at (Start + phi + k * beatInterval) for all k.
    // Pick phi with max energy.

    int step = 5; // 5ms step for precision
    int maxPhi = int(beatInterval);

    float bestEnergy = -1.0f;
    int bestPhi = 0;

    Time songLen = GetSongLengthMilliSeconds();
    Time searchEnd = std::min(End, songLen);

    for (int phi = 0; phi < maxPhi; phi += step)
    {
        float energy = 0.0f;
        int count = 0;

        for (double t = Start + phi; t < searchEnd; t += beatInterval)
        {
            Time timeIdx = Time(t);
            if (timeIdx < songLen)
            {
                // Simple energy: amplitude
                const WaveFormData& data = _ReadableWaveFormData[timeIdx];
                energy += (std::abs(data.Left) + std::abs(data.Right));
                count++;
            }
        }

        if (count > 0)
        {
            // We might want average energy to avoid bias if count differs (though it shouldn't differ much)
            // But sum is fine if range is large.
            if (energy > bestEnergy)
            {
                bestEnergy = energy;
                bestPhi = phi;
            }
        }
    }

    return Start + bestPhi;
}

Time AudioModule::FindNearestPeak(Time Center, int WindowMs)
{
    if (!_ReadableWaveFormData || WindowMs <= 0)
        return Center;

    Time songLen = GetSongLengthMilliSeconds();
    Time start = std::max(0, Center - WindowMs);
    Time end = std::min(songLen, Center + WindowMs);

    float maxAmp = -1.0f;
    Time peakTime = Center;

    for (Time t = start; t <= end; ++t)
    {
        // SampleWaveFormData handles index checks, but direct access is faster if we are sure
        // Let's use direct access for loop
        const WaveFormData& data = _ReadableWaveFormData[t];
        float amp = std::abs(data.Left) + std::abs(data.Right);

        if (amp > maxAmp)
        {
            maxAmp = amp;
            peakTime = t;
        }
    }

    return peakTime;
}

WaveFormData* AudioModule::GenerateAndGetWaveformData(const std::filesystem::path& InPath)
{
	HSTREAM decoder = BASS_StreamCreateFile(FALSE, InPath.string().c_str(), 0, 0, BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE);

	if (_WaveFormData != nullptr)
	{
		delete[] _WaveFormData;
		_WaveFormData = nullptr;
	}

	if (_ReadableWaveFormData != nullptr)
	{
		delete[] _ReadableWaveFormData;
		_ReadableWaveFormData = nullptr;
	}

	_SongByteLength = BASS_ChannelGetLength(decoder, BASS_POS_BYTE);
	_WaveFormData = (float*)std::malloc(_SongByteLength);
	_SongByteLength = BASS_ChannelGetData(decoder, _WaveFormData, _SongByteLength);

	_ReadableWaveFormData = new WaveFormData[GetSongLengthMilliSeconds()]();

	for(Time time = 0; time < GetSongLengthMilliSeconds(); ++time)
	{
		double timeSeconds = double(time) / 1000.0;
		QWORD index = BASS_ChannelSeconds2Bytes(_StreamHandle, std::max(0.0, timeSeconds)) / 2;

		if(index < _SongByteLength / sizeof(int))
		{
			float right = _WaveFormData[index + 1];
			float left = _WaveFormData[index];

			_ReadableWaveFormData[time].Right = right;
			_ReadableWaveFormData[time].Left = left;
		}
	}

	return _ReadableWaveFormData;
}

const WaveFormData& AudioModule::SampleWaveFormData(const Time InTimePoint)
{
	return _ReadableWaveFormData[std::max(0, std::min(GetSongLengthMilliSeconds(), InTimePoint))];
}
