/**
 * engine.c - 游戏引擎实现
 * 
 * 核心功能：
 * 1. 场景渲染 - 显示场景描述、地点、氛围、可用选项
 * 2. 状态面板 - 展示HP、线索、道具、嫌疑人信任度
 * 3. 选项过滤 - 检查道具前置条件，隐藏不可用选项
 * 4. 效果应用 - 处理HP/Clue/Item变化
 * 5. 随机事件 - rand()决定随机场景分支
 * 6. 结局判定 - 综合条件判定结局类型
 */

#include "engine.h"
#include <time.h>
#include <stdlib.h>

/* ========== 全局随机种子标志 ========== */
static int rand_seeded = 0;

/**
 * 确保随机种子已设置
 */
static void ensureRandSeed(void) {
    if (!rand_seeded) {
        srand((unsigned int)time(NULL));
        rand_seeded = 1;
    }
}

/* ========== 道具检查 ========== */
int hasItem(const GameState *state, const char *name) {
    if (state == NULL || name == NULL || name[0] == '\0') {
        return 0;
    }
    for (int i = 0; i < state->item_count; i++) {
        if (strcmp(state->items[i].name, name) == 0) {
            return 1;
        }
    }
    return 0;
}

int addItem(GameState *state, const char *name, const char *desc) {
    if (state == NULL || name == NULL || name[0] == '\0') {
        return 0;
    }
    if (state->item_count >= MAX_ITEMS) {
        printf(COLOR_RED "背包已满，无法获得新道具！\n" COLOR_RESET);
        return 0;
    }
    /* 检查是否已拥有 */
    if (hasItem(state, name)) {
        return 1; /* 已拥有，不算失败 */
    }
    /* 添加新道具 */
    strncpy(state->items[state->item_count].name, name, MAX_ITEM_NAME - 1);
    state->items[state->item_count].name[MAX_ITEM_NAME - 1] = '\0';
    if (desc != NULL) {
        strncpy(state->items[state->item_count].description, desc, MAX_DESC - 1);
        state->items[state->item_count].description[MAX_DESC - 1] = '\0';
    }
    state->item_count++;
    return 1;
}

/* ========== 状态面板 ========== */
void showStatusPanel(const GameState *state) {
    if (state == NULL) return;

    printf(COLOR_CYAN "\n┌──────────────────────────────────────────────────────┐\n" COLOR_RESET);
    printf(COLOR_CYAN "│" COLOR_RESET " " COLOR_BOLD "侦探状态" COLOR_RESET "                                          " COLOR_CYAN "│\n" COLOR_RESET);
    printf(COLOR_CYAN "├──────────────────────────────────────────────────────┤\n" COLOR_RESET);

    /* 生命值 - 根据HP值变色 */
    printf(COLOR_CYAN "│" COLOR_RESET " 生命值 (HP): ");
    if (state->hp >= 70) {
        printf(COLOR_GREEN "%d/100" COLOR_RESET, state->hp);
    } else if (state->hp >= 30) {
        printf(COLOR_YELLOW "%d/100" COLOR_RESET, state->hp);
    } else {
        printf(COLOR_RED "%d/100" COLOR_RESET, state->hp);
    }
    /* HP 进度条 */
    printf(" [");
    int bar_len = state->hp / 5;
    for (int i = 0; i < 20; i++) {
        if (i < bar_len) printf("█");
        else printf("░");
    }
    printf("]\n");

    /* 线索点数 */
    printf(COLOR_CYAN "│" COLOR_RESET " 线索点数:  " COLOR_YELLOW "%d" COLOR_RESET "\n", state->clue);

    /* 持有道具 */
    printf(COLOR_CYAN "│" COLOR_RESET " 持有道具:  ");
    if (state->item_count == 0) {
        printf(COLOR_DIM "(无)" COLOR_RESET);
    } else {
        printf(COLOR_GREEN);
        for (int i = 0; i < state->item_count; i++) {
            printf("%s", state->items[i].name);
            if (i < state->item_count - 1) printf(", ");
        }
        printf(COLOR_RESET);
    }
    printf("\n");

    /* 已用时间 */
    if (state->start_time > 0) {
        time_t now = time(NULL);
        int elapsed = (int)(now - state->start_time);
        int minutes = elapsed / 60;
        int seconds = elapsed % 60;
        printf(COLOR_CYAN "│" COLOR_RESET " 已用时间:  " COLOR_BLUE "%02d分%02d秒" COLOR_RESET "\n", minutes, seconds);
    }

    /* 嫌疑人信任度 */
    printf(COLOR_CYAN "│" COLOR_RESET " 嫌疑人信任度:\n");
    for (int i = 0; i < state->suspect_count; i++) {
        printf(COLOR_CYAN "│" COLOR_RESET "   %s: ", state->suspects[i].name);
        if (state->suspect_trust[i] >= 60) {
            printf(COLOR_GREEN "信任 %d%%" COLOR_RESET, state->suspect_trust[i]);
        } else if (state->suspect_trust[i] >= 30) {
            printf(COLOR_YELLOW "中立 %d%%" COLOR_RESET, state->suspect_trust[i]);
        } else {
            printf(COLOR_RED "怀疑 %d%%" COLOR_RESET, state->suspect_trust[i]);
        }
        printf("\n");
    }

    printf(COLOR_CYAN "└──────────────────────────────────────────────────────┘\n" COLOR_RESET);
}

/* ========== 场景渲染 ========== */
void renderScene(const GameState *state) {
    if (state == NULL) return;

    Scene *scene = findScene((GameState *)state, state->current_scene_id);
    if (scene == NULL) {
        printf(COLOR_RED "错误：找不到场景 ID %d\n" COLOR_RESET, state->current_scene_id);
        return;
    }

    /* 地点和氛围标签 */
    printf(COLOR_MAGENTA "\n【%s】" COLOR_RESET, scene->location);
    printf("  氛围: " COLOR_BLUE "%s" COLOR_RESET "\n", scene->atmosphere);
    printDivider('-', 60);

    /* 场景描述 */
    printf("\n");
    /* 根据氛围选择颜色 */
    if (strcmp(scene->atmosphere, "紧张") == 0 || strcmp(scene->atmosphere, "危险") == 0) {
        printf(COLOR_RED);
    } else if (strcmp(scene->atmosphere, "悬疑") == 0) {
        printf(COLOR_YELLOW);
    } else if (strcmp(scene->atmosphere, "发现") == 0) {
        printf(COLOR_GREEN);
    } else {
        printf(COLOR_WHITE);
    }
    printf("%s", scene->description);
    printf(COLOR_RESET "\n\n");

    printDivider('-', 60);

    /* 显示可用选项 */
    printf(COLOR_BOLD "\n请选择你的行动：\n\n" COLOR_RESET);

    for (int i = 0; i < scene->choice_count; i++) {
        Choice *ch = &scene->choices[i];

        /* 检查选项是否可用 */
        int available = 1;

        if (ch->item_required[0] != '\0') {
            if (!hasItem(state, ch->item_required)) {
                available = 0;
            }
        }

        if (ch->is_hidden && ch->hidden_min_clue > state->clue) {
            available = 0;
        }

        for (int j = 0; j < ch->required_visited_count; j++) {
            if (ch->required_visited[j] > 0) {
                if (!isSceneVisited(state, ch->required_visited[j])) {
                    available = 0;
                    break;
                }
            }
        }

        if (available) {
            /* 可用选项 */
            printf("  " COLOR_GREEN "[%d]" COLOR_RESET " %s\n", i + 1, ch->text);
            /* 显示效果提示 */
            printf("     ");
            if (ch->hp_change != 0) {
                if (ch->hp_change > 0) {
                    printf(COLOR_GREEN "HP+%d " COLOR_RESET, ch->hp_change);
                } else {
                    printf(COLOR_RED "HP%d " COLOR_RESET, ch->hp_change);
                }
            }
            if (ch->clue_change != 0) {
                printf(COLOR_YELLOW "线索+%d " COLOR_RESET, ch->clue_change);
            }
            if (ch->item_gain[0] != '\0') {
                printf(COLOR_CYAN "获得: %s " COLOR_RESET, ch->item_gain);
            }
            printf("\n");
        } else {
            /* 不可用选项 */
            printf("  " COLOR_DIM "[X] %s" COLOR_RESET, ch->text);
            if (ch->item_required[0] != '\0') {
                printf(COLOR_DIM " [需要道具: %s]" COLOR_RESET, ch->item_required);
            }
            if (ch->is_hidden && ch->hidden_min_clue > state->clue) {
                printf(COLOR_DIM " [需要线索值: %d]" COLOR_RESET, ch->hidden_min_clue);
            }
            for (int j = 0; j < ch->required_visited_count; j++) {
                if (ch->required_visited[j] > 0 && !isSceneVisited(state, ch->required_visited[j])) {
                    Scene *req_scene = findScene((GameState *)state, ch->required_visited[j]);
                    if (req_scene) {
                        printf(COLOR_DIM " [需先调查: %s]" COLOR_RESET, req_scene->location);
                    } else {
                        printf(COLOR_DIM " [需要先完成关键调查]" COLOR_RESET);
                    }
                }
            }
            printf("\n");
        }
    }

    printDivider('-', 60);
}

/* ========== 效果应用 ========== */
void applyEffect(GameState *state, const Choice *c) {
    if (state == NULL || c == NULL) return;

    /* HP 变化 */
    state->hp += c->hp_change;
    if (state->hp > 100) state->hp = 100;
    if (state->hp < 0) state->hp = 0;

    /* 线索变化 */
    state->clue += c->clue_change;
    if (state->clue > 100) state->clue = 100;
    if (state->clue < 0) state->clue = 0;

    /* 获得道具 */
    if (c->item_gain[0] != '\0') {
        if (addItem(state, c->item_gain, "")) {
            printf(COLOR_GREEN "\n✓ 获得了新道具: %s\n" COLOR_RESET, c->item_gain);
        }
    }

    /* 显示变化摘要 */
    if (c->hp_change != 0) {
        if (c->hp_change > 0) {
            printf(COLOR_GREEN "生命值恢复了 %d 点\n" COLOR_RESET, c->hp_change);
        } else {
            printf(COLOR_RED "生命值减少了 %d 点\n" COLOR_RESET, -c->hp_change);
        }
    }
    if (c->clue_change != 0) {
        printf(COLOR_YELLOW "线索值变化 %+d\n" COLOR_RESET, c->clue_change);
    }
}

/* ========== 处理选择 ========== */
int processChoice(GameState *state, int choice_idx) {
    if (state == NULL) return -1;

    Scene *scene = findScene(state, state->current_scene_id);
    if (scene == NULL) return -1;

    if (choice_idx < 0 || choice_idx >= scene->choice_count) {
        printf(COLOR_RED "无效的选择索引。\n" COLOR_RESET);
        return state->current_scene_id;
    }

    Choice *chosen = &scene->choices[choice_idx];

    /* 应用效果 */
    applyEffect(state, chosen);

    /* 检查 HP 是否归零 */
    if (state->hp <= 0) {
        printf(COLOR_RED "\n⚠ 你的生命值耗尽！调查被迫终止...\n" COLOR_RESET);
        return -1; /* 特殊值，表示强制结束 */
    }

    /* 记录访问 */
    recordVisitedScene(state, state->current_scene_id);

    return chosen->target_scene;
}

/* ========== 随机事件处理 ========== */
int handleRandomEvent(const GameState *state, int base_choice_idx) {
    ensureRandSeed();

    Scene *scene = findScene((GameState *)state, state->current_scene_id);
    if (scene == NULL || !scene->is_random) {
        return base_choice_idx; /* 非随机场景，直接返回 */
    }

    /* 使用当前场景ID和HP作为随机种子的一部分 */
    int roll = rand() % 100;

    printf(COLOR_MAGENTA "\n  ⚡ 随机事件触发！\n" COLOR_RESET);

    /* 暗杀危机：5%概率，仅当线索值<20时触发 */
    if (roll < 5 && state->clue < 20) {
        printf(COLOR_RED "\n  ☠ 危机事件：暗杀！\n" COLOR_RESET);
        printf(COLOR_RED "  一个黑影突然从暗处冲出！你还没来得及反应，\n" COLOR_RESET);
        printf(COLOR_RED "  一把冰冷的匕首已经刺入了你的胸口...\n" COLOR_RESET);
        printf(COLOR_RED "  你的调查经验不足，未能察觉潜伏的危险。\n" COLOR_RESET);
        printf(COLOR_RED "  生命值直接归零！\n" COLOR_RESET);
        ((GameState *)state)->hp = 0;
        return base_choice_idx;
    }

    /* 根据随机数决定额外效果 */
    if (roll < 30) {
        /* 30% 概率触发额外线索 */
        printf(COLOR_YELLOW "  在搜索过程中，你意外发现了一个被忽略的细节！线索+5\n" COLOR_RESET);
        ((GameState *)state)->clue += 5;
        if (((GameState *)state)->clue > 100) ((GameState *)state)->clue = 100;
    } else if (roll < 55) {
        /* 25% 概率触发 HP 损耗 */
        printf(COLOR_RED "  你不慎触发了警报装置，受到了电击伤害！HP-10\n" COLOR_RESET);
        ((GameState *)state)->hp -= 10;
        if (((GameState *)state)->hp < 0) ((GameState *)state)->hp = 0;
    } else if (roll < 70) {
        /* 15% 概率发现额外道具 */
        if (((GameState *)state)->item_count < MAX_ITEMS) {
            const char *random_items[] = {"破碎的玻璃片", "旧报纸碎片", "可疑的收据"};
            int item_idx = rand() % 3;
            printf(COLOR_GREEN "  你在角落发现了一样东西！获得: %s\n" COLOR_RESET, random_items[item_idx]);
            addItem((GameState *)state, random_items[item_idx], "随机发现的道具");
        }
    } else if (roll < 80) {
        /* 10% 概率触发轻度伤害 */
        printf(COLOR_RED "  你在调查中不慎摔倒，受了些擦伤。HP-3\n" COLOR_RESET);
        ((GameState *)state)->hp -= 3;
        if (((GameState *)state)->hp < 0) ((GameState *)state)->hp = 0;
    }

    return base_choice_idx;
}

/* ========== 结局判定 ========== */
const EndingCondition* checkEnding(const GameState *state) {
    if (state == NULL) return NULL;

    /* 如果 HP 归零，强制返回失败结局 */
    if (state->hp <= 0) {
        for (int i = 0; i < state->ending_count; i++) {
            if (state->endings[i].type == ENDING_ESCAPE) {
                return &state->endings[i];
            }
        }
        return NULL;
    }

    /* 收集所有符合条件的结局 */
    const EndingCondition *candidates[10];
    int candidate_count = 0;

    for (int i = 0; i < state->ending_count; i++) {
        const EndingCondition *ending = &state->endings[i];

        /* 检查线索要求 */
        if (state->clue < ending->min_clue) continue;

        /* 检查道具要求 */
        if (ending->required_item[0] != '\0') {
            if (!hasItem(state, ending->required_item)) continue;
        }

        /* 检查 HP 要求 */
        if (ending->required_hp > 0 && state->hp < ending->required_hp) continue;

        /* 检查指认标记：如果结局有要求（suspect_flag_count > 0），
           则玩家的指认必须完全包含结局要求的所有嫌疑人 */
        if (ending->suspect_flag_count > 0) {
            if (state->accused_count == 0) continue;  /* 玩家没指认任何人 */
            int all_found = 1;
            for (int j = 0; j < ending->suspect_flag_count; j++) {
                int flag = ending->suspect_flags[j];
                int found = 0;
                for (int k = 0; k < state->accused_count; k++) {
                    if (state->accused_suspects[k] == flag) {
                        found = 1;
                        break;
                    }
                }
                if (!found) { all_found = 0; break; }
            }
            if (!all_found) continue;
        }

        candidates[candidate_count++] = ending;
    }

    if (candidate_count == 0) {
        /* 没有符合条件的结局，返回默认（真凶逃脱） */
        for (int i = 0; i < state->ending_count; i++) {
            if (state->endings[i].type == ENDING_ESCAPE) {
                return &state->endings[i];
            }
        }
        return NULL;
    }

    /* 从候选中选择最优结局（优先级：PERFECT > TWIST > PARTIAL > WRONG_ACCUSE > ESCAPE） */
    const EndingCondition *best = candidates[0];
    for (int i = 1; i < candidate_count; i++) {
        if (candidates[i]->type < best->type) {
            best = candidates[i];
        }
    }

    return best;
}

/* ========== 场景访问记录 ========== */
void recordVisitedScene(GameState *state, int scene_id) {
    if (state == NULL) return;
    for (int i = 0; i < state->visited_count; i++) {
        if (state->visited_scenes[i] == scene_id) return;
    }
    if (state->visited_count < MAX_SCENES) {
        state->visited_scenes[state->visited_count++] = scene_id;
    }
}

int isSceneVisited(const GameState *state, int scene_id) {
    if (state == NULL) return 0;
    for (int i = 0; i < state->visited_count; i++) {
        if (state->visited_scenes[i] == scene_id) return 1;
    }
    return 0;
}

/* ========== 场景查找 ========== */
Scene* findScene(GameState *state, int scene_id) {
    if (state == NULL) return NULL;
    for (int i = 0; i < state->scene_count; i++) {
        if (state->scenes[i].id == scene_id) {
            return &state->scenes[i];
        }
    }
    return NULL;
}

/* ========== 结局查找 ========== */
const EndingCondition* findEndingByType(const GameState *state, EndingType ending_type) {
    if (state == NULL) return NULL;
    for (int i = 0; i < state->ending_count; i++) {
        if (state->endings[i].type == ending_type) {
            return &state->endings[i];
        }
    }
    return NULL;
}

/* ========== 结局画面 ========== */
void showEndingScreen(const GameState *state, const EndingCondition *ending) {
    clearScreen();

    printf("\n");
    printColoredDivider(COLOR_CYAN, '=', 60);
    printf(COLOR_CYAN COLOR_BOLD);
    printf("                    结 局 揭 晓\n");
    printf(COLOR_RESET);
    printColoredDivider(COLOR_CYAN, '=', 60);

    /* 结局类型名称 */
    printf("\n  ");
    switch (ending->type) {
        case ENDING_PERFECT:
            printf(COLOR_GREEN COLOR_BOLD "★ 完美破案 ★" COLOR_RESET);
            break;
        case ENDING_PARTIAL:
            printf(COLOR_YELLOW COLOR_BOLD "▲ 部分成功 ▲" COLOR_RESET);
            break;
        case ENDING_WRONG_ACCUSE:
            printf(COLOR_RED COLOR_BOLD "✘ 错判无辜 ✘" COLOR_RESET);
            break;
        case ENDING_ESCAPE:
            printf(COLOR_RED COLOR_BOLD "⚠ 真凶逃脱 ⚠" COLOR_RESET);
            break;
        case ENDING_TWIST:
            printf(COLOR_MAGENTA COLOR_BOLD "◆ 意外反转 ◆" COLOR_RESET);
            break;
        default:
            printf(COLOR_WHITE COLOR_BOLD "结局" COLOR_RESET);
            break;
    }
    printf("\n\n");

    /* 结局叙述 */
    printf("  %s\n\n", ending->description);

    printDivider('-', 60);

    /* 调查总结 */
    printf(COLOR_BOLD "\n  ── 调查总结 ──\n\n" COLOR_RESET);
    printf("  最终生命值: %d/100\n", state->hp);
    printf("  收集线索点数: %d\n", state->clue);
    printf("  收集道具数量: %d", state->item_count);
    if (state->item_count > 0) {
        printf(" (");
        for (int i = 0; i < state->item_count; i++) {
            printf("%s", state->items[i].name);
            if (i < state->item_count - 1) printf(", ");
        }
        printf(")");
    }
    printf("\n");
    printf("  访问场景数量: %d\n", state->visited_count);

    if (state->accused_count > 0) {
        printf("  最终指认: ");
        for (int i = 0; i < state->accused_count; i++) {
            printf("%s", state->suspects[state->accused_suspects[i]].name);
            if (i < state->accused_count - 1) printf(", ");
        }
        printf("\n");
    }

    printDivider('=', 60);
    printf(COLOR_CYAN "\n  感谢游玩《%s》！\n" COLOR_RESET, state->story_title);
    printf(COLOR_DIM "  重新运行程序可以体验不同的剧情分支。\n" COLOR_RESET);
    printDivider('=', 60);
    printf("\n");
}

/* ========== 嫌疑人档案 ========== */
void showSuspectProfiles(const GameState *state) {
    if (state == NULL) return;

    printf(COLOR_CYAN "\n┌──────────────────────────────────────────────────────┐\n" COLOR_RESET);
    printf(COLOR_CYAN "│" COLOR_RESET " " COLOR_BOLD "嫌疑人档案" COLOR_RESET "                                          " COLOR_CYAN "│\n" COLOR_RESET);
    printf(COLOR_CYAN "└──────────────────────────────────────────────────────┘\n" COLOR_RESET);

    for (int i = 0; i < state->suspect_count; i++) {
        const Suspect *s = &state->suspects[i];
        printf(COLOR_YELLOW "\n  [%d] %s" COLOR_RESET " - %s\n", i + 1, s->name, s->identity);
        printf("  动机: %s\n", s->motive);
        printf("  不在场证明: %s\n", s->alibi);
        if (state->clue >= 40) {
            printf("  隐藏秘密: %s\n", s->secret);
        } else {
            printf("  隐藏秘密: " COLOR_DIM "(线索不足，无法获知)" COLOR_RESET "\n");
        }
    }
    printf("\n");
}

/* ========== 指认环节 ========== */
int showAccusationScreen(GameState *state) {
    if (state == NULL || state->suspect_count == 0) return -1;

    clearScreen();

    printf("\n");
    printColoredDivider(COLOR_CYAN, '=', 60);
    printf(COLOR_CYAN COLOR_BOLD);
    printf("                   指 认 环 节\n");
    printf(COLOR_RESET);
    printColoredDivider(COLOR_CYAN, '=', 60);

    printf("\n  " COLOR_YELLOW "经过漫长的调查，你已收集了大量线索。\n" COLOR_RESET);
    printf("  " COLOR_YELLOW "现在，是时候做出最终判断了。\n" COLOR_RESET);
    printf("\n  " COLOR_BOLD "你认为谁与本案有关？（可多选）\n" COLOR_RESET);
    printf("  " COLOR_DIM "提示：案件可能涉及多人，你可以选择1个或多个嫌疑人。\n" COLOR_RESET);
    printf("\n");

    /* 显示嫌疑人列表 */
    for (int i = 0; i < state->suspect_count; i++) {
        const Suspect *s = &state->suspects[i];
        printf("  " COLOR_GREEN "[%d]" COLOR_RESET " " COLOR_YELLOW "%s" COLOR_RESET " - %s\n", i + 1, s->name, s->identity);
        printf("      动机: %s\n", s->motive);
        printf("\n");
    }

    /* 线索值>=50时给予推理提示 */
    if (state->clue >= 50) {
        printDivider('-', 60);
        printf(COLOR_GREEN "\n  推理提示（线索值>=50解锁）：\n" COLOR_RESET);
        printf("  案件涉及多人交织的动机与行为。仔细回顾：\n");
        printf("  - 毒药瓶上有谁的指纹？\n");
        printf("  - 神秘访客的真实目的是什么？\n");
        printf("  - 谁最有条件在酒中下毒？\n");
        printf("  - 密室的门锁是如何反锁的？\n");
        printf("\n");
    }

    printDivider('-', 60);

    /* 获取玩家指认 - 支持多选 */
    printf("\n  " COLOR_BOLD "请选择你认为有罪的嫌疑人编号" COLOR_RESET);
    printf("\n  " COLOR_DIM "输入格式：用空格或逗号分隔多个编号，如 \"1 2 4\" 或 \"1,2,4\"" COLOR_RESET);
    printf("\n  " COLOR_DIM "输入 0 表示暂不指认" COLOR_RESET);
    printf("\n");

    char input_buffer[64];
    printf("> ");
    if (fgets(input_buffer, sizeof(input_buffer), stdin) == NULL) {
        printf(COLOR_RED "\n  输入错误。\n" COLOR_RESET);
        return -1;
    }

    /* 解析输入：支持空格或逗号分隔 */
    int selected[MAX_SUSPECTS] = {0, 0, 0, 0, 0};
    int selected_count = 0;
    char *token = strtok(input_buffer, " ,\t\n\r");

    while (token != NULL && selected_count < MAX_SUSPECTS) {
        int num = 0;
        if (sscanf(token, "%d", &num) == 1) {
            if (num == 0) {
                printf(COLOR_YELLOW "\n  你决定暂不指认，等待更多证据...\n" COLOR_RESET);
                state->accused_count = 0;
                return -1;
            }
            if (num >= 1 && num <= state->suspect_count) {
                /* 检查是否重复 */
                int dup = 0;
                for (int i = 0; i < selected_count; i++) {
                    if (selected[i] == num - 1) { dup = 1; break; }
                }
                if (!dup) {
                    selected[selected_count++] = num - 1;
                }
            }
        }
        token = strtok(NULL, " ,\t\n\r");
    }

    if (selected_count == 0) {
        printf(COLOR_YELLOW "\n  未识别到有效选择，暂不指认...\n" COLOR_RESET);
        state->accused_count = 0;
        return -1;
    }

    /* 显示指认结果 */
    printf(COLOR_BOLD "\n  你指认了以下嫌疑人与本案有关：\n" COLOR_RESET);
    for (int i = 0; i < selected_count; i++) {
        printf("    " COLOR_RED "● %s" COLOR_RESET " (%s)\n",
               state->suspects[selected[i]].name,
               state->suspects[selected[i]].identity);
    }

    /* 存储指认结果 */
    for (int i = 0; i < selected_count; i++) {
        state->accused_suspects[i] = selected[i];
    }
    state->accused_count = selected_count;

    printf(COLOR_DIM "\n  让我们看看你的判断是否正确...\n" COLOR_RESET);
    pressEnterToContinue();

    return selected_count;
}
