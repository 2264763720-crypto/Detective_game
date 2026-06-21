/**
 * main.c - 侦探推理游戏《雨夜密室杀人案》主入口
 * 
 * 程序启动流程：
 * 1. 加载 story.json 剧情数据
 * 2. 初始化游戏状态（HP=100, Clue=0, 空背包）
 * 3. 显示开场剧情和故事背景
 * 4. 进入主游戏循环：
 *    - 渲染当前场景
 *    - 显示状态面板
 *    - 等待玩家选择
 *    - 处理选择效果
 *    - 检查是否到达结局
 * 5. 显示结局画面
 * 6. 释放内存，退出
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "types.h"
#include "utils.h"
#include "engine.h"

/* cJSON 库头文件 */
#include "../lib/cJSON.h"

/* ========== 函数声明 ========== */

/**
 * 加载并解析 story.json 剧情文件
 * @param filename  JSON 文件路径
 * @param state     游戏状态（输出参数，将填充场景/结局/嫌疑人数据）
 * @return          1=成功，0=失败
 */
static int loadStoryData(const char *filename, GameState *state);

/**
 * 从 cJSON 对象解析单个场景
 * @param json_scene  cJSON 场景对象
 * @param scene       输出场景结构体
 * @return            1=成功，0=失败
 */
static int parseScene(cJSON *json_scene, Scene *scene);

/**
 * 从 cJSON 对象解析单个选项
 * @param json_choice  cJSON 选项对象
 * @param choice       输出选项结构体
 * @return             1=成功，0=失败
 */
static int parseChoice(cJSON *json_choice, Choice *choice);

/**
 * 从 cJSON 对象解析结局条件
 * @param json_ending  cJSON 结局对象
 * @param ending       输出结局条件结构体
 * @return             1=成功，0=失败
 */
static int parseEnding(cJSON *json_ending, EndingCondition *ending);

/**
 * 从 cJSON 对象解析嫌疑人
 * @param json_suspect  cJSON 嫌疑人对象
 * @param suspect       输出嫌疑人结构体
 * @return              1=成功，0=失败
 */
static int parseSuspect(cJSON *json_suspect, Suspect *suspect);

/**
 * 读取整个文件内容到字符串
 * @param filename  文件路径
 * @return          文件内容字符串（需调用者 free），失败返回 NULL
 */
static char *readFile(const char *filename);

/**
 * 显示开场动画和故事背景
 * @param state  游戏状态
 */
static void showOpening(GameState *state);

/**
 * 获取可用选项的索引数组（过滤不可用选项）
 * @param state          游戏状态
 * @param scene          当前场景
 * @param valid_indices  输出：可用选项的索引数组
 * @return               可用选项数量
 */
static int getAvailableChoices(const GameState *state, const Scene *scene, int valid_indices[]);

/**
 * 清理游戏状态，释放动态分配的内存
 * @param state  游戏状态
 */
static void cleanupGame(GameState *state);

/* ========== 主函数 ========== */
int main(void) {
    GameState state;

    /* 初始化控制台（UTF-8 编码，解决中文乱码） */
    initConsole();

    /* 初始化随机种子 */
    srand((unsigned int)time(NULL));

    /* 初始化游戏状态 */
    memset(&state, 0, sizeof(GameState));
    state.current_scene_id = 1;
    state.hp = 100;
    state.clue = 0;
    state.item_count = 0;
    state.accused_count = 0;
    state.start_time = time(NULL);  /* 记录游戏开始时间 */

    /* 加载剧情数据 */
    printf(COLOR_CYAN "正在加载剧情数据..." COLOR_RESET);
    if (!loadStoryData("data/story.json", &state)) {
        printf(COLOR_RED "\n错误：无法加载剧情文件 data/story.json\n" COLOR_RESET);
        printf("请确保该文件存在于程序所在目录的 data 子目录下。\n");
        printf("按回车键退出...");
        getchar();
        return 1;
    }
    printf(COLOR_GREEN " 加载成功！\n" COLOR_RESET);

    /* 显示开场画面 */
    showOpening(&state);

    /* ===== 主游戏循环 ===== */
    int game_running = 1;
    while (game_running) {
        clearScreen();

        /* 获取当前场景 */
        Scene *current_scene = findScene(&state, state.current_scene_id);
        if (current_scene == NULL) {
            printf(COLOR_RED "错误：找不到场景 ID %d\n" COLOR_RESET, state.current_scene_id);
            /* 尝试跳转到默认结局 */
            const EndingCondition *fallback = findEndingByType(&state, ENDING_ESCAPE);
            if (fallback) {
                showEndingScreen(&state, fallback);
            }
            break;
        }

        /* 检查是否到达结局场景 */
        if (current_scene->is_ending) {
            /* 结局场景前：先让玩家指认嫌疑人（除非已经指认过） */
            if (state.accused_count == 0) {
                clearScreen();
                showAccusationScreen(&state);
            }

            const EndingCondition *result = checkEnding(&state);
            if (result == NULL) {
                /* 无匹配结局，使用默认 */
                result = findEndingByType(&state, ENDING_ESCAPE);
            }
            if (result != NULL) {
                showEndingScreen(&state, result);
            } else {
                printf(COLOR_RED "游戏结束。\n" COLOR_RESET);
            }
            break;
        }

        /* 渲染场景 */
        renderScene(&state);

        /* 显示状态面板 */
        showStatusPanel(&state);

        /* 获取可用选项 */
        int valid_indices[MAX_CHOICES];
        int valid_count = getAvailableChoices(&state, current_scene, valid_indices);

        if (valid_count == 0) {
            printf(COLOR_RED "\n当前没有可用的选项。游戏无法继续。\n" COLOR_RESET);
            const EndingCondition *fallback = findEndingByType(&state, ENDING_ESCAPE);
            if (fallback) {
                showEndingScreen(&state, fallback);
            }
            break;
        }

        /* 构建提示信息 */
        printf(COLOR_BOLD "\n请输入选择 (" COLOR_RESET);
        for (int i = 0; i < valid_count; i++) {
            printf(COLOR_GREEN "%d" COLOR_RESET, valid_indices[i] + 1);
            if (i < valid_count - 1) printf("/");
        }
        printf(COLOR_BOLD ")" COLOR_RESET);

        /* 新增特殊指令 */
        printf(COLOR_DIM "  [S=查看嫌疑人档案]" COLOR_RESET);
        printf("\n");

        /* 获取玩家输入 */
        char input_buffer[32];
        printf("> ");
        if (fgets(input_buffer, sizeof(input_buffer), stdin) == NULL) {
            printf(COLOR_RED "输入错误，程序退出。\n" COLOR_RESET);
            break;
        }

        /* 处理特殊指令 */
        if (input_buffer[0] == 'S' || input_buffer[0] == 's') {
            clearScreen();
            showSuspectProfiles(&state);
            pressEnterToContinue();
            continue;
        }

        /* 解析数字输入 */
        int player_choice;
        if (sscanf(input_buffer, "%d", &player_choice) != 1) {
            printf(COLOR_RED "输入无效，请输入数字。\n" COLOR_RESET);
            pressEnterToContinue();
            continue;
        }

        /* 验证选择是否有效 */
        int valid_choice = 0;
        for (int i = 0; i < valid_count; i++) {
            if (player_choice == valid_indices[i] + 1) {
                valid_choice = 1;
                break;
            }
        }

        if (!valid_choice) {
            printf(COLOR_RED "无效选择，请从可用选项中选取。\n" COLOR_RESET);
            pressEnterToContinue();
            continue;
        }

        /* 玩家选择了第 player_choice 个选项（1-based），转换为 0-based 索引 */
        int choice_idx = player_choice - 1;

        /* 处理随机事件（在应用选择效果之前） */
        handleRandomEvent(&state, choice_idx);

        /* 检查 HP 是否在随机事件中归零 */
        if (state.hp <= 0) {
            printf(COLOR_RED "\n⚠ 你的生命值耗尽！调查被迫终止...\n" COLOR_RESET);
            pressEnterToContinue();
            const EndingCondition *result = findEndingByType(&state, ENDING_ESCAPE);
            if (result) {
                showEndingScreen(&state, result);
            }
            break;
        }

        /* 应用选择效果并获取目标场景 */
        int next_scene = processChoice(&state, choice_idx);

        /* 检查 HP 是否在选择效果中归零 */
        if (next_scene == -1 || state.hp <= 0) {
            printf(COLOR_RED "\n⚠ 你的生命值耗尽！调查被迫终止...\n" COLOR_RESET);
            pressEnterToContinue();
            const EndingCondition *result = findEndingByType(&state, ENDING_ESCAPE);
            if (result) {
                showEndingScreen(&state, result);
            }
            break;
        }

        /* 跳转到目标场景 */
        state.current_scene_id = next_scene;

        pressEnterToContinue();
    }

    /* 清理 */
    cleanupGame(&state);

    printf("\n按回车键退出...");
    getchar();

    return 0;
}

/* ========== 读取文件 ========== */
static char *readFile(const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf(COLOR_RED "无法打开文件: %s\n" COLOR_RESET, filename);
        return NULL;
    }

    /* 获取文件大小 */
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    /* 分配内存（+1 给 null 终止符） */
    char *buffer = (char *)malloc((size_t)(file_size + 1));
    if (buffer == NULL) {
        printf(COLOR_RED "内存分配失败。\n" COLOR_RESET);
        fclose(fp);
        return NULL;
    }

    /* 读取文件内容 */
    size_t bytes_read = fread(buffer, 1, (size_t)file_size, fp);
    buffer[bytes_read] = '\0';

    fclose(fp);
    return buffer;
}

/* ========== 加载剧情数据 ========== */
static int loadStoryData(const char *filename, GameState *state) {
    char *json_str = readFile(filename);
    if (json_str == NULL) {
        return 0;
    }

    cJSON *root = cJSON_Parse(json_str);
    free(json_str);

    if (root == NULL) {
        printf(COLOR_RED "JSON 解析失败: %s\n" COLOR_RESET, cJSON_GetErrorPtr());
        return 0;
    }

    /* 读取标题 */
    cJSON *title_item = cJSON_GetObjectItem(root, "title");
    if (title_item && cJSON_IsString(title_item)) {
        strncpy(state->story_title, title_item->valuestring, 127);
        state->story_title[127] = '\0';
    }

    /* 读取场景数组 */
    cJSON *scenes_array = cJSON_GetObjectItem(root, "scenes");
    if (scenes_array == NULL || !cJSON_IsArray(scenes_array)) {
        printf(COLOR_RED "JSON 缺少 scenes 数组。\n" COLOR_RESET);
        cJSON_Delete(root);
        return 0;
    }

    int scene_count = cJSON_GetArraySize(scenes_array);
    if (scene_count > MAX_SCENES) scene_count = MAX_SCENES;

    cJSON *json_scene = NULL;
    int idx = 0;
    cJSON_ArrayForEach(json_scene, scenes_array) {
        if (idx >= MAX_SCENES) break;
        if (parseScene(json_scene, &state->scenes[idx])) {
            idx++;
        }
    }
    state->scene_count = idx;
    printf("加载了 %d 个场景\n", state->scene_count);

    /* 读取结局数组 */
    cJSON *endings_array = cJSON_GetObjectItem(root, "endings");
    if (endings_array && cJSON_IsArray(endings_array)) {
        int ending_count = cJSON_GetArraySize(endings_array);
        if (ending_count > 10) ending_count = 10;

        cJSON *json_ending = NULL;
        int eidx = 0;
        cJSON_ArrayForEach(json_ending, endings_array) {
            if (eidx >= 10) break;
            if (parseEnding(json_ending, &state->endings[eidx])) {
                eidx++;
            }
        }
        state->ending_count = eidx;
        printf("加载了 %d 个结局条件\n", state->ending_count);
    }

    /* 读取嫌疑人数组 */
    cJSON *suspects_array = cJSON_GetObjectItem(root, "suspects");
    if (suspects_array && cJSON_IsArray(suspects_array)) {
        int suspect_count = cJSON_GetArraySize(suspects_array);
        if (suspect_count > MAX_SUSPECTS) suspect_count = MAX_SUSPECTS;

        cJSON *json_suspect = NULL;
        int sidx = 0;
        cJSON_ArrayForEach(json_suspect, suspects_array) {
            if (sidx >= MAX_SUSPECTS) break;
            if (parseSuspect(json_suspect, &state->suspects[sidx])) {
                /* 初始信任度设为 50 */
                state->suspect_trust[sidx] = 50;
                sidx++;
            }
        }
        state->suspect_count = sidx;
        printf("加载了 %d 个嫌疑人\n", state->suspect_count);
    }

    cJSON_Delete(root);
    return 1;
}

/* ========== 解析场景 ========== */
static int parseScene(cJSON *json_scene, Scene *scene) {
    if (json_scene == NULL || scene == NULL) return 0;

    memset(scene, 0, sizeof(Scene));

    /* id */
    cJSON *item = cJSON_GetObjectItem(json_scene, "id");
    if (item && cJSON_IsNumber(item)) scene->id = item->valueint;

    /* location */
    item = cJSON_GetObjectItem(json_scene, "location");
    if (item && cJSON_IsString(item)) {
        strncpy(scene->location, item->valuestring, MAX_LOCATION - 1);
    }

    /* atmosphere */
    item = cJSON_GetObjectItem(json_scene, "atmosphere");
    if (item && cJSON_IsString(item)) {
        strncpy(scene->atmosphere, item->valuestring, MAX_ATMOSPHERE - 1);
    }

    /* description */
    item = cJSON_GetObjectItem(json_scene, "description");
    if (item && cJSON_IsString(item)) {
        strncpy(scene->description, item->valuestring, MAX_DESC - 1);
    }

    /* is_random */
    item = cJSON_GetObjectItem(json_scene, "is_random");
    if (item && cJSON_IsNumber(item)) scene->is_random = item->valueint;

    /* is_ending */
    item = cJSON_GetObjectItem(json_scene, "is_ending");
    if (item && cJSON_IsNumber(item)) scene->is_ending = item->valueint;

    /* choices */
    cJSON *choices_array = cJSON_GetObjectItem(json_scene, "choices");
    if (choices_array && cJSON_IsArray(choices_array)) {
        int count = cJSON_GetArraySize(choices_array);
        if (count > MAX_CHOICES) count = MAX_CHOICES;

        cJSON *json_choice = NULL;
        int cidx = 0;
        cJSON_ArrayForEach(json_choice, choices_array) {
            if (cidx >= MAX_CHOICES) break;
            if (parseChoice(json_choice, &scene->choices[cidx])) {
                cidx++;
            }
        }
        scene->choice_count = cidx;
    }

    return 1;
}

/* ========== 解析选项 ========== */
static int parseChoice(cJSON *json_choice, Choice *choice) {
    if (json_choice == NULL || choice == NULL) return 0;

    memset(choice, 0, sizeof(Choice));

    cJSON *item;

    /* text */
    item = cJSON_GetObjectItem(json_choice, "text");
    if (item && cJSON_IsString(item)) {
        strncpy(choice->text, item->valuestring, MAX_DESC - 1);
    }

    /* target_scene */
    item = cJSON_GetObjectItem(json_choice, "target_scene");
    if (item && cJSON_IsNumber(item)) choice->target_scene = item->valueint;

    /* hp_change */
    item = cJSON_GetObjectItem(json_choice, "hp_change");
    if (item && cJSON_IsNumber(item)) choice->hp_change = item->valueint;

    /* clue_change */
    item = cJSON_GetObjectItem(json_choice, "clue_change");
    if (item && cJSON_IsNumber(item)) choice->clue_change = item->valueint;

    /* item_gain */
    item = cJSON_GetObjectItem(json_choice, "item_gain");
    if (item && cJSON_IsString(item)) {
        strncpy(choice->item_gain, item->valuestring, MAX_ITEM_NAME - 1);
    }

    /* item_required */
    item = cJSON_GetObjectItem(json_choice, "item_required");
    if (item && cJSON_IsString(item)) {
        strncpy(choice->item_required, item->valuestring, MAX_ITEM_NAME - 1);
    }

    /* is_hidden */
    item = cJSON_GetObjectItem(json_choice, "is_hidden");
    if (item && cJSON_IsNumber(item)) choice->is_hidden = item->valueint;

    /* hidden_min_clue */
    item = cJSON_GetObjectItem(json_choice, "hidden_min_clue");
    if (item && cJSON_IsNumber(item)) choice->hidden_min_clue = item->valueint;

    /* required_visited - 必须访问过的场景ID数组 */
    item = cJSON_GetObjectItem(json_choice, "required_visited");
    if (item) {
        if (cJSON_IsArray(item)) {
            int arr_size = cJSON_GetArraySize(item);
            if (arr_size > 3) arr_size = 3;
            for (int i = 0; i < arr_size; i++) {
                cJSON *elem = cJSON_GetArrayItem(item, i);
                if (elem && cJSON_IsNumber(elem)) {
                    choice->required_visited[i] = elem->valueint;
                    choice->required_visited_count++;
                }
            }
        } else if (cJSON_IsNumber(item)) {
            /* 向后兼容：单个数字 */
            choice->required_visited[0] = item->valueint;
            choice->required_visited_count = 1;
        }
    }

    return 1;
}

/* ========== 解析结局 ========== */
static int parseEnding(cJSON *json_ending, EndingCondition *ending) {
    if (json_ending == NULL || ending == NULL) return 0;

    memset(ending, 0, sizeof(EndingCondition));

    cJSON *item;

    /* id */
    item = cJSON_GetObjectItem(json_ending, "id");
    if (item && cJSON_IsNumber(item)) ending->id = item->valueint;

    /* type */
    item = cJSON_GetObjectItem(json_ending, "type");
    if (item && cJSON_IsString(item)) {
        if (strcmp(item->valuestring, "perfect") == 0) {
            ending->type = ENDING_PERFECT;
        } else if (strcmp(item->valuestring, "partial") == 0) {
            ending->type = ENDING_PARTIAL;
        } else if (strcmp(item->valuestring, "wrong_accuse") == 0) {
            ending->type = ENDING_WRONG_ACCUSE;
        } else if (strcmp(item->valuestring, "escape") == 0) {
            ending->type = ENDING_ESCAPE;
        } else if (strcmp(item->valuestring, "twist") == 0) {
            ending->type = ENDING_TWIST;
        }
    }

    /* description */
    item = cJSON_GetObjectItem(json_ending, "description");
    if (item && cJSON_IsString(item)) {
        strncpy(ending->description, item->valuestring, MAX_DESC - 1);
    }

    /* min_clue */
    item = cJSON_GetObjectItem(json_ending, "min_clue");
    if (item && cJSON_IsNumber(item)) ending->min_clue = item->valueint;

    /* required_item */
    item = cJSON_GetObjectItem(json_ending, "required_item");
    if (item && cJSON_IsString(item)) {
        strncpy(ending->required_item, item->valuestring, MAX_ITEM_NAME - 1);
    }

    /* required_hp */
    item = cJSON_GetObjectItem(json_ending, "required_hp");
    if (item && cJSON_IsNumber(item)) ending->required_hp = item->valueint;

    /* suspect_flags - 支持单个数字或数组 */
    ending->suspect_flag_count = 0;
    for (int i = 0; i < MAX_SUSPECTS; i++) {
        ending->suspect_flags[i] = -1;
    }
    item = cJSON_GetObjectItem(json_ending, "suspect_flags");
    if (item && cJSON_IsArray(item)) {
        int arr_size = cJSON_GetArraySize(item);
        if (arr_size > MAX_SUSPECTS) arr_size = MAX_SUSPECTS;
        for (int i = 0; i < arr_size; i++) {
            cJSON *elem = cJSON_GetArrayItem(item, i);
            if (elem && cJSON_IsNumber(elem)) {
                ending->suspect_flags[i] = elem->valueint;
            }
        }
        ending->suspect_flag_count = arr_size;
    } else {
        /* 向后兼容：支持旧的 suspect_flag 单值格式 */
        item = cJSON_GetObjectItem(json_ending, "suspect_flag");
        if (item && cJSON_IsNumber(item)) {
            int val = item->valueint;
            if (val >= 0) {
                ending->suspect_flags[0] = val;
                ending->suspect_flag_count = 1;
            }
        }
    }

    return 1;
}

/* ========== 解析嫌疑人 ========== */
static int parseSuspect(cJSON *json_suspect, Suspect *suspect) {
    if (json_suspect == NULL || suspect == NULL) return 0;

    memset(suspect, 0, sizeof(Suspect));

    cJSON *item;

    /* name */
    item = cJSON_GetObjectItem(json_suspect, "name");
    if (item && cJSON_IsString(item)) {
        strncpy(suspect->name, item->valuestring, 31);
    }

    /* identity */
    item = cJSON_GetObjectItem(json_suspect, "identity");
    if (item && cJSON_IsString(item)) {
        strncpy(suspect->identity, item->valuestring, 63);
    }

    /* motive */
    item = cJSON_GetObjectItem(json_suspect, "motive");
    if (item && cJSON_IsString(item)) {
        strncpy(suspect->motive, item->valuestring, MAX_DESC - 1);
    }

    /* alibi */
    item = cJSON_GetObjectItem(json_suspect, "alibi");
    if (item && cJSON_IsString(item)) {
        strncpy(suspect->alibi, item->valuestring, MAX_DESC - 1);
    }

    /* secret */
    item = cJSON_GetObjectItem(json_suspect, "secret");
    if (item && cJSON_IsString(item)) {
        strncpy(suspect->secret, item->valuestring, MAX_DESC - 1);
    }

    return 1;
}

/* ========== 获取可用选项 ========== */
static int getAvailableChoices(const GameState *state, const Scene *scene, int valid_indices[]) {
    if (state == NULL || scene == NULL || valid_indices == NULL) return 0;

    int count = 0;
    for (int i = 0; i < scene->choice_count; i++) {
        const Choice *ch = &scene->choices[i];
        int available = 1;

        /* 检查道具前置条件 */
        if (ch->item_required[0] != '\0') {
            if (!hasItem(state, ch->item_required)) {
                available = 0;
            }
        }

        /* 检查隐藏选项的线索要求 */
        if (ch->is_hidden && ch->hidden_min_clue > state->clue) {
            available = 0;
        }

        /* 检查必须访问过的场景 */
        for (int j = 0; j < ch->required_visited_count; j++) {
            if (ch->required_visited[j] > 0) {
                if (!isSceneVisited(state, ch->required_visited[j])) {
                    available = 0;
                    break;
                }
            }
        }

        if (available) {
            valid_indices[count++] = i;
        }
    }

    return count;
}

/* ========== 开场画面 ========== */
static void showOpening(GameState *state) {
    clearScreen();

    printBanner(state->story_title);

    printf(COLOR_BOLD "【故事背景】\n\n" COLOR_RESET);

    printf("暴雨如注的深夜，警探林正接到紧急电话——\n");
    printf("亿万富豪陈远山被发现死于自家豪宅的反锁书房密室中，死因不明。\n\n");

    printf("当你赶到陈氏豪宅时，大厅灯火通明，空气中弥漫着一股\n");
    printf("淡淡的苦杏仁味。陈远山的尸体躺在书房密室内，\n");
    printf("门从内部反锁——这是一个完美的密室。\n\n");

    printf(COLOR_YELLOW "管家老周" COLOR_RESET "面色苍白地站在一旁，\n");
    printf(COLOR_YELLOW "陈夫人" COLOR_RESET "坐在沙发上低声啜泣，\n");
    printf(COLOR_YELLOW "商业伙伴赵明辉" COLOR_RESET "则在窗边焦急地踱步...\n\n");

    printf("每个人都有不在场证明，每个人似乎都有动机。\n");
    printf("雨越下越大，你必须在天亮之前找出真相。\n\n");

    printDivider('=', 60);

    printf(COLOR_BOLD "\n【游戏说明】\n\n" COLOR_RESET);
    printf("• 每个场景会显示当前情况，你需要从选项中选择行动。\n");
    printf("• 不同选择会影响你的" COLOR_RED "生命值" COLOR_RESET "、");
    printf(COLOR_YELLOW "线索点数" COLOR_RESET "和" COLOR_CYAN "道具收集" COLOR_RESET "。\n");
    printf("• 部分选项需要持有特定道具或达到一定线索值才能选择。\n");
    printf("• 游戏中有随机事件，每次游玩体验可能不同。\n");
    printf("• 收集足够的线索和关键道具，才能揭开真相！\n");
    printf("• 游戏中有 5 种不同的结局等待你的探索。\n\n");

    printDivider('=', 60);

    printf(COLOR_BOLD "\n你扮演警探林正，现在开始调查！\n" COLOR_RESET);

    pressEnterToContinue();
}

/* ========== 清理 ========== */
static void cleanupGame(GameState *state) {
    /* 当前使用静态数组存储数据，无需额外释放内存 */
    /* 如果有动态分配的内存，在此释放 */
    (void)state;
}
