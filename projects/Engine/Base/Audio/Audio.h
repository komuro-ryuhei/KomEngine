#pragma once

#include <xaudio2.h>

#pragma comment(lib, "xaudio2.lib")

// C++
#include <fstream>

// MyClass
#include "Engine/lib/ComPtr/ComPtr.h"

// チャンクヘッダ
struct ChunkHeader {
	char id[4];   // チャンク毎のID
	int32_t size; // チャンクサイズ
};

// RIFFヘッダチャンク
struct RiffHeader {
	ChunkHeader chunk; // RIFF
	char type[4];      // WAVE
};

// FMTチャンク
struct FormatChunk {
	ChunkHeader chunk; // fmt
	WAVEFORMATEX fmt;  // 波形フォーマット
};

// 音声データ
struct SoundData {
	// 波形フォーマット
	WAVEFORMATEX wfex;
	// バッファの先頭アドレス
	BYTE* pBuffer;
	// バッファのサイズ
	unsigned int bufferSize;
};

class Audio {

public:
	void Init();

	// 音声データ読み込み
	SoundData SoundLoadWave(const char* filename);
	// 音声データ開放
	void SoundUnload(SoundData* soundData);
	// 音声再生
	void SoundPlayWave(IXAudio2* xAudio2, const SoundData& soundData);


	IXAudio2* GetXAudio2() const;

private:
	ComPtr<IXAudio2> xAudio2_;
	IXAudio2MasteringVoice* masterVoice_;
};