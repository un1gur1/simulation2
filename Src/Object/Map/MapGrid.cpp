#define NOMINMAX
#include "MapGrid.h"
#include <DxLib.h>
#include <cmath>
#include <string>

namespace App {

    MapGrid::MapGrid(int tileSize, Vector2 offset)
        : m_tileSize(tileSize)
        , m_offset(offset)
        , m_ruleMode(BattleRuleMode::CLASSIC)
        , m_totalTurns(0)
        , m_currentCycleTick(0)
        , m_spawnInterval(3)
    {
        // 初期化は SetRuleMode で行う
    }

    void MapGrid::SetRuleMode(BattleRuleMode mode) {
        m_ruleMode = mode;

        // モード別の湧き間隔を設定
        if (m_ruleMode == BattleRuleMode::CLASSIC) {
            m_spawnInterval = 3;  // クラシック: 3ターンごと
        }
        else {
            m_spawnInterval = 2;  // ゼロワン: 2ターンごと
        }

        // モード別の初期化を実行
        InitializeSpawnPoints();
    }

    void MapGrid::InitializeSpawnPoints() {
        m_spawnPoints.clear();

        if (m_ruleMode == BattleRuleMode::CLASSIC) {
            InitializeClassicSpawns();
        }
        else {
            InitializeZeroOneSpawns();
        }
    }

    void MapGrid::InitializeClassicSpawns() {
        // ★ クラシックモード（ノーマルバトル）
        // 【配置戦略】
        // - 1~3: 縦横移動（3マス/2マス/1マス） → 十字交差点に配置
        // - 4~6: 斜め移動（3マス/2マス/1マス） → 対角線上に配置
        // - 7~9: 全方向移動（3マス/2マス/1マス） → 中央・中間地点に配置
        // - '÷' は移動の中継点となる戦略的位置に配置

        struct PointDef { int x, y; char op; };
        PointDef defs[] = {
            // 【中央十字エリア】- 縦横移動（1~3）が最も効率的にアクセス可能
            { 4, 1, '+' },  // 上辺中央：スタート地点から縦移動で到達しやすい
            { 4, 7, '+' },  // 下辺中央：同上
            { 1, 4, '-' },  // 左辺中央：横移動で到達しやすい
            { 7, 4, '-' },  // 右辺中央：同上

            // 【四隅】- 斜め移動（4~6）が最も効率的、全方向移動（7~9）も2手で到達
            { 2, 2, '*' },  // 左上：斜め移動の要所、攻撃的な'*'を配置
            { 6, 2, '/' },  // 右上：陣地構築用、安全な位置
            { 2, 6, '/' },  // 左下：同上
            { 6, 6, '*' },  // 右下：斜め移動の要所、攻撃的な'*'を配置

            // 【中央】- 全方向からアクセス可能な最重要争奪地点
            { 4, 4, '+' },  // 中心：全ての数値から最もアクセスしやすい汎用演算子

            // 【副次的戦略ポイント】- 斜め＋縦横の中間地点
            { 3, 3, '-' },  // 左上寄り：低コストで左上隅の'*'への中継点
            { 5, 5, '-' },  // 右下寄り：低コストで右下隅の'*'への中継点
        };

        for (const auto& d : defs) {
            SpawnPoint sp;
            sp.pos = { d.x, d.y };
            // ★ クラシックモードでは単一の演算子を繰り返す
            sp.sequence = { d.op };
            sp.currentIndex = 0;
            sp.currentSymbol = d.op;
            sp.isAvailable = true;
            m_spawnPoints.push_back(sp);
        }
    }

    void MapGrid::InitializeZeroOneSpawns() {
        // ★ ゼロワンモード（カウントバトル）
        // 【配置戦略】
        // - スコア調整に必須の '+', '-' を中央・アクセスしやすい位置に多数配置
        // - '*' は倍率調整用に中間地点へ配置
        // - '/' は分数スコア操作と陣地構築の二刀流
        // - ローテーション制で戦略性を高める

        // ★ '/' を追加したシーケンス
        std::vector<char> zeroOneSeq = { '+', '-', '*', '/', '+', '-' };

        struct PointDef { int x, y, startIdx; };
        PointDef defs[] = {
            // 【中央十字ライン】- 最もアクセスしやすい位置に '+', '-' を集中配置
            { 4, 2, 0 },  // 上寄り: + スタート
            { 4, 6, 1 },  // 下寄り: - スタート
            { 2, 4, 1 },  // 左寄り: - スタート
            { 6, 4, 0 },  // 右寄り: + スタート

            // 【内側四隅】- 斜め移動での中継点、戦略的な争奪ポイント
            { 3, 3, 0 },  // 左上: + スタート
            { 5, 3, 1 },  // 右上: - スタート
            { 3, 5, 2 },  // 左下: * スタート
            { 5, 5, 3 },  // 右下: / スタート

            // 【外側四隅】- 移動コストは高いが、陣取り要素として重要
            { 2, 2, 4 },  // 左上: + スタート
            { 6, 2, 5 },  // 右上: - スタート
            { 2, 6, 3 },  // 左下: / スタート
            { 6, 6, 2 },  // 右下: * スタート

            // 【中央争奪ポイント】- 全方向からアクセス可能
            { 4, 4, 2 },  // 中心: * スタート（倍率調整の要）

            // 【辺の追加ポイント】- 数値が低い時でも到達しやすい位置
            { 4, 1, 0 },  // 上辺: + スタート
            { 4, 7, 4 },  // 下辺: + スタート
            { 1, 4, 1 },  // 左辺: - スタート
            { 7, 4, 5 },  // 右辺: - スタート
        };

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
    char MapGrid::PickUpItem(int x, int y) {
        for (auto& sp : m_spawnPoints) {
            if (sp.pos.x == x && sp.pos.y == y && sp.isAvailable) {
                char picked = sp.currentSymbol;
                sp.isAvailable = false; // 取られた状態にする

                // 次のアイテムへ進める
                sp.currentIndex = (sp.currentIndex + 1) % sp.sequence.size();
                sp.currentSymbol = sp.sequence[sp.currentIndex];

                return picked;
            }
        }
        return '\0';
    }

    void MapGrid::UpdateTurn() {
        m_totalTurns++;
        m_currentCycleTick++;

        if (m_currentCycleTick >= m_spawnInterval) {
            m_currentCycleTick = 0;

            for (auto& sp : m_spawnPoints) {
                // 取られている（空の）場所だけ、次を出現させる
                if (!sp.isAvailable) {
                    sp.isAvailable = true;
                }
            }
        }
    }

    IntVector2 MapGrid::ScreenToGrid(const Vector2& pos) const {
        return {
            static_cast<int>((pos.x - m_offset.x) / m_tileSize),
            static_cast<int>((pos.y - m_offset.y) / m_tileSize)
        };
    }

    Vector2 MapGrid::GetCellCenter(int x, int y) const {
        return {
            m_offset.x + x * m_tileSize + m_tileSize / 2.0f,
            m_offset.y + y * m_tileSize + m_tileSize / 2.0f
        };
    }

    bool MapGrid::IsWithinBounds(int x, int y) const {
        return x >= 0 && x < 9 && y >= 0 && y < 9;
    }

    char MapGrid::GetItemAt(int x, int y) const {
        for (const auto& sp : m_spawnPoints) {
            if (sp.pos.x == x && sp.pos.y == y && sp.isAvailable) {
                return sp.currentSymbol;
            }
        }
        return '\0';
    }

    void MapGrid::Draw() const {
        // --- 1. 盤面描画 ---
        // 紺色ベースの画面に完全に馴染む、深く静かなマス目
        unsigned int lineCol = GetColor(40, 45, 60);
        unsigned int fillCol = GetColor(15, 18, 25);

        for (int y = 0; y < 9; y++) {
            for (int x = 0; x < 9; x++) {
                Vector2 pos = GetCellCenter(x, y);
                // 四角形はジャギーが出ないのでそのままDrawBoxでOK
                DrawBox((int)pos.x - 39, (int)pos.y - 39, (int)pos.x + 39, (int)pos.y + 39, lineCol, FALSE);
                DrawBox((int)pos.x - 38, (int)pos.y - 38, (int)pos.x + 38, (int)pos.y + 38, fillCol, TRUE);
            }
        }

        // --- 2. アイテム描画（高級盤面はめ込み ＆ ツルツルAA仕様） ---
        double time = GetNowCount() / 1000.0;
        // ★ floatにして滑らかに脈動させる
        float pulse = (float)(sin(time * 5.0) * 4.0);

        for (const auto& sp : m_spawnPoints) {
            Vector2 basePos = GetCellCenter(sp.pos.x, sp.pos.y);
            // ★ AA用に座標もfloatで保持
            float drawX = basePos.x;
            float drawY = basePos.y;

            if (!sp.isAvailable) {
                // ★ ホログラム（出現待ち）演出：ダークな盤面に浮かぶ電子予測
                SetDrawBlendMode(DX_BLENDMODE_ADD, 120);
                // ★ DrawCircleAAに変更（第4引数はポリゴン分割数。64でかなり綺麗）
                DrawCircleAA(drawX, drawY, 22.0f + pulse / 2.0f, 64, GetColor(80, 100, 120), FALSE, 2.0f);

                SetFontSize(24);
                char waitStr[2] = { sp.currentSymbol, '\0' };
                int tw = GetDrawStringWidth(waitStr, 1);
                // 文字はボヤけないように int にキャスト
                DrawString((int)drawX - tw / 2, (int)drawY - 12, waitStr, GetColor(100, 140, 180));
                SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
                continue;
            }

            // アイテムの色決定（宝石のようなリッチで深い色合い）
            unsigned int itemCol, addCol, darkCol;
            if (sp.currentSymbol == '+') {
                itemCol = GetColor(200, 30, 50);  // ルビー（赤）
                addCol = GetColor(255, 100, 100);
                darkCol = GetColor(80, 10, 20);
            }
            else if (sp.currentSymbol == '-') {
                itemCol = GetColor(30, 120, 220); // サファイア（青）
                addCol = GetColor(100, 180, 255);
                darkCol = GetColor(10, 30, 80);
            }
            else if (sp.currentSymbol == '*') {
                itemCol = GetColor(40, 180, 60);  // エメラルド（緑）
                addCol = GetColor(120, 255, 150);
                darkCol = GetColor(15, 60, 20);
            }
            else {
                itemCol = GetColor(180, 40, 200); // アメジスト（紫）
                addCol = GetColor(220, 100, 255);
                darkCol = GetColor(60, 10, 80);
            }

            // ① 盤面に漏れ出すエネルギー（オーラ）
            for (int i = 0; i < 2; ++i) {
                SetDrawBlendMode(DX_BLENDMODE_ALPHA, 60 - i * 20);
                DrawCircleAA(drawX, drawY, 34.0f + i * 8.0f + pulse, 64, itemCol, TRUE);
            }
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

            // ② アイテム本体（多層クリスタル構造）
            DrawCircleAA(drawX, drawY, 28.0f, 64, darkCol, TRUE); // 外郭の暗い色
            DrawCircleAA(drawX, drawY, 24.0f, 64, itemCol, TRUE); // メインカラー

            // ③ 加算合成で内部からの発光感
            SetDrawBlendMode(DX_BLENDMODE_ADD, 150);
            DrawCircleAA(drawX, drawY, 18.0f, 64, addCol, TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

            // ④ 盤面にはめ込まれているような硬質なリング
            DrawCircleAA(drawX, drawY, 26.0f, 64, GetColor(200, 220, 255), FALSE, 2.0f);

            // ⑤ 強い光沢
            SetDrawBlendMode(DX_BLENDMODE_ALPHA, 180);
            DrawCircleAA(drawX - 8.0f, drawY - 8.0f, 6.0f, 32, GetColor(255, 255, 255), TRUE);
            SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

            // ⑥ 記号（ユニットに合わせて「白文字＋暗い影」に変更）
            SetFontSize(38);
            char symStr[2] = { sp.currentSymbol, '\0' };
            int tw = GetDrawStringWidth(symStr, 1);

            // 彫り込まれている感のあるドロップシャドウ
            DrawString((int)drawX - tw / 2 + 2, (int)drawY - 18 + 2, symStr, GetColor(20, 20, 30));
            // 本体文字（発光する白）
            DrawString((int)drawX - tw / 2, (int)drawY - 18, symStr, GetColor(255, 255, 255));
        }
    }

} // namespace App