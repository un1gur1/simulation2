#pragma once
#include <string>
#include <unordered_map>

// サウンドID
enum class SoundID
{
	BGM_TITLE,
	BGM_BATTLE,
	SE_BUTTON,
	SE_RUN,
	SE_ATTACK,
};

// 読み込むシーン
enum class LoadScene
{
	SYSTEM,		// システム
	TITLE,		// タイトル
	GAME,		// ゲーム
	CLEAR,		// クリア
	GAMEOVER,	// ゲームオーバー
};

// サウンド属性
struct SoundData
{
	std::string path;	// サウンドのパス
};

// サウンドテーブル（名称被りや拡張性の為にクラス化)
class SoundTable
{
public:
	// サウンドテーブルのマップ
	// サウンドID から サウンドパスを取得
	static const std::unordered_map<SoundID, SoundData> Table;
};
