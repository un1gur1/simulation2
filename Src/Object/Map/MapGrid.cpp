#define NOMINMAX
#include "MapGrid.h"
#include <DxLib.h>
#include <cmath>
#include <string>

namespace App {

    // ==========================================
    // コンストラクタ: マップグリッドの基本設定
    // ==========================================
    MapGrid::MapGrid(int tileSize, Vector2 offset)
        : m_tileSize(tileSize)           // マス目1個のサイズ（ピクセル）
        , m_offset(offset)               // マップ全体の左上座標
        , m_ruleMode(BattleRuleMode::CLASSIC)
        , m_totalTurns(0)                // 経過ターン数
        , m_currentCycleTick(0)          // アイテム再出現カウンター
        , m_spawnInterval(3)             // アイテム再出現間隔（ターン数）
    {
    }

    // ==========================================
    // 出現地点の初期化: ルールモードに応じた配置
    // ==========================================
    void MapGrid::InitializeSpawnPoints() {
        m_spawnPoints.clear();

        if (m_ruleMode == BattleRuleMode::CLASSIC) {
            InitializeClassicSpawns();   // クラシックモード用配置
        }
        else {
            InitializeZeroOneSpawns();   // ゼロワンモード用配置
        }
    }

    // ==========================================
    // ルールモード＆ステージ設定
    // 引数: mode - ゲームルール / stageIndex - ステージ番号（0~2）
    // ==========================================
    void MapGrid::SetRuleModeAndStage(BattleRuleMode mode, int stageIndex) {
        m_ruleMode = mode;
        m_stageIndex = stageIndex;

        // アイテム再出現間隔の設定（モード別）
        if (m_ruleMode == BattleRuleMode::CLASSIC) {
            m_spawnInterval = 3;  // クラシック: 3ターンごと
        }
        else {
            m_spawnInterval = 2;  // ゼロワン: 2ターンごと（テンポ重視）
        }

        // ステージレイアウト初期化
        InitializeSpawnPoints();
    }

    // ==========================================
    // クラシックモードのアイテム配置
    // 特徴: ステージ3のみアイテムが時間で循環する
    // ==========================================
    void MapGrid::InitializeClassicSpawns() {
        m_spawnPoints.clear();
        struct PointDef { int x, y; char op; };
        std::vector<PointDef> defs;

        if (m_stageIndex == 0) {
            // ==========================================
            // 【STAGE 1】標準配置: バランス重視
            // マイナスが多めで攻撃的な展開が有利
            // ==========================================
            defs = {
                { 4, 1, '*' }, { 4, 7, '+' }, { 1, 4, '-' }, { 7, 4, '-' },
                { 2, 2, '-' }, { 6, 2, '/' }, { 2, 6, '/' }, { 6, 6, '-' },
                { 4, 4, '+' }, { 3, 3, '-' }, { 5, 5, '-' }
            };
        }
        else if (m_stageIndex == 1) {
            // ==========================================
            // 【STAGE 2】中央密集: マイナス祭り
            // 中央での激しい攻防が展開される
            // ==========================================
            defs = {
                // 外周8方向
                { 4, 0, '-' }, { 7, 1, '-' }, { 8, 4, '-' }, { 7, 7, '-' },
                { 4, 8, '-' }, { 1, 7, '-' }, { 0, 4, '-' }, { 1, 1, '-' },

                // 中間層8方向
                { 4, 2, '-' }, { 6, 2, '-' }, { 6, 4, '-' }, { 6, 6, '-' },
                { 4, 6, '-' }, { 2, 6, '-' }, { 2, 4, '-' }, { 2, 2, '-' },

                // 中央
                { 4, 4, '-' }
            };
        }
        else {
            // ==========================================
            // 【STAGE 3】円形配置: 時間で循環する特殊ステージ
            // アイテムが (+→-→*→/) の順で自動変化
            // 取るタイミングが勝敗を分ける
            // ==========================================
            defs = {
                // 外周8方向（プラススタート）
                { 4, 0, '+' }, { 7, 1, '+' }, { 8, 4, '+' }, { 7, 7, '+' },
                { 4, 8, '+' }, { 1, 7, '+' }, { 0, 4, '+' }, { 1, 1, '+' },

                // 中間層8方向（マイナススタート）
                { 4, 2, '-' }, { 6, 2, '-' }, { 6, 4, '-' }, { 6, 6, '-' },
                { 4, 6, '-' }, { 2, 6, '-' }, { 2, 4, '-' }, { 2, 2, '-' },

                // 中央（掛け算スタート）
                { 4, 4, '*' }
            };
        }

        // ステージ3専用: 循環シーケンス
        std::vector<char> cycleSeq = { '+', '-', '*', '/' };

        for (const auto& d : defs) {
            SpawnPoint sp;
            sp.pos = { d.x, d.y };

            if (m_stageIndex == 2) {
                // ステージ3: 循環モード
                sp.sequence = cycleSeq;

                // 初期記号から開始位置を決定
                auto it = std::find(cycleSeq.begin(), cycleSeq.end(), d.op);
                if (it != cycleSeq.end()) {
                    sp.currentIndex = (int)std::distance(cycleSeq.begin(), it);
                }
                else {
                    sp.currentIndex = 0;
                }
            }
            else {
                // ステージ1,2: 固定モード（同じ記号がずっと出る）
                sp.sequence = { d.op };
                sp.currentIndex = 0;
            }

            sp.currentSymbol = sp.sequence[sp.currentIndex];
            sp.isAvailable = true;  // 初期状態: 取得可能
            m_spawnPoints.push_back(sp);
        }
    }

    // ==========================================
    // ゼロワンモードのアイテム配置
    // 特徴: 全ステージで (+→-→*→/→+→-) の6段階循環
    // ==========================================
    void MapGrid::InitializeZeroOneSpawns() {
        m_spawnPoints.clear();
        std::vector<char> zeroOneSeq = { '+', '-', '*', '/', '+', '-' };

        struct PointDef { int x, y, startIdx; };
        std::vector<PointDef> defs;

        if (m_stageIndex == 0) {
            // ==========================================
            // 【STAGE 1】標準配置: 各地点で異なる開始位置
            // 17個のアイテムでバランスよく分散
            // ==========================================
            defs = {
                { 4, 2, 0 }, { 4, 6, 1 }, { 2, 4, 1 }, { 6, 4, 0 },
                { 3, 3, 0 }, { 5, 3, 1 }, { 3, 5, 2 }, { 5, 5, 3 },
                { 2, 2, 4 }, { 6, 2, 5 }, { 2, 6, 3 }, { 6, 6, 2 },
                { 4, 4, 2 }, { 4, 1, 0 }, { 4, 7, 4 }, { 1, 4, 1 }, { 7, 4, 5 }
            };
        }
        else if (m_stageIndex == 1) {
            // ==========================================
            // 【STAGE 2】密集配置: 全てマイナススタート
            // 中央での取り合いが激化
            // ==========================================
            defs = {
                // 外周8方向（全て index=1 でマイナススタート）
                { 4, 0, 1 }, { 7, 1, 1 }, { 8, 4, 1 }, { 7, 7, 1 },
                { 4, 8, 1 }, { 1, 7, 1 }, { 0, 4, 1 }, { 1, 1, 1 },

                // 中間層8方向（同じくマイナススタート）
                { 4, 2, 1 }, { 6, 2, 1 }, { 6, 4, 1 }, { 6, 6, 1 },
                { 4, 6, 1 }, { 2, 6, 1 }, { 2, 4, 1 }, { 2, 2, 1 },

                // 中央（マイナススタート）
                { 4, 4, 1 }
            };
        }
        else {
            // ==========================================
            // 【STAGE 3】四隅散開: 遠距離移動が必須
            // 移動コスト管理が重要になる高難度ステージ
            // ==========================================
            defs = {
                { 1, 1, 0 }, { 7, 1, 1 }, { 1, 7, 1 }, { 7, 7, 0 },
                { 4, 1, 2 }, { 4, 7, 3 }, { 1, 4, 4 }, { 7, 4, 5 },
                { 2, 2, 0 }, { 6, 2, 1 }, { 2, 6, 1 }, { 6, 6, 0 },
                { 4, 4, 3 }  // 中央: 割り算スタート
            };
        }

        for (const auto& d : defs) {
            SpawnPoint sp;
            sp.pos = { d.x, d.y };
            sp.sequence = zeroOneSeq;
            sp.currentIndex = d.startIdx % zeroOneSeq.size();
            sp.currentSymbol = sp.sequence[sp.currentIndex];
            sp.isAvailable = true;
            m_spawnPoints.push_back(sp);
        }
    }

    // ==========================================
    // アイテム取得処理
    // 戻り値: 取得した記号（取得不可なら '\0'）
    // ==========================================
    char MapGrid::PickUpItem(int x, int y) {
        for (auto& sp : m_spawnPoints) {
            if (sp.pos.x == x && sp.pos.y == y && sp.isAvailable) {
                char picked = sp.currentSymbol;
                sp.isAvailable = false;  // 取得済み状態にする

                // クラシックのステージ3「以外」は取得時に次へ進める
                // ステージ3は時間経過でのみ進むため、ここでは進めない
                if (!(m_ruleMode == BattleRuleMode::CLASSIC && m_stageIndex == 2)) {
                    sp.currentIndex = (sp.currentIndex + 1) % sp.sequence.size();
                    sp.currentSymbol = sp.sequence[sp.currentIndex];
                }

                return picked;
            }
        }
        return '\0';  // 取得失敗
    }

    // ==========================================
    // ターン更新: アイテムの再出現と循環
    // ==========================================
    void MapGrid::UpdateTurn() {
        m_totalTurns++;
        m_currentCycleTick++;

        // 再出現タイミング判定（3または2ターンごと）
        if (m_currentCycleTick >= m_spawnInterval) {
            m_currentCycleTick = 0;

            for (auto& sp : m_spawnPoints) {
                // 取得済みのアイテムを再出現
                if (!sp.isAvailable) {
                    sp.isAvailable = true;
                }

                // クラシックのステージ3のみ特殊処理:
                // 取られていなくても定期的に記号が変化する
                if (m_ruleMode == BattleRuleMode::CLASSIC && m_stageIndex == 2) {
                    sp.currentIndex = (sp.currentIndex + 1) % sp.sequence.size();
                    sp.currentSymbol = sp.sequence[sp.currentIndex];
                }
            }
        }
    }

    // ==========================================
    // 座標変換: 画面座標→グリッド座標
    // 用途: マウスカーソル位置からマス目を特定
    // ==========================================
    IntVector2 MapGrid::ScreenToGrid(const Vector2& pos) const {
        return {
            static_cast<int>((pos.x - m_offset.x) / m_tileSize),
            static_cast<int>((pos.y - m_offset.y) / m_tileSize)
        };
    }

    // ==========================================
    // マス目の中心座標取得
    // 用途: ユニット・アイテムの描画位置計算
    // ==========================================
    Vector2 MapGrid::GetCellCenter(int x, int y) const {
        return {
            m_offset.x + x * m_tileSize + m_tileSize / 2.0f,
            m_offset.y + y * m_tileSize + m_tileSize / 2.0f
        };
    }

    // ==========================================
    // 範囲内判定: 指定座標が9x9グリッド内か
    // ==========================================
    bool MapGrid::IsWithinBounds(int x, int y) const {
        return x >= 0 && x < 9 && y >= 0 && y < 9;
    }

    // ==========================================
    // アイテム検索: 指定座標のアイテム記号を取得
    // 戻り値: アイテム記号（存在しない or 取得済みなら '\0'）
    // ==========================================
    char MapGrid::GetItemAt(int x, int y) const {
        for (const auto& sp : m_spawnPoints) {
            if (sp.pos.x == x && sp.pos.y == y && sp.isAvailable) {
                return sp.currentSymbol;
            }
        }
        return '\0';
    }

    // ==========================================
    // マップ描画: グリッド＆アイテムの表示
    // ==========================================
    void MapGrid::Draw() const {
        // ==========================================
        // 1. グリッド描画: 9x9のマス目
        // ==========================================
        unsigned int lineCol = GetColor(40, 45, 60);   // 枠線の色（暗い青）
        unsigned int fillCol = GetColor(15, 18, 25);   // マス目の塗りつぶし色

        for (int y = 0; y < 9; y++) {
            for (int x = 0; x < 9; x++) {
                Vector2 pos = GetCellCenter(x, y);
                // 枠線
                DrawBox((int)pos.x - 39, (int)pos.y - 39, (int)pos.x + 39, (int)pos.y + 39, lineCol, FALSE);
                // 塗りつぶし
                DrawBox((int)pos.x - 38, (int)pos.y - 38, (int)pos.x + 38, (int)pos.y + 38, fillCol, TRUE);
            }
        }

        // ==========================================
        // 2. アイテム描画: 宝石風エフェクト
        // ==========================================
        double time = GetNowCount() / 1000.0;
        float pulse = (float)(sin(time * 5.0) * 4.0);  // 脈動アニメーション

        for (const auto& sp : m_spawnPoints) {
            Vector2 basePos = GetCellCenter(sp.pos.x, sp.pos.y);
            float drawX = basePos.x;
            float drawY = basePos.y;

            // --- 取得済みアイテムの表示（ホログラム） ---
            if (!sp.isAvailable) {
                SetDrawBlendMode(DX_BLENDMODE_ADD, 120);
                DrawCircleAA(drawX, drawY, 22.0f + pulse / 2.0f, 64, GetColor(80, 100, 120), FALSE, 2.0f);

                // 次に出現する記号を予告表示
                char nextOp = sp.currentSymbol;
                if (m_ruleMode == BattleRuleMode::CLASSIC && m_stageIndex == 2) {
                    // ステージ3は次の記号を表示
                    nextOp = sp.sequence[(sp.currentIndex + 1) % sp.sequence.size()];
                }

                SetFontSize(24);
                char waitStr[2] = { nextOp, '\0' };
                int tw = GetDrawStringWidth(waitStr, 1);
                DrawString((int)drawX - tw / 2, (int)drawY - 12, waitStr, GetColor(100, 140, 180));
                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                continue;
            }

            // --- アイテムの色設定（宝石風） ---
            unsigned int itemCol, addCol, darkCol;
            if (sp.currentSymbol == '+') {
                // ルビー（赤）
                itemCol = GetColor(200, 30, 50);
                addCol = GetColor(255, 100, 100);
                darkCol = GetColor(80, 10, 20);
            }
            else if (sp.currentSymbol == '-') {
                // サファイア（青）
                itemCol = GetColor(30, 120, 220);
                addCol = GetColor(100, 180, 255);
                darkCol = GetColor(10, 30, 80);
            }
            else if (sp.currentSymbol == '*') {
                // エメラルド（緑）
                itemCol = GetColor(40, 180, 60);
                addCol = GetColor(120, 255, 150);
                darkCol = GetColor(15, 60, 20);
            }
            else {
                // アメジスト（紫）
                itemCol = GetColor(180, 40, 200);
                addCol = GetColor(220, 100, 255);
                darkCol = GetColor(60, 10, 80);
            }

            // --- 多層エフェクト描画 ---

            // ① オーラ（盤面に漏れるエネルギー）
            for (int i = 0; i < 2; ++i) {
                SetDrawBlendMode(DX_BLENDMODE_ALPHA, 60 - i * 20);
                DrawCircleAA(drawX, drawY, 34.0f + i * 8.0f + pulse, 64, itemCol, TRUE);
            }
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

            // ② 本体（多層クリスタル構造）
            DrawCircleAA(drawX, drawY, 28.0f, 64, darkCol, TRUE);  // 外郭（暗）
            DrawCircleAA(drawX, drawY, 24.0f, 64, itemCol, TRUE);  // 本体

            // ③ 内部発光（加算合成）
            SetDrawBlendMode(DX_BLENDMODE_ADD, 150);
            DrawCircleAA(drawX, drawY, 18.0f, 64, addCol, TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

            // ④ 硬質リング（はめ込み感）
            DrawCircleAA(drawX, drawY, 26.0f, 64, GetColor(200, 220, 255), FALSE, 2.0f);

            // ⑤ 光沢（左上のハイライト）
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 180);
            DrawCircleAA(drawX - 8.0f, drawY - 8.0f, 6.0f, 32, GetColor(255, 255, 255), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

            // ⑥ 記号（影付き白文字）
            SetFontSize(38);
            char symStr[2] = { sp.currentSymbol, '\0' };
            int tw = GetDrawStringWidth(symStr, 1);

            // ドロップシャドウ（彫り込み感）
            DrawString((int)drawX - tw / 2 + 2, (int)drawY - 18 + 2, symStr, GetColor(20, 20, 30));
            // 本体文字（発光する白）
            DrawString((int)drawX - tw / 2, (int)drawY - 18, symStr, GetColor(255, 255, 255));
        }
    }

} // namespace App