#pragma once

#include "SoundTable.h"

class AudioManager
{
public:
	// シングルトン（生成・取得・削除）
	static void CreateInstance(void) { if (instance_ == nullptr) { instance_ = new AudioManager(); } }
	static AudioManager* GetInstance(void) { return instance_; }
	static void DeleteInstance(void) { if (instance_ != nullptr) { delete instance_; instance_ = nullptr; } }

private:
	// 静的インスタンス
	static AudioManager* instance_;

	AudioManager();		// コンストラクタ
	~AudioManager();	// デストラクタ

	// コピー・ムーブ操作を禁止
	AudioManager(const AudioManager&) = delete;
	AudioManager& operator=(const AudioManager&) = delete;
	AudioManager(AudioManager&&) = delete;
	AudioManager& operator=(AudioManager&&) = delete;

public:
	// 初期化
	void Init(void);

	// サウンド制御
	// 指定したシーンのサウンドテーブルを読み込む
	void LoadSceneSound(LoadScene scene);
	
	// 指定したシーンのサウンドテーブルを削除する
	void DeleteSceneSound(LoadScene scene);

	void PlayBGM(SoundID id);		// BGMを再生
	void StopBGM(void);				// BGMを止める
	void PlaySE(SoundID id);		// SEを再生
	void DeleteAll(void);			// 全てのサウンド削除

	// 音量設定
	void SetBgmVolume(int volume);		// 0～255
	void SetSeVolume(int volume);		// 0～255
	void SetMasterVolume(int volume);	// 0～255

	int GetBgmVolume(int volume) const { return bgmVolume_; }
	int GetSeVolume(int volume) const { return seVolume_; }
	int GetMasterVolume(int volume) const { return masterVolume_; }

private:
	std::unordered_map<SoundID, int> handles_;	// ハンドル管理用

	// 現在再生されているBGM
	SoundID currentBgm_;

	int bgmVolume_;		// bgm音量
	int seVolume_;		// se音量
	int masterVolume_;	// master音量
};
