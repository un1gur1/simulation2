#include "SoundTable.h"

// サウンドID から サウンドパスを取得

// システム用
namespace SoundTable_System 
{
	static const std::unordered_map<SoundID, std::string> Table = 
	{
		{ SoundID::BGM_TITLE, "Data/Sound/BGM/system.mp3" },
		{ SoundID::SE_BUTTON, "Data/Sound/SE/system.mp3" },
	};
}

// タイトル画面用
namespace SoundTable_Title
{
	static const std::unordered_map<SoundID, std::string> Table =
	{
		{ SoundID::BGM_TITLE, "Data/Sound/BGM/maou_bgm_neorock83.mp3" },
		{ SoundID::SE_BUTTON, "Data/Sound/SE/maou_se_sound24.mp3" },
	};
}

// ゲーム画面用
namespace SoundTable_Game 
{
	static const std::unordered_map<SoundID, std::string> Table = 
	{
		{ SoundID::BGM_BATTLE, "Data/Sound/BGM/game.wav" },
		{ SoundID::SE_RUN,	"Data/Sound/SE/hit.wav" },
		{ SoundID::SE_ATTACK, "Data/Sound/SE/attack.wav" },
	};
}
