/**
 * engine.h - 游戏引擎头文件
 * 
 * 声明游戏引擎的核心功能：
 * - 场景渲染与状态面板显示
 * - 选项过滤（道具前置条件检查）
 * - 选择处理与效果应用
 * - 随机事件触发
 * - 结局综合判定
 */

#ifndef ENGINE_H
#define ENGINE_H

#include "types.h"
#include "utils.h"

/* ========== 函数声明 ========== */

/**
 * 显示游戏状态面板
 * 包括：生命值、线索点数、持有道具列表、嫌疑人信任度
 * @param state  当前游戏状态
 */
void showStatusPanel(const GameState *state);

/**
 * 检查玩家是否持有指定道具
 * @param state  游戏状态
 * @param name   道具名称
 * @return       1=持有，0=未持有
 */
int hasItem(const GameState *state, const char *name);

/**
 * 添加道具到玩家背包
 * @param state  游戏状态
 * @param name   道具名称
 * @param desc   道具描述
 * @return       1=添加成功，0=背包已满
 */
int addItem(GameState *state, const char *name, const char *desc);

/**
 * 渲染当前场景
 * 显示场景地点、氛围、描述文字、可用选项
 * @param state  游戏状态
 */
void renderScene(const GameState *state);

/**
 * 处理玩家选择
 * 应用选项效果（HP/Clue/Item变化），返回目标场景ID
 * @param state       游戏状态
 * @param choice_idx  玩家选择的选项索引
 * @return            跳转目标场景ID
 */
int processChoice(GameState *state, int choice_idx);

/**
 * 应用选项效果
 * 修改HP、线索值、添加道具
 * @param state  游戏状态
 * @param c      选中的选项
 */
void applyEffect(GameState *state, const Choice *c);

/**
 * 处理随机事件
 * 如果当前场景标记为随机场景，使用rand()决定子分支
 * @param state  游戏状态
 * @return       实际跳转的目标场景ID
 */
int handleRandomEvent(const GameState *state, int base_choice_idx);

/**
 * 综合结局判定
 * 根据线索值、持有道具、历史选择等条件决定结局类型
 * @param state  游戏状态
 * @return       匹配的结局条件指针，NULL=无匹配（使用默认）
 */
const EndingCondition* checkEnding(const GameState *state);

/**
 * 记录已访问场景
 * @param state     游戏状态
 * @param scene_id  场景ID
 */
void recordVisitedScene(GameState *state, int scene_id);

/**
 * 检查场景是否已访问过
 * @param state     游戏状态
 * @param scene_id  场景ID
 * @return          1=已访问，0=未访问
 */
int isSceneVisited(const GameState *state, int scene_id);

/**
 * 根据ID查找场景
 * @param state     游戏状态
 * @param scene_id  场景ID
 * @return          场景指针，NULL=未找到
 */
Scene* findScene(GameState *state, int scene_id);

/**
 * 根据类型获取结局条件
 * @param state        游戏状态
 * @param ending_type  结局类型
 * @return             结局条件指针，NULL=未找到
 */
const EndingCondition* findEndingByType(const GameState *state, EndingType ending_type);

/**
 * 显示结局画面
 * @param state    游戏状态
 * @param ending   结局条件
 */
void showEndingScreen(const GameState *state, const EndingCondition *ending);

/**
 * 显示嫌疑人档案
 * @param state  游戏状态
 */
void showSuspectProfiles(const GameState *state);

/**
 * 显示指认环节，让玩家选择最终嫌疑人
 * @param state  游戏状态
 * @return       玩家指认的嫌疑人索引（0-4），-1=取消
 */
int showAccusationScreen(GameState *state);

#endif /* ENGINE_H */
