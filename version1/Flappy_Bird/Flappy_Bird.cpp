#include <easyx.h>
#include <graphics.h>
#include <iostream>
#include <stdio.h>
#include <string> 
#include <string.h>
#include <algorithm>
#include <vector>
#include <fstream>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

// Resources
#define PI 3.14159265359
bool GAME_START = FALSE;        // To check if game has started, i.e. a mouse click happened
bool GAME_END = FALSE;          // To check if game has end, i.e. a collision occured

// Game Modes
enum GameMode {
    MODE_NONE = 0,
    MODE_ENDLESS = 1,
    MODE_LEVEL = 2
};
int GAME_MODE = MODE_NONE;
int CURRENT_LEVEL = 1;
const int MAX_LEVEL = 10;
int LEVEL_THRESHOLD = 5;         // points per level

// UI (simple rectangles)
struct RectBtn { int x; int y; int w; int h; };
RectBtn btn_mode_endless;        // start screen buttons
RectBtn btn_mode_level;
RectBtn btn_settings;            // endless settings gear button (top-right)
bool settings_open = false;

// Window
int WIDTH = 0;
int HEIGHT = 0;

// Background
IMAGE  background;
IMAGE  background_day;
IMAGE  background_night;
int background_theme = 0; // 0-day, 1-night
bool GAME_PAUSED = false;        // pause when settings open or countdown
int resume_countdown_frames = 0; // 3*FPS frames countdown when resuming
bool VICTORY = false;            // level victory state
int LEVEL_STARTED = 1;           // level actually started (for unlock)

// Ground
struct Ground
{
    int x = 0;
    int y = 420;
    int speed = 3;
    IMAGE image;
} ground;


// Time
unsigned long time1, time2;
int FPS = 60;


// Bird
typedef struct Bird
{
    int x = 30;
    int y = 200;
    int size_x = 0;
    int size_y = 0;
    int speed = 0;
    int frame = 0;
    int g = 0;
    int num_image = 3;
    IMAGE image[3][2];
    IMAGE image_rotated[3][2];
};
int SPEED_UP = -8;
int G = 1;
Bird bird;

// Endless settings (runtime configurable)
int config_pipe_thickness = 100;    // percent
int config_bird_horz_speed = 3;     // maps to pipe speed / ground speed
int config_gravity = 1;             // maps to G
int config_jump_strength = 8;       // maps to -SPEED_UP
int music_index = 1;                // 0: bgm.wav, 1: bgm2.wav

// Settings panel controls (computed while drawing panel)
RectBtn btn_minus_horz, btn_plus_horz;
RectBtn btn_minus_grav, btn_plus_grav;
RectBtn btn_minus_jump, btn_plus_jump;
RectBtn btn_minus_thick, btn_plus_thick;
RectBtn btn_toggle_bg, btn_toggle_music;

// Level select & targets
bool LEVEL_SELECT = false;
RectBtn level_btns[10];
int LEVEL_TARGETS[10] = { 5,10,13,16,19,22,25,28,31,34 };
int level_target = 0;
bool LEVEL_COMPLETE = false;
bool level_unlocked[10] = { true, false, false, false, false, false, false, false, false, false };
bool level_stop_spawn = false;   // stop spawning new pipes after last one passed
RectBtn btn_back;                // back button in level mode
bool level_last_phase = false;   // enter last phase after target reached
int pipe_base_y[2] = { 0,0 };
int pipe_osc_offset[2] = { 0,0 };
int pipe_osc_dir[2] = { 1,1 };


// Pipe
// Two pair of pipes, each pair combines the top one and the buttom one
typedef struct Pipe
{
    int x[2] = { 0, 0 };
    int y[2] = { 0, 0 };
    int size_x[2] = { 0, 0 };
    int size_y[2] = { 0, 0 };
    int offset[2] = { 0, 0 };           // To decide the distence between the top one and the buttom one
    int speed = 0;
    IMAGE image[2];
    IMAGE mask[2];
    int base_size_x[2] = { 0, 0 };
    int base_size_y[2] = { 0, 0 };
};
int SPEED_PIPE = 3;
Pipe pipe_green;

// Pipe thickness control (percentage of base width)
int pipe_thickness_percent = 100;

void setPipeThicknessPercent(int percent)
{
    // 取消按贴图缩放，避免遮罩变黑问题；始终使用原始贴图尺寸
    if (percent < 30) percent = 30;
    if (percent > 200) percent = 200;
    pipe_thickness_percent = percent;
    pipe_green.size_x[0] = pipe_green.base_size_x[0];
    pipe_green.size_y[0] = pipe_green.base_size_y[0];
    pipe_green.size_x[1] = pipe_green.base_size_x[1];
    pipe_green.size_y[1] = pipe_green.base_size_y[1];
}

int calcLevelPipeThicknessPercent(int level)
{
    if (level < 1) level = 1;
    if (level > MAX_LEVEL) level = MAX_LEVEL;
    // Level 1: 60%, Level 10: 96% (increase ~4% per level)
    return 60 + (level - 1) * 4;
}

void applyMusic()
{
    mciSendString("stop bgm", 0, 0, 0);
    mciSendString("close bgm", 0, 0, 0);
    if (music_index == 0) {
        mciSendString("open sound\\bgm.wav alias bgm TYPE MPEGVideo ", 0, 0, 0);
    }
    else {
        mciSendString("open sound\\bgm2.wav alias bgm TYPE MPEGVideo ", 0, 0, 0);
    }
    mciSendString("play bgm repeat", 0, 0, 0);
}

void applyConfig()
{
    // Apply to gameplay parameters
    setPipeThicknessPercent(config_pipe_thickness);
    G = config_gravity;
    SPEED_UP = -config_jump_strength;
    SPEED_PIPE = config_bird_horz_speed;
    ground.speed = config_bird_horz_speed;
}


// Score
struct Score
{
    int point = 0;
    int y = 5;
    IMAGE image[10];
    IMAGE mask[10];
} score;


// Text
typedef struct Text {
    int x = 0;
    int y = 0;
    //bool display = true;
    IMAGE image = NULL;
    IMAGE mask = NULL;
};
// 1. 首先确保已经添加了按钮图片变量（在Text结构体定义后）
// 在Text结构体定义后添加按钮图片变量
Text game_over;
Text tutorial;
Text title;
Text button_play;
// 添加新的按钮图片变量
IMAGE btn_endless_img;
IMAGE btn_level_img;

// 添加关卡按钮图片变量
IMAGE level_btn_images[10];


// Progress & Achievements (simple persistence)
int high_score = 0;
int total_exp = 0;
bool achievements_open = false;
bool game_over_processed = false;

RectBtn btn_go_menu;     // 返回主菜单
RectBtn btn_restart;     // 重玩
RectBtn btn_view_achv;   // 查看成就

// Enemy bird (black) from level 4
struct EnemyBird {
    int x; int y; int w; int h; int speed;
    bool active;
};
EnemyBird enemy_bird = { 0, 0, 32, 24, 5, false };
IMAGE enemy_img; IMAGE enemy_mask;

// Coins
struct Coin { int x; int y; int r; int frame; bool taken; };
std::vector<Coin> coins;
int coins_collected = 0;
IMAGE coin_img;

void loadProgress()
{
    std::ifstream fin("save.dat", std::ios::in | std::ios::binary);
    if (!fin.good()) return;
    fin.read((char*)&high_score, sizeof(high_score));
    fin.read((char*)&total_exp, sizeof(total_exp));
    fin.close();
}

void saveProgress()
{
    std::ofstream fout("save.dat", std::ios::out | std::ios::binary | std::ios::trunc);
    if (!fout.good()) return;
    fout.write((char*)&high_score, sizeof(high_score));
    fout.write((char*)&total_exp, sizeof(total_exp));
    fout.close();
}

void onGameOver()
{
    if (score.point > high_score) high_score = score.point;
    total_exp += score.point;
    saveProgress();
}


// 在gameInitResource函数中加载按钮图片
void gameInitResource() {
    // BGM
    int rc = mciSendString("open sound\\bgm2.wav alias bgm TYPE MPEGVideo ", 0, 0, 0);
    rc = mciSendString("play bgm repeat", 0, 0, 0);
    rc = mciSendString("open sound\\jump.mp3 alias jump", 0, 0, 0);

    // Background
    loadimage(&background, "img\\background.png");
    WIDTH = background.getwidth();
    HEIGHT = background.getheight();

    // Window
    initgraph(WIDTH, HEIGHT, 1);

    // Additional backgrounds (scaled to window size)
    loadimage(&background_day, "img\\all_img\\bg_day.png", WIDTH, HEIGHT);
    loadimage(&background_night, "img\\all_img\\bg_night.png", WIDTH, HEIGHT);

    // Ground
    loadimage(&ground.image, "img/ground.png");

    // Bird
    for (int i = 0; i < 3; i++) {
        char address[30];

        sprintf_s(address, "img\\bird\\bird_1_%d.png", i);
        loadimage(&bird.image[i][0], address);
        loadimage(&bird.image_rotated[i][0], address);

        sprintf_s(address, "img\\bird\\bird_1_%d_mask.png", i);
        loadimage(&bird.image[i][1], address);
        loadimage(&bird.image_rotated[i][1], address);
    }


    // Pipe
    loadimage(&pipe_green.image[0], "img/pipe_green_top.png");
    loadimage(&pipe_green.mask[0], "img/pipe_green_top_mask.png");
    loadimage(&pipe_green.image[1], "img/pipe_green_down.png");
    loadimage(&pipe_green.mask[1], "img/pipe_green_down_mask.png");

    // Enemy bird & coin resources (fallback to simple recolor if not exists)
    loadimage(&enemy_img, "img/all_img/bird_oriange.png");
    loadimage(&enemy_mask, "img/all_img/black.png");
    loadimage(&coin_img, "img/all_img/medals.png");

    // Record base sizes for thickness scaling
    pipe_green.base_size_x[0] = pipe_green.image[0].getwidth();
    pipe_green.base_size_y[0] = pipe_green.image[0].getheight();
    pipe_green.base_size_x[1] = pipe_green.image[1].getwidth();
    pipe_green.base_size_y[1] = pipe_green.image[1].getheight();


    // score
    for (int i = 0; i < 10; i++) {
        char address[30];

        sprintf_s(address, "img\\score\\%d.png", i);
        loadimage(&score.image[i], address);

        sprintf_s(address, "img\\score\\%d_mask.png", i);
        loadimage(&score.mask[i], address);
    }


    // text
    loadimage(&game_over.image, "img\\text\\text_game_over.png");
    loadimage(&game_over.mask, "img\\text\\text_game_over_mask.png");

    loadimage(&tutorial.image, "img\\text\\tutorial.png");
    loadimage(&tutorial.mask, "img\\text\\tutorial_mask.png");

    loadimage(&button_play.image, "img\\text\\button_play.png");
    loadimage(&button_play.mask, "img\\text\\button_play_mask.png");

    loadimage(&title.image, "img\\text\\title.png");
    loadimage(&title.mask, "img\\text\\title_mask.png");

    // 加载按钮图片 - 修改为固定尺寸
    loadimage(&btn_endless_img, "img\\无尽模式.jpg", 140, 42);
    loadimage(&btn_level_img, "img\\关卡模式.jpg", 140, 42);

    // 加载关卡按钮图片
    for (int i = 0; i < 10; i++) {
        char address[30];
        if (i < 2) { // 1-2关使用.png格式
            sprintf_s(address, "img\\%d.png", i + 1);
        }
        else { // 3-10关使用.jpg格式
            sprintf_s(address, "img\\%d.jpg", i + 1);
        }
        loadimage(&level_btn_images[i], address, 56, 40); // 使用与原按钮相同的尺寸
    }

    loadProgress();
};


void gameInitValue() {
    srand(time(0));

    GAME_START = FALSE;
    GAME_END = FALSE;

    // score
    score.point = 0;
    score.y = 5;

    // pipe
    pipe_green.speed = 0;
    pipe_green.x[0] = ground.image.getwidth();
    pipe_green.x[1] = ground.image.getwidth() + 190;
    pipe_green.y[0] = rand() % 250;
    pipe_green.y[1] = rand() % 250;
    pipe_green.offset[0] = -305;
    pipe_green.offset[1] = 165;
    pipe_green.size_x[0] = pipe_green.image[0].getwidth();
    pipe_green.size_y[0] = pipe_green.image[0].getheight();
    pipe_green.size_x[1] = pipe_green.image[1].getwidth();
    pipe_green.size_y[1] = pipe_green.image[1].getheight();

    // Mode defaults
    CURRENT_LEVEL = 1;
    if (GAME_MODE == MODE_LEVEL) {
        setPipeThicknessPercent(calcLevelPipeThicknessPercent(CURRENT_LEVEL));
    }
    else {
        setPipeThicknessPercent(pipe_thickness_percent);
    }

    // bird
    bird.x = 30;
    bird.y = 200;
    bird.g = 0;
    bird.speed = 0;
    bird.frame = 0;
    bird.size_x = bird.image[0][0].getwidth() * 0.8;
    bird.size_y = bird.image[0][0].getheight() * 0.8;

    // Time
    time1 = GetTickCount();
    time2 = GetTickCount();

    // Text
    button_play.x = (WIDTH - button_play.image.getwidth()) / 2;
    button_play.y = HEIGHT * 0.8;
    tutorial.x = (WIDTH - tutorial.image.getwidth()) / 2;
    tutorial.y = HEIGHT * 0.4;  // 修改为更靠上的位置，避免与按钮重叠
    title.x = (WIDTH - title.image.getwidth()) / 2;
    title.y = HEIGHT * 0.1;
    game_over.x = (WIDTH - game_over.image.getwidth()) / 2;
    game_over.y = HEIGHT * 0.1;

    // Mode select buttons
    int btnW = 140, btnH = 42, gap = 20;
    btn_mode_endless.w = btnW; btn_mode_endless.h = btnH;
    btn_mode_level.w = btnW; btn_mode_level.h = btnH;
    btn_mode_endless.x = WIDTH / 2 - btnW - gap / 2;
    btn_mode_level.x = WIDTH / 2 + gap / 2;
    btn_mode_endless.y = HEIGHT * 0.65;
    btn_mode_level.y = HEIGHT * 0.65;

    // Settings gear button (top-right)
    btn_settings.w = 32; btn_settings.h = 32;
    btn_settings.x = WIDTH - btn_settings.w - 10;
    btn_settings.y = 10;

    // Level select grid buttons (precompute positions)
    int gridCols = 5;
    int gridRows = 2;
    int cellW = 56, cellH = 40;
    int startX = (WIDTH - gridCols * cellW - (gridCols - 1) * 10) / 2;
    int startY = HEIGHT * 0.5 + 60;
    for (int i = 0; i < 10; i++) {
        int r = i / gridCols, c = i % gridCols;
        level_btns[i].x = startX + c * (cellW + 10);
        level_btns[i].y = startY + r * (cellH + 10);
        level_btns[i].w = cellW;
        level_btns[i].h = cellH;
    }
    // Game Over action buttons layout
    int bw = 120, bh = 42; int spacing = 12;
    btn_go_menu.w = bw; btn_go_menu.h = bh;
    btn_restart.w = bw; btn_restart.h = bh;
    btn_view_achv.w = bw; btn_view_achv.h = bh;
    int totalW = bw * 3 + spacing * 2;
    int baseX = (WIDTH - totalW) / 2;
    int baseY = HEIGHT * 0.8;
    btn_go_menu.x = baseX; btn_go_menu.y = baseY;
    btn_restart.x = baseX + bw + spacing; btn_restart.y = baseY;
    btn_view_achv.x = baseX + (bw + spacing) * 2; btn_view_achv.y = baseY;
    game_over_processed = false;
    GAME_PAUSED = false;
    resume_countdown_frames = 0;
    level_stop_spawn = false;
    // back button in level mode
    btn_back.w = 72; btn_back.h = 28; btn_back.x = 10; btn_back.y = 10;

    // enemy bird & coins reset
    enemy_bird.active = false;
    coins.clear(); coins_collected = 0;
}

void gameDraw() {
    BeginBatchDraw();                                               // Start drawing
    // Background theme
    if (background_theme == 0) putimage(0, 0, &background_day);
    else putimage(0, 0, &background_night);


    // Put mask first with AND operation ==> Will make whiet area transparent and leave central black area
    // Put the original graph with OR operation ==> Will overlay the black area
    if (GAME_START) {
        for (int i = 0; i < 2; i++) {
            putimage(pipe_green.x[i], pipe_green.y[i] + pipe_green.offset[0], &pipe_green.mask[0], SRCAND);         // Put upper pipe
            putimage(pipe_green.x[i], pipe_green.y[i] + pipe_green.offset[0], &pipe_green.image[0], SRCPAINT);
            putimage(pipe_green.x[i], pipe_green.y[i] + pipe_green.offset[1], &pipe_green.mask[1], SRCAND);         // Put buttom pipe
            putimage(pipe_green.x[i], pipe_green.y[i] + pipe_green.offset[1], &pipe_green.image[1], SRCPAINT);
        }
    }

    putimage(ground.x, ground.y, &ground.image);

    putimage(bird.x, bird.y, &bird.image_rotated[bird.frame][1], SRCAND);
    putimage(bird.x, bird.y, &bird.image_rotated[bird.frame][0], SRCPAINT);


    if (GAME_START) {                                                                                           // Put score
        std::string credit = std::to_string(score.point);
        for (int i = 0; i < credit.size(); i++) {
            putimage(background.getwidth() / 2 - ((int)credit.size() / 2 - i + 0.5) * score.mask[0].getwidth(),
                score.y,
                &score.mask[credit[i] - 48],
                SRCAND
            );

            putimage(background.getwidth() / 2 - ((int)credit.size() / 2 - i + 0.5) * score.image[0].getwidth(),
                score.y,
                &score.image[credit[i] - 48],
                SRCPAINT
            );
        }
    }



    // Text
    if (!GAME_START) {
        putimage(tutorial.x, tutorial.y, &tutorial.mask, SRCAND);
        putimage(tutorial.x, tutorial.y, &tutorial.image, SRCPAINT);

        putimage(title.x, title.y, &title.mask, SRCAND);
        putimage(title.x, title.y, &title.image, SRCPAINT);

        // 只在未选择模式时显示模式选择按钮
        if (GAME_MODE == MODE_NONE) {
            // 使用图片按钮替代文本按钮
            putimage(btn_mode_endless.x, btn_mode_endless.y, &btn_endless_img);
            putimage(btn_mode_level.x, btn_mode_level.y, &btn_level_img);
        }

        // 如果选择了关卡模式，显示关卡选择界面
        if (GAME_MODE == MODE_LEVEL) {
            // 使用图片按钮替代文本按钮
            for (int i = 0; i < 10; i++) {
                // 直接绘制图片按钮
                putimage(level_btns[i].x, level_btns[i].y, &level_btn_images[i]);
            }
        }
    }

    // Endless settings gear (top-right) visible when endless mode selected and game not over
    if (GAME_MODE == MODE_ENDLESS && !GAME_END) {
        setfillcolor(RGB(255, 230, 120));
        solidrectangle(btn_settings.x, btn_settings.y, btn_settings.x + btn_settings.w, btn_settings.y + btn_settings.h);
        setlinecolor(RGB(180, 140, 40));
        rectangle(btn_settings.x, btn_settings.y, btn_settings.x + btn_settings.w, btn_settings.y + btn_settings.h);
        settextcolor(RGB(50, 40, 20));
        outtextxy(btn_settings.x + 10, btn_settings.y + 12, "设置");
    }

    // Settings panel overlay
    if (settings_open) {
        int px = WIDTH - 240, py = 50, pw = 230, ph = 200;
        setfillcolor(RGB(255, 255, 255));
        solidrectangle(px, py, px + pw, py + ph);
        settextcolor(RGB(0, 0, 0));
        // Panel header with background capsule for better readability
        setfillcolor(RGB(240, 240, 255));
        solidrectangle(px + 6, py + 6, px + pw - 6, py + 28);
        settextcolor(RGB(60, 60, 120));
        outtextxy(px + 12, py + 8, "无尽设置");
        // Lines: horz speed, gravity, jump, thickness, bg, music
        int ly = py + 32;
        auto drawRow = [&](const char* label, int value, RectBtn& minusBtn, RectBtn& plusBtn) {
            // label with small background
            setfillcolor(RGB(245, 245, 245));
            solidrectangle(px + 6, ly - 2, px + 100, ly + 20);
            settextcolor(RGB(40, 40, 40));
            outtextxy(px + 10, ly, label);
            char buf[16]; sprintf_s(buf, "%d", value);
            setfillcolor(RGB(235, 250, 235));
            solidrectangle(px + 106, ly - 2, px + 146, ly + 20);
            settextcolor(RGB(20, 80, 20));
            outtextxy(px + 110, ly, buf);
            minusBtn.x = px + 150; minusBtn.y = ly; minusBtn.w = 20; minusBtn.h = 20;
            plusBtn.x = px + 175; plusBtn.y = ly; plusBtn.w = 20; plusBtn.h = 20;
            solidrectangle(minusBtn.x, minusBtn.y, minusBtn.x + minusBtn.w, minusBtn.y + minusBtn.h);
            solidrectangle(plusBtn.x, plusBtn.y, plusBtn.x + plusBtn.w, plusBtn.y + plusBtn.h);
            outtextxy(minusBtn.x + 6, minusBtn.y + 2, "-");
            outtextxy(plusBtn.x + 5, plusBtn.y + 2, "+");
            ly += 26;
            };
        drawRow("水平速度", config_bird_horz_speed, btn_minus_horz, btn_plus_horz);
        drawRow("重力", config_gravity, btn_minus_grav, btn_plus_grav);
        drawRow("跳跃力度", config_jump_strength, btn_minus_jump, btn_plus_jump);
        drawRow("管子粗细%", config_pipe_thickness, btn_minus_thick, btn_plus_thick);
        // Toggles
        btn_toggle_bg.x = px + 10; btn_toggle_bg.y = ly; btn_toggle_bg.w = 90; btn_toggle_bg.h = 22;
        setfillcolor(RGB(230, 240, 255));
        solidrectangle(btn_toggle_bg.x, btn_toggle_bg.y, btn_toggle_bg.x + btn_toggle_bg.w, btn_toggle_bg.y + btn_toggle_bg.h);
        outtextxy(btn_toggle_bg.x + 10, btn_toggle_bg.y + 2, background_theme == 0 ? "白天" : "夜晚");
        btn_toggle_music.x = px + 110; btn_toggle_music.y = ly; btn_toggle_music.w = 90; btn_toggle_music.h = 22;
        setfillcolor(RGB(230, 255, 235));
        solidrectangle(btn_toggle_music.x, btn_toggle_music.y, btn_toggle_music.x + btn_toggle_music.w, btn_toggle_music.y + btn_toggle_music.h);
        outtextxy(btn_toggle_music.x + 10, btn_toggle_music.y + 2, music_index == 0 ? "音乐1" : "音乐2");
    }
    // Countdown overlay when resuming
    if (resume_countdown_frames > 0) {
        int seconds = (resume_countdown_frames + FPS - 1) / FPS;
        settextstyle(48, 0, "微软雅黑");
        settextcolor(RGB(255, 255, 255));
        char c[4]; sprintf_s(c, "%d", seconds);
        outtextxy(WIDTH / 2 - 12, HEIGHT / 2 - 24, c);
        settextstyle(16, 0, "宋体");
    }
    // Draw enemy bird
    if (enemy_bird.active && GAME_START && !GAME_END) {
        putimage(enemy_bird.x, enemy_bird.y, &enemy_mask, SRCAND);
        putimage(enemy_bird.x, enemy_bird.y, &enemy_img, SRCPAINT);
    }
    // Draw coins
    if (GAME_START && !GAME_END) {
        for (auto& co : coins) {
            if (co.taken) continue;
            int rr = co.r + (co.frame % 10 < 5 ? 1 : -1); // simple pulsate
            setfillcolor(RGB(255, 215, 0));
            solidcircle(co.x, co.y, rr);
            co.frame = (co.frame + 1) % 10;
        }
        settextcolor(RGB(255, 255, 0));
        char cbuf[32]; sprintf_s(cbuf, "金币:%d", coins_collected);
        outtextxy(10, 40, cbuf);
    }
    if (GAME_END) {
        // one-time progress update
        if (!game_over_processed) {
            onGameOver();
            game_over_processed = true;
            if (GAME_MODE == MODE_LEVEL) {
                int idx = LEVEL_STARTED - 1; // strictly use started level index
                if (idx >= 0 && idx < 9 && LEVEL_COMPLETE) {
                    // unlock exactly the next after the started level
                    level_unlocked[idx + 1] = true;
                }
            }
        }

        // Title
        putimage(game_over.x, game_over.y, &game_over.mask, SRCAND);
        putimage(game_over.x, game_over.y, &game_over.image, SRCPAINT);

        // 只在这里设置文本背景为透明，这将消除黑色色块
        setbkmode(TRANSPARENT);

        // Score & Stars
        settextcolor(RGB(255, 255, 0));
        char thisScore[32]; sprintf_s(thisScore, "本次分数: %d", score.point);
        outtextxy(WIDTH / 2 - 80, HEIGHT * 0.32, thisScore);
        // Stars by thresholds
        int star = (score.point >= 30) ? 3 : (score.point >= 20 ? 2 : (score.point >= 10 ? 1 : 0));
        for (int i = 0; i < star; i++) {
            setfillcolor(RGB(255, 215, 0));
            solidcircle(WIDTH / 2 - 60 + i * 60, HEIGHT * 0.42, 12);
        }

        // High score and coins
        settextcolor(RGB(230, 240, 255));
        char best[32]; sprintf_s(best, "历史最高分: %d", high_score);
        outtextxy(WIDTH / 2 - 90, HEIGHT * 0.50, best);
        char coinbuf[32]; sprintf_s(coinbuf, "本关金币: %d", coins_collected);
        settextcolor(RGB(255, 230, 120)); outtextxy(WIDTH / 2 - 80, HEIGHT * 0.46, coinbuf);

        // Level bar from total_exp
        int level = total_exp / 50 + 1;
        int expInLevel = total_exp % 50;
        char lvbuf[32]; sprintf_s(lvbuf, "当前等级: Lv.%d", level);
        outtextxy(WIDTH / 2 - 90, HEIGHT * 0.56, lvbuf);
        // progress bar
        int bx = WIDTH / 2 - 100, by = HEIGHT * 0.60, bw = 200, bh = 14;
        setlinecolor(RGB(0, 0, 0)); rectangle(bx, by, bx + bw, by + bh);
        setfillcolor(RGB(80, 220, 120));
        int fillw = bw * expInLevel / 50; solidrectangle(bx + 1, by + 1, bx + fillw, by + bh - 1);
        char xp[32]; sprintf_s(xp, "%d/50经验", expInLevel);
        settextcolor(RGB(0, 0, 0)); outtextxy(WIDTH / 2 - 30, by - 18, xp);

        // Buttons
        setfillcolor(RGB(180, 120, 80));
        solidrectangle(btn_go_menu.x, btn_go_menu.y, btn_go_menu.x + btn_go_menu.w, btn_go_menu.y + btn_go_menu.h);
        settextcolor(RGB(255, 255, 255)); outtextxy(btn_go_menu.x + 18, btn_go_menu.y + 12, "返回主菜单");

        setfillcolor(RGB(80, 180, 120));
        solidrectangle(btn_restart.x, btn_restart.y, btn_restart.x + btn_restart.w, btn_restart.y + btn_restart.h);
        settextcolor(RGB(255, 255, 255)); outtextxy(btn_restart.x + 38, btn_restart.y + 12, "重玩");

        setfillcolor(RGB(150, 80, 180));
        solidrectangle(btn_view_achv.x, btn_view_achv.y, btn_view_achv.x + btn_view_achv.w, btn_view_achv.y + btn_view_achv.h);
        settextcolor(RGB(255, 255, 255)); outtextxy(btn_view_achv.x + 18, btn_view_achv.y + 12, "查看成就");
    }


    EndBatchDraw();                                                 // End drawing
};


void gameUpdate() {
    // Player update
    MOUSEMSG msg = { 0 };
    if (MouseHit()) {
        msg = GetMouseMsg();
        if (msg.uMsg == WM_LBUTTONDOWN) {
            if (!GAME_START) {                                          // First-time click or mode select
                // If mode not chosen, check buttons
                if (GAME_MODE == MODE_NONE) {
                    bool hitEndless = (msg.x > btn_mode_endless.x && msg.x < btn_mode_endless.x + btn_mode_endless.w &&
                        msg.y > btn_mode_endless.y && msg.y < btn_mode_endless.y + btn_mode_endless.h);
                    bool hitLevel = (msg.x > btn_mode_level.x && msg.x < btn_mode_level.x + btn_mode_level.w &&
                        msg.y > btn_mode_level.y && msg.y < btn_mode_level.y + btn_mode_level.h);
                    if (hitEndless) {
                        GAME_MODE = MODE_ENDLESS;
                        setPipeThicknessPercent(pipe_thickness_percent);
                    }
                    else if (hitLevel) {
                        GAME_MODE = MODE_LEVEL;
                        CURRENT_LEVEL = 1;
                        setPipeThicknessPercent(calcLevelPipeThicknessPercent(CURRENT_LEVEL));
                        LEVEL_SELECT = true;
                    }
                }
                // Level mode requires selecting a level
                if (GAME_MODE == MODE_LEVEL && LEVEL_SELECT) {
                    // check clicks on level buttons
                    for (int i = 0; i < 10; i++) {
                        if (msg.x > level_btns[i].x && msg.x < level_btns[i].x + level_btns[i].w &&
                            msg.y > level_btns[i].y && msg.y < level_btns[i].y + level_btns[i].h) {
                            CURRENT_LEVEL = i + 1;
                            LEVEL_STARTED = CURRENT_LEVEL; // record started level
                            level_target = LEVEL_TARGETS[i];
                            setPipeThicknessPercent(calcLevelPipeThicknessPercent(CURRENT_LEVEL));
                            LEVEL_SELECT = false;
                            GAME_START = TRUE;
                            bird.g = G;
                            pipe_green.speed = SPEED_PIPE;
                            // init oscillation base y
                            pipe_base_y[0] = pipe_green.y[0];
                            pipe_base_y[1] = pipe_green.y[1];
                            pipe_osc_offset[0] = pipe_osc_offset[1] = 0;
                            pipe_osc_dir[0] = pipe_osc_dir[1] = 1;
                        }
                    }
                }
                else if (GAME_MODE == MODE_ENDLESS) {
                    GAME_START = TRUE;
                    bird.g = G;
                    pipe_green.speed = SPEED_PIPE;
                }
            }
            else if (GAME_END) {                                        // Restart the game
                // Click buttons on Game Over screen
                auto hit = [&](RectBtn b) { return msg.x > b.x && msg.x < b.x + b.w && msg.y > b.y && msg.y < b.y + b.h; };
                if (hit(btn_restart)) {
                    gameInitValue();
                    mciSendString("seek bgm to start", 0, 0, 0);
                    mciSendString("play bgm repeat", 0, 0, 0);
                    LEVEL_SELECT = (GAME_MODE == MODE_LEVEL);
                    LEVEL_COMPLETE = FALSE;
                }
                else if (hit(btn_go_menu)) {
                    GAME_MODE = MODE_NONE;
                    gameInitValue();
                }
                else if (hit(btn_view_achv)) {
                    achievements_open = !achievements_open; // simple toggle placeholder
                }
            }
            else {
                bird.speed = SPEED_UP;
                mciSendString("seek jump to start", 0, 0, 0);           // Reset jump sound to begin
                mciSendString("play jump", 0, 0, 0);                    // Play jump sound
            }
        }
    }

    // Settings panel and gear click handling when in endless mode
    if (GAME_MODE == MODE_ENDLESS && !GAME_END) {
        if (msg.uMsg == WM_LBUTTONDOWN) {
            // Toggle panel
            if (msg.x > btn_settings.x && msg.x < btn_settings.x + btn_settings.w &&
                msg.y > btn_settings.y && msg.y < btn_settings.y + btn_settings.h) {
                settings_open = !settings_open;
            }
            if (settings_open) {
                auto hit = [&](RectBtn b) { return msg.x > b.x && msg.x < b.x + b.w && msg.y > b.y && msg.y < b.y + b.h; };
                if (hit(btn_minus_horz)) { if (config_bird_horz_speed > 1) config_bird_horz_speed--; applyConfig(); }
                if (hit(btn_plus_horz)) { if (config_bird_horz_speed < 10) config_bird_horz_speed++; applyConfig(); }
                if (hit(btn_minus_grav)) { if (config_gravity > 0) config_gravity--; applyConfig(); }
                if (hit(btn_plus_grav)) { if (config_gravity < 5) config_gravity++; applyConfig(); }
                if (hit(btn_minus_jump)) { if (config_jump_strength > 3) config_jump_strength--; applyConfig(); }
                if (hit(btn_plus_jump)) { if (config_jump_strength < 16) config_jump_strength++; applyConfig(); }
                if (hit(btn_minus_thick)) { if (config_pipe_thickness > 30) { config_pipe_thickness -= 5; applyConfig(); } }
                if (hit(btn_plus_thick)) { if (config_pipe_thickness < 200) { config_pipe_thickness += 5; applyConfig(); } }
                if (hit(btn_toggle_bg)) { background_theme = 1 - background_theme; }
                if (hit(btn_toggle_music)) { music_index = 1 - music_index; applyMusic(); }
            }
        }
    }



    // Automatically update during each frame
    time2 = GetTickCount();
    while ((int)time2 - time1 > 1000 / FPS) {

        // Bird speed & location
        // Bird will always fall, until reach the ground
        if (bird.y + bird.size_y < ground.y) {
            bird.y += bird.speed;
            bird.speed += bird.g;
        }
        else {
            GAME_END = TRUE;
        }

        if (GAME_END) {
            score.y = HEIGHT * 0.6;                                 // Put the socre in the middle of the screen
            time1 = time2;
            break;
        }

        // Bird frame
        if (++bird.frame >= 3) {
            bird.frame = 0;
        }


        // Bird rotation
        float angle = 0;
        if (bird.speed != 0) {
            angle = PI / 3 * max(-1, (float)bird.speed / SPEED_UP);
        }
        else if (bird.speed < 0) {
            angle = PI / 3.5 * ((float)bird.speed / SPEED_UP);
        }
        for (int i = 0; i < bird.num_image; i++) {
            rotateimage(&bird.image_rotated[i][0], &bird.image[i][0], angle);
            rotateimage(&bird.image_rotated[i][1], &bird.image[i][1], angle, WHITE);            // Rotate and set background color as white
        }


        // Bird collision with pipe
        for (int i = 0; i < 2; i++) {
            if (bird.x + bird.image[0][0].getwidth() * 0.1 <= pipe_green.x[i] + pipe_green.size_x[i] &&
                bird.x + bird.size_x >= pipe_green.x[i] &&
                (bird.y + bird.image[0][0].getheight() * 0.1 <= pipe_green.y[i] + pipe_green.offset[0] + pipe_green.size_y[0] ||
                    bird.y + bird.size_y >= pipe_green.y[i] + pipe_green.offset[1]
                    )
                ) {

#if 0
                std::cout << bird.y << ' ' << pipe_green.y[0] << ' ' << pipe_green.size_y[0] << std::endl;
                std::cout << "collision" << std::endl;
#endif
                if (!(GAME_MODE == MODE_LEVEL && LEVEL_COMPLETE)) {
                    bird.speed = SPEED_UP / 2;              // Add a little jump as ending scene
                    GAME_END = TRUE;
                }
            }
        }

        // Enemy bird spawn and collision (from level 4)
        if (GAME_MODE == MODE_LEVEL && CURRENT_LEVEL >= 4 && !GAME_END && !GAME_PAUSED && resume_countdown_frames == 0) {
            if (!enemy_bird.active && rand() % 200 == 0) {
                enemy_bird.active = true;
                enemy_bird.x = WIDTH;
                enemy_bird.y = 80 + rand() % 240;
                enemy_bird.speed = 5 + rand() % 3;
            }
            if (enemy_bird.active) {
                enemy_bird.x -= enemy_bird.speed;
                // collision AABB
                if (bird.x < enemy_bird.x + enemy_bird.w && bird.x + bird.size_x > enemy_bird.x &&
                    bird.y < enemy_bird.y + enemy_bird.h && bird.y + bird.size_y > enemy_bird.y) {
                    GAME_END = TRUE;
                }
                if (enemy_bird.x + enemy_bird.w < 0) enemy_bird.active = false;
            }
        }

        // Coins spawn and pickup (in level mode only)
        if (GAME_MODE == MODE_LEVEL && !GAME_END && !GAME_PAUSED && resume_countdown_frames == 0) {
            if ((int)coins.size() < 3 && rand() % 120 == 0) {
                Coin c; c.x = WIDTH + 20; c.y = 80 + rand() % 260; c.r = 6; c.frame = 0; c.taken = false; coins.push_back(c);
            }
            for (auto& co : coins) {
                if (co.taken) continue;
                co.x -= SPEED_PIPE;
                // pickup check
                if (bird.x < co.x + co.r && bird.x + bird.size_x > co.x - co.r &&
                    bird.y < co.y + co.r && bird.y + bird.size_y > co.y - co.r) {
                    co.taken = true; coins_collected++;
                }
            }
            // remove off-screen/taken coins to keep vector small
            coins.erase(std::remove_if(coins.begin(), coins.end(), [&](const Coin& c) { return c.taken || c.x + c.r < 0; }), coins.end());
        }


        // Update ground
        if (ground.x < -20) {                                       // Reset ground
            ground.x = 0;
        }
        else {
            ground.x -= ground.speed;
        }


        // Pipe
        if (!GAME_PAUSED && resume_countdown_frames == 0) {
            for (int i = 0; i < 2; i++) {
                pipe_green.x[i] -= pipe_green.speed;
                // last phase: allow slight vertical oscillation on the remaining pipes
                if (GAME_MODE == MODE_LEVEL && level_last_phase) {
                    pipe_osc_offset[i] += pipe_osc_dir[i];
                    if (pipe_osc_offset[i] > 20 || pipe_osc_offset[i] < -20) pipe_osc_dir[i] = -pipe_osc_dir[i];
                    pipe_green.y[i] = pipe_base_y[i] + pipe_osc_offset[i];
                }
                if (pipe_green.x[i] < -52) {
                    int j = i;
                    if (++j > 1) {
                        j = 0;
                    }
                    if (!level_stop_spawn || GAME_MODE != MODE_LEVEL) {
                        pipe_green.x[i] = pipe_green.x[j] + 190;
                        pipe_green.y[i] = rand() % 250;                     // Reset pipe location
                        // reset oscillation base for new pipe
                        pipe_base_y[i] = pipe_green.y[i];
                        pipe_osc_offset[i] = 0; pipe_osc_dir[i] = 1;
                    }
                    score.point += 1;                                   // Update score
                    if (GAME_MODE == MODE_LEVEL && !LEVEL_COMPLETE) {
                        if (score.point >= level_target) {
                            LEVEL_COMPLETE = true;
                            level_stop_spawn = true;                     // stop generating new
                            GAME_END = TRUE; // Treat as win end
                            level_last_phase = true;                     // enable vertical oscillation
                        }
                    }
                }
            }
        }

        // Level progression for level mode
        if (GAME_MODE == MODE_LEVEL) {
            int newLevel = min(MAX_LEVEL, 1 + score.point / LEVEL_THRESHOLD);
            if (newLevel != CURRENT_LEVEL) {
                CURRENT_LEVEL = newLevel;
                setPipeThicknessPercent(calcLevelPipeThicknessPercent(CURRENT_LEVEL));
            }
        }
        time1 = time2;
    }

};

int main()
{
    gameInitResource();
    gameInitValue();

    while (1) {
        gameDraw();
        gameUpdate();
    }

    closegraph();
    return EXIT_SUCCESS;
}