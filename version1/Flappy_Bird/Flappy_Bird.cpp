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
#include <Windows.h>
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "kernel32.lib")

// 常量与游戏状态
#define PI 3.14159265359
bool GAME_START = FALSE;        // 游戏是否开始
bool GAME_END = FALSE;          // 游戏是否结束

// 游戏模式枚举
enum GameMode {
    MODE_NONE = 0,       // 未选择模式
    MODE_ENDLESS = 1,    // 无尽模式
    MODE_LEVEL = 2       // 关卡模式
};
int GAME_MODE = MODE_NONE;
int CURRENT_LEVEL = 1;          // 当前关卡
const int MAX_LEVEL = 10;       // 最大关卡数
int LEVEL_THRESHOLD = 5;        // 每关分数阈值

// UI按钮结构
struct RectBtn { int x; int y; int w; int h; };
RectBtn btn_mode_endless;        // 无尽模式按钮
RectBtn btn_mode_level;          // 关卡模式按钮
RectBtn btn_settings;            // 设置按钮
bool settings_open = false;      // 设置面板是否打开

// 资源与窗口尺寸
IMAGE btn_settings_img;          // 设置按钮图片
int WIDTH = 0;                   // 窗口宽度
int HEIGHT = 0;                  // 窗口高度

// 背景与主题
IMAGE background_day;            // 白天背景
IMAGE background_night;          // 夜晚背景
int background_theme = 0;        // 0-白天,1-夜晚
bool GAME_PAUSED = false;        // 游戏暂停状态
int resume_countdown_frames = 0; // 继续倒计时帧数
int LEVEL_STARTED = 1;           // 已开始的关卡

// 地面结构
struct Ground {
    int x = 0;
    int y = 420;
    int speed = 3;
    IMAGE image;
} ground;

// 时间与帧率
unsigned long time1, time2;      // 时间变量（兼容GetTickCount）
int FPS = 60;                    // 帧率

// 鸟结构
typedef struct Bird {
    int x = 30;
    int y = 200;
    int size_x = 0;
    int size_y = 0;
    int speed = 0;
    int frame = 0;
    int g = 0;
    int num_image = 3;
    IMAGE image[3][2];           // 原始图片
    IMAGE image_rotated[3][2];   // 旋转后图片
};
int SPEED_UP = -8;               // 跳跃初速度
int G = 1;                       // 重力加速度
Bird bird;

// 无尽模式配置
int config_pipe_thickness = 100; // 管道粗细百分比
int config_bird_horz_speed = 3;  // 水平速度
int config_gravity = 1;          // 重力配置
int config_jump_strength = 8;    // 跳跃力度
int music_index = 1;             // 音乐索引

// 设置面板按钮
RectBtn btn_minus_horz, btn_plus_horz;
RectBtn btn_minus_grav, btn_plus_grav;
RectBtn btn_minus_jump, btn_plus_jump;
RectBtn btn_minus_thick, btn_plus_thick;
RectBtn btn_toggle_bg, btn_toggle_music;

// 关卡选择相关
bool LEVEL_SELECT = false;               // 是否显示关卡选择
RectBtn level_btns[10];                  // 关卡按钮
int LEVEL_TARGETS[10] = { 5,10,13,16,19,22,25,28,31,34 }; // 每关目标分数
int level_target = 0;                    // 当前关卡目标
bool LEVEL_COMPLETE = false;             // 关卡是否完成
bool level_unlocked[10] = { true, false, false, false, false, false, false, false, false, false }; // 关卡解锁状态（默认第1关解锁）
bool level_stop_spawn = false;           // 达到目标后停止生成管道
RectBtn btn_back;                        // 返回按钮
bool level_last_phase = false;           // 目标达成后阶段
int pipe_base_y[2] = { 0,0 };              // 管道基准Y坐标
int pipe_osc_offset[2] = { 0,0 };          // 管道摆动偏移
int pipe_osc_dir[2] = { 1,1 };             // 管道摆动方向

// 管道粗细控制
int pipe_thickness_percent = 100;

// 管道结构
typedef struct Pipe {
    int x[2] = { 0, 0 };              // 两个管道X坐标
    int y[2] = { 0, 0 };              // 两个管道Y坐标
    int size_x[2] = { 0, 0 };         // 宽度
    int size_y[2] = { 0, 0 };         // 高度
    int offset[2] = { 0, 0 };         // 上下管道间距
    int speed = 0;                  // 移动速度
    IMAGE image[2];                 // 管道图片
    IMAGE mask[2];                  // 管道遮罩
    int base_size_x[2] = { 0, 0 };    // 基准宽度
    int base_size_y[2] = { 0, 0 };    // 基准高度
};
int SPEED_PIPE = 3;                 // 管道速度
Pipe pipe_green;                    // 绿色管道

// 设置管道粗细百分比
void setPipeThicknessPercent(int percent) {
    if (percent < 30) percent = 30;
    if (percent > 200) percent = 200;
    pipe_thickness_percent = percent;
    pipe_green.size_x[0] = pipe_green.base_size_x[0];
    pipe_green.size_y[0] = pipe_green.base_size_y[0];
    pipe_green.size_x[1] = pipe_green.base_size_x[1];
    pipe_green.size_y[1] = pipe_green.base_size_y[1];
}

// 计算关卡管道粗细
int calcLevelPipeThicknessPercent(int level) {
    if (level < 1) level = 1;
    if (level > MAX_LEVEL) level = MAX_LEVEL;
    return 60 + (level - 1) * 4; // 每关增加4%
}

// 应用音乐设置
void applyMusic() {
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

// 应用配置到游戏
void applyConfig() {
    setPipeThicknessPercent(config_pipe_thickness);
    G = config_gravity;
    SPEED_UP = -config_jump_strength;
    SPEED_PIPE = config_bird_horz_speed;
    ground.speed = config_bird_horz_speed;
}

// 分数结构
struct Score {
    int point = 0;              // 当前分数
    int y = 5;                  // 显示Y坐标
    IMAGE image[10];            // 数字图片
    IMAGE mask[10];             // 数字遮罩
} score;

// 文本结构
typedef struct Text {
    int x = 0;                  // X坐标
    int y = 0;                  // Y坐标
    IMAGE image = NULL;         // 文本图片
    IMAGE mask = NULL;          // 文本遮罩
};

// 文本与按钮图片资源
Text game_over;                // 游戏结束文本
Text tutorial;                 // 教程文本
Text title;                    // 标题文本
Text button_play;              // 开始按钮
IMAGE btn_endless_img;         // 无尽模式按钮图片
IMAGE btn_level_img;           // 关卡模式按钮图片
IMAGE level_btn_images[10];    // 关卡按钮图片

// 进度与成就
int high_score = 0;            // 最高分
int total_exp = 0;             // 总经验
bool achievements_open = false;// 成就面板是否打开
bool game_over_processed = false; // 游戏结束处理标记

// 动态按钮（游戏结束界面）
RectBtn btn_btn1;              // 按钮1（返回）
RectBtn btn_btn2;              // 按钮2（下一关/重玩）
RectBtn btn_btn3;              // 按钮3（主菜单）

// 敌人鸟
struct EnemyBird {
    int x; int y; int w; int h; int speed;
    bool active;
};
EnemyBird enemy_bird = { 0, 0, 32, 24, 5, false };
IMAGE enemy_img; IMAGE enemy_mask;

// 金币
struct Coin { int x; int y; int r; int frame; bool taken; };
std::vector<Coin> coins;       // 金币列表
int coins_collected = 0;       // 收集的金币数
IMAGE coin_img;                // 金币图片

// 加载进度
void loadProgress() {
    std::ifstream fin("save.dat", std::ios::in | std::ios::binary);
    if (!fin.good()) return;
    fin.read((char*)&high_score, sizeof(high_score));
    fin.read((char*)&total_exp, sizeof(total_exp));
    fin.read((char*)level_unlocked, sizeof(level_unlocked)); // 加载关卡解锁状态
    fin.close();
}

// 保存进度
void saveProgress() {
    std::ofstream fout("save.dat", std::ios::out | std::ios::binary | std::ios::trunc);
    if (!fout.good()) return;
    fout.write((char*)&high_score, sizeof(high_score));
    fout.write((char*)&total_exp, sizeof(total_exp));
    fout.write((char*)level_unlocked, sizeof(level_unlocked)); // 保存关卡解锁状态
    fout.close();
}

// 游戏结束处理
void onGameOver() {
    if (score.point > high_score) high_score = score.point;
    total_exp += score.point;
    // 通关后解锁下一关
    if (GAME_MODE == MODE_LEVEL && LEVEL_COMPLETE && CURRENT_LEVEL < MAX_LEVEL) {
        // 修复：解锁下一关，而不是当前关
        level_unlocked[CURRENT_LEVEL] = true; // CURRENT_LEVEL 是当前关卡，解锁下一关
    }
    saveProgress();
}

// 初始化游戏资源
void gameInitResource() {
    // 加载音乐
    mciSendString("open sound\\bgm2.wav alias bgm TYPE MPEGVideo ", 0, 0, 0);
    mciSendString("play bgm repeat", 0, 0, 0);
    mciSendString("open sound\\jump.mp3 alias jump", 0, 0, 0);

    // 加载背景（用于获取窗口尺寸）
    IMAGE background;
    loadimage(&background, "img\\background.png");
    WIDTH = background.getwidth();  // 全局宽度赋值
    HEIGHT = background.getheight(); // 全局高度赋值

    // 初始化窗口
    initgraph(WIDTH, HEIGHT, 1);

    // 加载昼夜背景
    loadimage(&background_day, "img\\all_img\\bg_day.png", WIDTH, HEIGHT);
    loadimage(&background_night, "img\\all_img\\bg_night.png", WIDTH, HEIGHT);

    // 加载地面图片
    loadimage(&ground.image, "img/ground.png");

    // 加载鸟图片（3帧，含遮罩）
    for (int i = 0; i < 3; i++) {
        char address[30];
        sprintf_s(address, "img\\bird\\bird_1_%d.png", i);
        loadimage(&bird.image[i][0], address);
        loadimage(&bird.image_rotated[i][0], address);
        sprintf_s(address, "img\\bird\\bird_1_%d_mask.png", i);
        loadimage(&bird.image[i][1], address);
        loadimage(&bird.image_rotated[i][1], address);
    }

    // 加载管道图片（上下管道，含遮罩）
    loadimage(&pipe_green.image[0], "img/pipe_green_top.png");
    loadimage(&pipe_green.mask[0], "img/pipe_green_top_mask.png");
    loadimage(&pipe_green.image[1], "img/pipe_green_down.png");
    loadimage(&pipe_green.mask[1], "img/pipe_green_down_mask.png");

    // 加载敌人鸟和金币图片
    loadimage(&enemy_img, "img/all_img/bird_oriange.png");
    loadimage(&enemy_mask, "img/all_img/black.png");
    loadimage(&coin_img, "img/all_img/medals.png");

    // 加载设置按钮图片
    loadimage(&btn_settings_img, "img\\设置.png", 32, 32);

    // 记录管道基准尺寸
    pipe_green.base_size_x[0] = pipe_green.image[0].getwidth();
    pipe_green.base_size_y[0] = pipe_green.image[0].getheight();
    pipe_green.base_size_x[1] = pipe_green.image[1].getwidth();
    pipe_green.base_size_y[1] = pipe_green.image[1].getheight();

    // 加载分数数字图片
    for (int i = 0; i < 10; i++) {
        char address[30];
        sprintf_s(address, "img\\score\\%d.png", i);
        loadimage(&score.image[i], address);
        sprintf_s(address, "img\\score\\%d_mask.png", i);
        loadimage(&score.mask[i], address);
    }

    // 加载文本图片
    loadimage(&game_over.image, "img\\text\\text_game_over.png");
    loadimage(&game_over.mask, "img\\text\\text_game_over_mask.png");
    loadimage(&tutorial.image, "img\\text\\tutorial.png");
    loadimage(&tutorial.mask, "img\\text\\tutorial_mask.png");
    loadimage(&button_play.image, "img\\text\\button_play.png");
    loadimage(&button_play.mask, "img\\text\\button_play_mask.png");
    loadimage(&title.image, "img\\text\\title.png");
    loadimage(&title.mask, "img\\text\\title_mask.png");

    // 加载模式按钮图片
    loadimage(&btn_endless_img, "img\\无尽模式.jpg", 140, 42);
    loadimage(&btn_level_img, "img\\关卡模式.jpg", 140, 42);

    // 加载关卡按钮图片
    for (int i = 0; i < 10; i++) {
        char address[30];
        if (i < 2) {
            sprintf_s(address, "img\\%d.png", i + 1);
        }
        else {
            sprintf_s(address, "img\\%d.jpg", i + 1);
        }
        loadimage(&level_btn_images[i], address, 56, 40);
    }

    // 加载存档
    loadProgress();
};

// 初始化游戏数值
void gameInitValue() {
    srand(time(0));

    GAME_START = FALSE;
    GAME_END = FALSE;

    // 重置分数
    score.point = 0;
    score.y = 5;

    // 初始化管道
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

    // 初始化关卡管道粗细
    if (GAME_MODE == MODE_LEVEL) {
        setPipeThicknessPercent(calcLevelPipeThicknessPercent(CURRENT_LEVEL));
    }
    else {
        setPipeThicknessPercent(pipe_thickness_percent);
    }

    // 初始化鸟
    bird.x = 30;
    bird.y = 200;
    bird.g = 0;
    bird.speed = 0;
    bird.frame = 0;
    bird.size_x = bird.image[0][0].getwidth() * 0.8;
    bird.size_y = bird.image[0][0].getheight() * 0.8;

    // 初始化时间
    time1 = GetTickCount();
    time2 = GetTickCount();

    // 初始化文本位置
    button_play.x = (WIDTH - button_play.image.getwidth()) / 2;
    button_play.y = HEIGHT * 0.8;
    tutorial.x = (WIDTH - tutorial.image.getwidth()) / 2;
    tutorial.y = HEIGHT * 0.4;
    title.x = (WIDTH - title.image.getwidth()) / 2;
    title.y = HEIGHT * 0.1;
    game_over.x = (WIDTH - game_over.image.getwidth()) / 2;
    game_over.y = HEIGHT * 0.1;

    // 初始化模式选择按钮位置
    int btnW = 140, btnH = 42, gap = 20;
    btn_mode_endless.w = btnW; btn_mode_endless.h = btnH;
    btn_mode_level.w = btnW; btn_mode_level.h = btnH;
    btn_mode_endless.x = WIDTH / 2 - btnW - gap / 2;
    btn_mode_level.x = WIDTH / 2 + gap / 2;
    btn_mode_endless.y = HEIGHT * 0.65;
    btn_mode_level.y = HEIGHT * 0.65;

    // 初始化设置按钮位置
    btn_settings.w = 32; btn_settings.h = 32;
    btn_settings.x = WIDTH - btn_settings.w - 10;
    btn_settings.y = 10;

    // 初始化关卡按钮网格
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

    // 初始化动态按钮位置
    int bw = 120, bh = 42; int spacing = 12;
    int totalW = bw * 3 + spacing * 2;
    int baseX = (WIDTH - totalW) / 2;
    int baseY = HEIGHT * 0.8;
    btn_btn1.w = bw; btn_btn1.h = bh; btn_btn1.x = baseX; btn_btn1.y = baseY;
    btn_btn2.w = bw; btn_btn2.h = bh; btn_btn2.x = baseX + bw + spacing; btn_btn2.y = baseY;
    btn_btn3.w = bw; btn_btn3.h = bh; btn_btn3.x = baseX + (bw + spacing) * 2; btn_btn3.y = baseY;

    // 重置游戏状态
    game_over_processed = false;
    GAME_PAUSED = false;
    resume_countdown_frames = 0;
    level_stop_spawn = false;
    btn_back.w = 72; btn_back.h = 28; btn_back.x = 10; btn_back.y = 10;

    // 重置敌人和金币
    enemy_bird.active = false;
    coins.clear(); coins_collected = 0;
}

// 绘制游戏界面
void gameDraw() {
    BeginBatchDraw();
    setbkmode(TRANSPARENT); // 透明背景

    // 绘制背景（根据主题切换）
    if (background_theme == 0) putimage(0, 0, &background_day);
    else putimage(0, 0, &background_night);

    // 绘制管道（游戏开始时）
    if (GAME_START) {
        for (int i = 0; i < 2; i++) {
            putimage(pipe_green.x[i], pipe_green.y[i] + pipe_green.offset[0], &pipe_green.mask[0], SRCAND);
            putimage(pipe_green.x[i], pipe_green.y[i] + pipe_green.offset[0], &pipe_green.image[0], SRCPAINT);
            putimage(pipe_green.x[i], pipe_green.y[i] + pipe_green.offset[1], &pipe_green.mask[1], SRCAND);
            putimage(pipe_green.x[i], pipe_green.y[i] + pipe_green.offset[1], &pipe_green.image[1], SRCPAINT);
        }
    }

    // 绘制地面
    putimage(ground.x, ground.y, &ground.image);

    // 绘制鸟（带旋转效果）
    putimage(bird.x, bird.y, &bird.image_rotated[bird.frame][1], SRCAND);
    putimage(bird.x, bird.y, &bird.image_rotated[bird.frame][0], SRCPAINT);

    // 绘制分数（游戏开始时）
    if (GAME_START) {
        std::string credit = std::to_string(score.point);
        for (int i = 0; i < credit.size(); i++) {
            // 修正：用全局WIDTH替换局部background变量
            putimage(WIDTH / 2 - ((int)credit.size() / 2 - i + 0.5) * score.mask[0].getwidth(),
                score.y, &score.mask[credit[i] - 48], SRCAND);
            putimage(WIDTH / 2 - ((int)credit.size() / 2 - i + 0.5) * score.image[0].getwidth(),
                score.y, &score.image[credit[i] - 48], SRCPAINT);
        }
    }

    // 绘制初始界面（未开始时）
    if (!GAME_START) {
        putimage(tutorial.x, tutorial.y, &tutorial.mask, SRCAND);
        putimage(tutorial.x, tutorial.y, &tutorial.image, SRCPAINT);
        putimage(title.x, title.y, &title.mask, SRCAND);
        putimage(title.x, title.y, &title.image, SRCPAINT);

        // 绘制模式选择按钮（未选择模式时）
        if (GAME_MODE == MODE_NONE) {
            putimage(btn_mode_endless.x, btn_mode_endless.y, &btn_endless_img);
            putimage(btn_mode_level.x, btn_mode_level.y, &btn_level_img);
        }

        // 绘制关卡选择界面（关卡模式且选择关卡时）
        if (GAME_MODE == MODE_LEVEL && LEVEL_SELECT) {
            // 设置加粗字体
            settextstyle(16, 0, "微软雅黑", 0, 0, 400, false, false, false);

            // 绘制返回按钮
            const char* text = "返回主界面";
            RECT r = { 0, 0, 0, 0 };
            drawtext(text, &r, DT_CALCRECT);
            int textWidth = r.right - r.left;
            int padding = 8;
            btn_back.w = textWidth + padding * 2;
            btn_back.h = 28;
            setfillcolor(RGB(200, 200, 200));
            solidrectangle(btn_back.x, btn_back.y, btn_back.x + btn_back.w, btn_back.y + btn_back.h);
            settextcolor(RGB(0, 0, 0));
            outtextxy(btn_back.x + padding, btn_back.y + 5, text);
            settextstyle(16, 0, "宋体");

            // 绘制关卡按钮（带锁标记）
            for (int i = 0; i < 10; i++) {
                putimage(level_btns[i].x, level_btns[i].y, &level_btn_images[i]);

                // 未解锁关卡：右侧1/3区域显示灰色遮罩+"锁"字
                if (!level_unlocked[i]) {
                    int maskW = level_btns[i].w / 3;
                    int maskX = level_btns[i].x + level_btns[i].w - maskW;
                    setfillcolor(RGB(180, 180, 180));
                    solidrectangle(maskX, level_btns[i].y, maskX + maskW, level_btns[i].y + level_btns[i].h);

                    // 绘制"锁"字
                    settextstyle(16, 0, "微软雅黑");
                    settextcolor(RGB(0, 0, 0));
                    int lockX = maskX + maskW / 2 - 8;
                    int lockY = level_btns[i].y + level_btns[i].h / 2 - 8;
                    outtextxy(lockX, lockY, "锁");
                    settextstyle(16, 0, "宋体");
                }
            }
        }
    }

    // 绘制设置按钮（无尽模式且未结束时）
    if (GAME_MODE == MODE_ENDLESS && !GAME_END) {
        putimage(btn_settings.x, btn_settings.y, &btn_settings_img);
    }

    // 绘制设置面板（打开时）
    if (settings_open) {
        int px = WIDTH - 240, py = 50, pw = 230, ph = 200;
        setfillcolor(RGB(255, 255, 255));
        solidrectangle(px, py, px + pw, py + ph);
        settextcolor(RGB(0, 0, 0));
        setfillcolor(RGB(240, 240, 255));
        solidrectangle(px + 6, py + 6, px + pw - 6, py + 28);
        settextcolor(RGB(60, 60, 120));
        outtextxy(px + 12, py + 8, "无尽设置");

        // 绘制设置项（水平速度、重力等）
        int ly = py + 32;
        auto drawRow = [&](const char* label, int value, RectBtn& minusBtn, RectBtn& plusBtn) {
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

        // 绘制主题和音乐切换按钮
        btn_toggle_bg.x = px + 10; btn_toggle_bg.y = ly; btn_toggle_bg.w = 90; btn_toggle_bg.h = 22;
        setfillcolor(RGB(230, 240, 255));
        solidrectangle(btn_toggle_bg.x, btn_toggle_bg.y, btn_toggle_bg.x + btn_toggle_bg.w, btn_toggle_bg.y + btn_toggle_bg.h);
        outtextxy(btn_toggle_bg.x + 10, btn_toggle_bg.y + 2, background_theme == 0 ? "白天" : "夜晚");
        btn_toggle_music.x = px + 110; btn_toggle_music.y = ly; btn_toggle_music.w = 90; btn_toggle_music.h = 22;
        setfillcolor(RGB(230, 255, 235));
        solidrectangle(btn_toggle_music.x, btn_toggle_music.y, btn_toggle_music.x + btn_toggle_music.w, btn_toggle_music.y + btn_toggle_music.h);
        outtextxy(btn_toggle_music.x + 10, btn_toggle_music.y + 2, music_index == 0 ? "音乐1" : "音乐2");
    }

    // 绘制倒计时（如果有）
    if (resume_countdown_frames > 0) {
        int seconds = (resume_countdown_frames + FPS - 1) / FPS;
        settextstyle(48, 0, "微软雅黑");
        settextcolor(RGB(255, 255, 255));
        char c[4]; sprintf_s(c, "%d", seconds);
        outtextxy(WIDTH / 2 - 12, HEIGHT / 2 - 24, c);
        settextstyle(16, 0, "宋体");
    }

    // 绘制敌人鸟（激活时）
    if (enemy_bird.active && GAME_START && !GAME_END) {
        putimage(enemy_bird.x, enemy_bird.y, &enemy_mask, SRCAND);
        putimage(enemy_bird.x, enemy_bird.y, &enemy_img, SRCPAINT);
    }

    // 绘制金币（游戏开始且未结束时）
    if (GAME_START && !GAME_END) {
        for (auto& co : coins) {
            if (co.taken) continue;
            int rr = co.r + (co.frame % 10 < 5 ? 1 : -1); // 呼吸效果
            setfillcolor(RGB(255, 215, 0));
            solidcircle(co.x, co.y, rr);
            co.frame = (co.frame + 1) % 10;
        }
        settextcolor(RGB(255, 255, 0));
        char cbuf[32]; sprintf_s(cbuf, "金币:%d", coins_collected);
        outtextxy(10, 40, cbuf);
    }

    // 绘制游戏结束界面（游戏结束时）
    if (GAME_END) {
        if (!game_over_processed) {
            onGameOver();
            game_over_processed = true;
        }

        // 绘制游戏结束标题
        putimage(game_over.x, game_over.y, &game_over.mask, SRCAND);
        putimage(game_over.x, game_over.y, &game_over.image, SRCPAINT);

        // 绘制分数和星星
        settextcolor(RGB(255, 255, 0));
        char thisScore[32]; sprintf_s(thisScore, "本次分数: %d", score.point);
        outtextxy(WIDTH / 2 - 80, HEIGHT * 0.32, thisScore);
        int star = (score.point >= 30) ? 3 : (score.point >= 20 ? 2 : (score.point >= 10 ? 1 : 0));
        for (int i = 0; i < star; i++) {
            setfillcolor(RGB(255, 215, 0));
            solidcircle(WIDTH / 2 - 60 + i * 60, HEIGHT * 0.42, 12);
        }

        // 绘制最高分和金币
        settextcolor(RGB(230, 240, 255));
        char best[32]; sprintf_s(best, "历史最高分: %d", high_score);
        outtextxy(WIDTH / 2 - 90, HEIGHT * 0.50, best);
        char coinbuf[32]; sprintf_s(coinbuf, "本关金币: %d", coins_collected);
        settextcolor(RGB(255, 230, 120)); outtextxy(WIDTH / 2 - 80, HEIGHT * 0.46, coinbuf);

        // 绘制等级进度
        int level = total_exp / 50 + 1;
        int expInLevel = total_exp % 50;
        char lvbuf[32]; sprintf_s(lvbuf, "当前等级: Lv.%d", level);
        outtextxy(WIDTH / 2 - 90, HEIGHT * 0.56, lvbuf);
        int bx = WIDTH / 2 - 100, by = HEIGHT * 0.60, bw = 200, bh = 14;
        setlinecolor(RGB(0, 0, 0)); rectangle(bx, by, bx + bw, by + bh);
        setfillcolor(RGB(80, 220, 120));
        int fillw = bw * expInLevel / 50; solidrectangle(bx + 1, by + 1, bx + fillw, by + bh - 1);
        char xp[32]; sprintf_s(xp, "%d/50经验", expInLevel);
        settextcolor(RGB(0, 0, 0)); outtextxy(WIDTH / 2 - 30, by - 18, xp);

        // 绘制关卡模式按钮（通关/未通关状态）
        if (GAME_MODE == MODE_LEVEL) {
            if (LEVEL_COMPLETE) {
                // 通关状态：返回 → 下一关 → 主菜单
                setfillcolor(RGB(180, 120, 80));
                solidrectangle(btn_btn1.x, btn_btn1.y, btn_btn1.x + btn_btn1.w, btn_btn1.y + btn_btn1.h);
                settextcolor(RGB(255, 255, 255)); outtextxy(btn_btn1.x + 45, btn_btn1.y + 12, "返回");

                setfillcolor(CURRENT_LEVEL >= MAX_LEVEL ? RGB(150, 150, 150) : RGB(80, 180, 120));
                solidrectangle(btn_btn2.x, btn_btn2.y, btn_btn2.x + btn_btn2.w, btn_btn2.y + btn_btn2.h);
                settextcolor(RGB(255, 255, 255));
                outtextxy(btn_btn2.x + 35, btn_btn2.y + 12, CURRENT_LEVEL >= MAX_LEVEL ? "已通关" : "下一关");

                setfillcolor(RGB(150, 80, 180));
                solidrectangle(btn_btn3.x, btn_btn3.y, btn_btn3.x + btn_btn3.w, btn_btn3.y + btn_btn3.h);
                settextcolor(RGB(255, 255, 255)); outtextxy(btn_btn3.x + 35, btn_btn3.y + 12, "主菜单");
            }
            else {
                // 未通关状态：返回 → 重玩 → 主菜单
                setfillcolor(RGB(180, 120, 80));
                solidrectangle(btn_btn1.x, btn_btn1.y, btn_btn1.x + btn_btn1.w, btn_btn1.y + btn_btn1.h);
                settextcolor(RGB(255, 255, 255)); outtextxy(btn_btn1.x + 45, btn_btn1.y + 12, "返回");

                setfillcolor(RGB(80, 180, 120));
                solidrectangle(btn_btn2.x, btn_btn2.y, btn_btn2.x + btn_btn2.w, btn_btn2.y + btn_btn2.h);
                settextcolor(RGB(255, 255, 255)); outtextxy(btn_btn2.x + 45, btn_btn2.y + 12, "重玩");

                setfillcolor(RGB(150, 80, 180));
                solidrectangle(btn_btn3.x, btn_btn3.y, btn_btn3.x + btn_btn3.w, btn_btn3.y + btn_btn3.h);
                settextcolor(RGB(255, 255, 255)); outtextxy(btn_btn3.x + 35, btn_btn3.y + 12, "主菜单");
            }
        }
        else {
            // 无尽模式按钮
            setfillcolor(RGB(180, 120, 80));
            solidrectangle(btn_btn1.x, btn_btn1.y, btn_btn1.x + btn_btn1.w, btn_btn1.y + btn_btn1.h);
            settextcolor(RGB(255, 255, 255)); outtextxy(btn_btn1.x + 18, btn_btn1.y + 12, "返回主菜单");

            setfillcolor(RGB(80, 180, 120));
            solidrectangle(btn_btn2.x, btn_btn2.y, btn_btn2.x + btn_btn2.w, btn_btn2.y + btn_btn2.h);
            settextcolor(RGB(255, 255, 255)); outtextxy(btn_btn2.x + 38, btn_btn2.y + 12, "重玩");

            setfillcolor(RGB(150, 80, 180));
            solidrectangle(btn_btn3.x, btn_btn3.y, btn_btn3.x + btn_btn3.w, btn_btn3.y + btn_btn3.h);
            settextcolor(RGB(255, 255, 255)); outtextxy(btn_btn3.x + 18, btn_btn3.y + 12, "查看成就");
        }
    }

    EndBatchDraw();
};

// 更新游戏逻辑
void gameUpdate() {
    MOUSEMSG msg = { 0 };
    if (MouseHit()) {
        msg = GetMouseMsg();
        if (msg.uMsg == WM_LBUTTONDOWN) {
            if (!GAME_START) {
                // 模式选择（未选择模式时）
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
                        LEVEL_SELECT = true;
                        gameInitValue();
                    }
                }

                // 关卡选择逻辑（关卡模式且选择关卡时）
                if (GAME_MODE == MODE_LEVEL && LEVEL_SELECT) {
                    // 返回主界面
                    if (msg.x > btn_back.x && msg.x < btn_back.x + btn_back.w &&
                        msg.y > btn_back.y && msg.y < btn_back.y + btn_back.h) {
                        GAME_MODE = MODE_NONE;
                        gameInitValue();
                        return;
                    }

                    // 选择关卡（点击解锁的关卡）
                    for (int i = 0; i < 10; i++) {
                        if (level_unlocked[i] && msg.x > level_btns[i].x && msg.x < level_btns[i].x + level_btns[i].w &&
                            msg.y > level_btns[i].y && msg.y < level_btns[i].y + level_btns[i].h) {
                            CURRENT_LEVEL = i + 1;
                            LEVEL_STARTED = CURRENT_LEVEL;
                            level_target = LEVEL_TARGETS[i];
                            setPipeThicknessPercent(calcLevelPipeThicknessPercent(CURRENT_LEVEL));
                            LEVEL_SELECT = false;
                            GAME_START = TRUE;
                            bird.g = G;
                            pipe_green.speed = SPEED_PIPE;
                            pipe_base_y[0] = pipe_green.y[0];
                            pipe_base_y[1] = pipe_green.y[1];
                            pipe_osc_offset[0] = pipe_osc_offset[1] = 0;
                            pipe_osc_dir[0] = pipe_osc_dir[1] = 1;
                        }
                    }
                }
                else if (GAME_MODE == MODE_ENDLESS) {
                    // 无尽模式开始游戏
                    GAME_START = TRUE;
                    bird.g = G;
                    pipe_green.speed = SPEED_PIPE;
                }
            }
            else if (GAME_END) {
                // 按钮点击检测
                auto hit = [&](RectBtn b) {
                    return msg.x > b.x && msg.x < b.x + b.w && msg.y > b.y && msg.y < b.y + b.h;
                    };

                // 关卡模式按钮逻辑
                if (GAME_MODE == MODE_LEVEL) {
                    // 返回关卡选择
                    if (hit(btn_btn1)) {
                        GAME_END = false;
                        LEVEL_SELECT = true;
                        gameInitValue();
                    }

                    // 下一关（通关状态：点击后返回关卡页面）
                    if (LEVEL_COMPLETE && hit(btn_btn2)) {
                        if (CURRENT_LEVEL < MAX_LEVEL) {
                            GAME_END = false;
                            GAME_START = false;
                            GAME_MODE = MODE_LEVEL;
                            LEVEL_SELECT = true;
                            gameInitValue(); // 重置并显示关卡选择
                        }
                    }

                    // 重玩（未通关状态）
                    if (!LEVEL_COMPLETE && hit(btn_btn2)) {
                        GAME_END = false;
                        GAME_START = true;
                        bird.x = 30; bird.y = 200; bird.speed = 0; bird.frame = 0;
                        pipe_green.x[0] = ground.image.getwidth();
                        pipe_green.x[1] = ground.image.getwidth() + 190;
                        pipe_green.y[0] = rand() % 250;
                        pipe_green.y[1] = rand() % 250;
                        score.point = 0;
                        coins.clear(); coins_collected = 0;
                        enemy_bird.active = false;
                        level_stop_spawn = false;
                        level_last_phase = false;
                    }

                    // 主菜单
                    if (hit(btn_btn3)) {
                        GAME_MODE = MODE_NONE;
                        gameInitValue();
                    }
                }
                else {
                    // 无尽模式按钮逻辑
                    if (hit(btn_btn1)) {
                        GAME_MODE = MODE_NONE;
                        gameInitValue();
                    }
                    if (hit(btn_btn2)) {
                        gameInitValue();
                        mciSendString("seek bgm to start", 0, 0, 0);
                        mciSendString("play bgm repeat", 0, 0, 0);
                        LEVEL_SELECT = (GAME_MODE == MODE_LEVEL);
                        LEVEL_COMPLETE = FALSE;
                    }
                    if (hit(btn_btn3)) {
                        achievements_open = !achievements_open;
                    }
                }
            }
            else {
                // 鸟跳跃（游戏中点击）
                bird.speed = SPEED_UP;
                mciSendString("seek jump to start", 0, 0, 0);
                mciSendString("play jump", 0, 0, 0);
            }
        }
    }

    // 无尽模式设置面板逻辑
    if (GAME_MODE == MODE_ENDLESS && !GAME_END) {
        if (msg.uMsg == WM_LBUTTONDOWN) {
            // 切换设置面板
            if (msg.x > btn_settings.x && msg.x < btn_settings.x + btn_settings.w &&
                msg.y > btn_settings.y && msg.y < btn_settings.y + btn_settings.h) {
                settings_open = !settings_open;
                GAME_PAUSED = settings_open;
            }
            if (settings_open) {
                auto hit = [&](RectBtn b) {
                    return msg.x > b.x && msg.x < b.x + b.w && msg.y > b.y && msg.y < b.y + b.h;
                    };
                // 调整设置项
                if (hit(btn_minus_horz) && config_bird_horz_speed > 1) { config_bird_horz_speed--; applyConfig(); }
                if (hit(btn_plus_horz) && config_bird_horz_speed < 10) { config_bird_horz_speed++; applyConfig(); }
                if (hit(btn_minus_grav) && config_gravity > 0) { config_gravity--; applyConfig(); }
                if (hit(btn_plus_grav) && config_gravity < 5) { config_gravity++; applyConfig(); }
                if (hit(btn_minus_jump) && config_jump_strength > 3) { config_jump_strength--; applyConfig(); }
                if (hit(btn_plus_jump) && config_jump_strength < 16) { config_jump_strength++; applyConfig(); }
                if (hit(btn_minus_thick) && config_pipe_thickness > 30) { config_pipe_thickness -= 5; applyConfig(); }
                if (hit(btn_plus_thick) && config_pipe_thickness < 200) { config_pipe_thickness += 5; applyConfig(); }
                if (hit(btn_toggle_bg)) { background_theme = 1 - background_theme; }
                if (hit(btn_toggle_music)) { music_index = 1 - music_index; applyMusic(); }
            }
        }
    }

    // 时间更新（控制帧率）
    time2 = GetTickCount();
    while ((int)(time2 - time1) > 1000 / FPS) {
        if (GAME_PAUSED) {
            time1 = time2;
            break;
        }

        // 鸟的移动（重力效果）
        if (bird.y + bird.size_y < ground.y) {
            bird.y += bird.speed;
            bird.speed += bird.g;
        }
        else {
            GAME_END = TRUE; // 碰到地面，游戏结束
        }

        if (GAME_END) {
            score.y = HEIGHT * 0.6;
            time1 = time2;
            break;
        }

        // 鸟的动画帧更新
        if (++bird.frame >= 3) {
            bird.frame = 0;
        }

        // 鸟的旋转（根据速度）
        float angle = 0;
        if (bird.speed != 0) {
            angle = PI / 3 * max(-1, (float)bird.speed / SPEED_UP);
        }
        else if (bird.speed < 0) {
            angle = PI / 3.5 * ((float)bird.speed / SPEED_UP);
        }
        for (int i = 0; i < bird.num_image; i++) {
            rotateimage(&bird.image_rotated[i][0], &bird.image[i][0], angle);
            rotateimage(&bird.image_rotated[i][1], &bird.image[i][1], angle, WHITE);
        }

        // 碰撞检测（鸟与管道）
        for (int i = 0; i < 2; i++) {
            if (bird.x + bird.image[0][0].getwidth() * 0.1 <= pipe_green.x[i] + pipe_green.size_x[i] &&
                bird.x + bird.size_x >= pipe_green.x[i] &&
                (bird.y + bird.image[0][0].getheight() * 0.1 <= pipe_green.y[i] + pipe_green.offset[0] + pipe_green.size_y[0] ||
                    bird.y + bird.size_y >= pipe_green.y[i] + pipe_green.offset[1])) {
                if (!(GAME_MODE == MODE_LEVEL && LEVEL_COMPLETE)) {
                    bird.speed = SPEED_UP / 2;
                    GAME_END = TRUE;
                }
            }
        }

        // 敌人鸟逻辑（4关及以上）
        if (GAME_MODE == MODE_LEVEL && CURRENT_LEVEL >= 4 && !GAME_END && !GAME_PAUSED && resume_countdown_frames == 0) {
            if (!enemy_bird.active && rand() % 200 == 0) {
                enemy_bird.active = true;
                enemy_bird.x = WIDTH;
                enemy_bird.y = 80 + rand() % 240;
                enemy_bird.speed = 5 + rand() % 3;
            }
            if (enemy_bird.active) {
                enemy_bird.x -= enemy_bird.speed;
                // 碰撞检测（鸟与敌人）
                if (bird.x < enemy_bird.x + enemy_bird.w && bird.x + bird.size_x > enemy_bird.x &&
                    bird.y < enemy_bird.y + enemy_bird.h && bird.y + bird.size_y > enemy_bird.y) {
                    GAME_END = TRUE;
                }
                // 敌人飞出屏幕，取消激活
                if (enemy_bird.x + enemy_bird.w < 0) enemy_bird.active = false;
            }
        }

        // 金币逻辑（关卡模式）
        if (GAME_MODE == MODE_LEVEL && !GAME_END && !GAME_PAUSED && resume_countdown_frames == 0) {
            // 随机生成金币（最多3个）
            if ((int)coins.size() < 3 && rand() % 120 == 0) {
                Coin c;
                c.x = WIDTH + 20;
                c.y = 80 + rand() % 260;
                c.r = 6;
                c.frame = 0;
                c.taken = false;
                coins.push_back(c);
            }
            // 金币移动与碰撞检测
            for (auto& co : coins) {
                if (co.taken) continue;
                co.x -= SPEED_PIPE;
                if (bird.x < co.x + co.r && bird.x + bird.size_x > co.x - co.r &&
                    bird.y < co.y + co.r && bird.y + bird.size_y > co.y - co.r) {
                    co.taken = true;
                    coins_collected++;
                }
            }
            // 移除已收集或飞出屏幕的金币
            coins.erase(std::remove_if(coins.begin(), coins.end(),
                [&](const Coin& c) { return c.taken || c.x + c.r < 0; }), coins.end());
        }

        // 地面移动（循环滚动）
        if (ground.x < -20) {
            ground.x = 0;
        }
        else {
            ground.x -= ground.speed;
        }

        // 管道移动（未暂停且倒计时结束）
        if (!GAME_PAUSED && resume_countdown_frames == 0) {
            for (int i = 0; i < 2; i++) {
                pipe_green.x[i] -= pipe_green.speed;
                // 关卡模式最终阶段：管道摆动
                if (GAME_MODE == MODE_LEVEL && level_last_phase) {
                    pipe_osc_offset[i] += pipe_osc_dir[i];
                    if (pipe_osc_offset[i] > 20 || pipe_osc_offset[i] < -20)
                        pipe_osc_dir[i] = -pipe_osc_dir[i];
                    pipe_green.y[i] = pipe_base_y[i] + pipe_osc_offset[i];
                }
                // 管道飞出屏幕，重置位置
                if (pipe_green.x[i] < -52) {
                    int j = i;
                    if (++j > 1) j = 0;
                    if (!level_stop_spawn || GAME_MODE != MODE_LEVEL) {
                        pipe_green.x[i] = pipe_green.x[j] + 190;
                        pipe_green.y[i] = rand() % 250;
                        pipe_base_y[i] = pipe_green.y[i];
                        pipe_osc_offset[i] = 0;
                        pipe_osc_dir[i] = 1;
                    }
                    // 通过管道，加分
                    score.point += 1;
                    // 关卡模式：检测是否达成目标
                    if (GAME_MODE == MODE_LEVEL && !LEVEL_COMPLETE) {
                        if (score.point >= level_target) {
                            LEVEL_COMPLETE = true;
                            level_stop_spawn = true;
                            GAME_END = TRUE;
                            level_last_phase = true;
                        }
                    }
                }
            }
        }

        // 关卡进度更新（根据分数）
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

// 主函数
int main() {
    gameInitResource();  // 初始化资源
    gameInitValue();     // 初始化数值
    while (1) {
        gameDraw();      // 绘制界面
        gameUpdate();    // 更新逻辑
    }
    closegraph();
    return EXIT_SUCCESS;
}