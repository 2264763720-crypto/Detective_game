/**
 * types.h - 侦探推理游戏核心数据结构定义
 * 
 * 本文件定义了游戏中使用的所有结构体和枚举类型，
 * 包括场景、选项、道具、游戏状态、结局条件等。
 */

#ifndef TYPES_H
#define TYPES_H

#include <time.h>

/* ========== 常量定义 ========== */
#define MAX_ITEMS       20      /* 最大道具数量 */
#define MAX_CHOICES     5       /* 每个场景最大选项数 */
#define MAX_SCENES      100     /* 最大场景数 */
#define MAX_ITEM_NAME   32      /* 道具名称最大长度 */
#define MAX_DESC        1024    /* 描述文本最大长度 */
#define MAX_SUSPECTS    5       /* 最大嫌疑人数 */
#define MAX_LOCATION    32      /* 地点名称最大长度 */
#define MAX_ATMOSPHERE  16      /* 氛围标签最大长度 */

/* ========== 结局类型枚举 ========== */
typedef enum {
    ENDING_PERFECT = 0,     /* A.完美破案 - 收集全部关键线索，证据链完整 */
    ENDING_PARTIAL,         /* B.部分成功 - 线索不足但直觉猜对 */
    ENDING_WRONG_ACCUSE,    /* C.错判无辜 - 被表面证据误导，指控错误 */
    ENDING_ESCAPE,          /* D.真凶逃脱 - 关键线索遗漏，真凶逍遥法外 */
    ENDING_TWIST            /* E.意外反转 - 发现幕后还有更大的阴谋 */
} EndingType;

/* ========== 道具结构 ========== */
typedef struct {
    char name[MAX_ITEM_NAME];           /* 道具名称 */
    char description[MAX_DESC];         /* 道具描述 */
} Item;

/* ========== 选项结构 ========== */
typedef struct {
    char text[MAX_DESC];                /* 选项文本（展示给玩家） */
    int  target_scene;                  /* 跳转目标场景ID */
    int  hp_change;                     /* 生命值变化（正=回复，负=扣减） */
    int  clue_change;                   /* 线索值变化 */
    char item_gain[MAX_ITEM_NAME];      /* 选择后获得的道具名（空串=无） */
    char item_required[MAX_ITEM_NAME];  /* 前置道具要求（空串=无要求） */
    int  is_hidden;                     /* 是否隐藏选项（1=需要满足条件才显示） */
    int  hidden_min_clue;               /* 隐藏选项所需最低线索值 */
    int  required_visited_count;         /* 必须访问的场景数量 */
    int  required_visited[3];            /* 必须访问过的场景ID列表，用于确保关键剧情不跳过 */
} Choice;

/* ========== 场景结构 ========== */
typedef struct {
    int  id;                            /* 场景唯一ID */
    char location[MAX_LOCATION];        /* 场景地点标签 */
    char atmosphere[MAX_ATMOSPHERE];    /* 氛围标签（紧张/悬疑/调查/发现/危险） */
    char description[MAX_DESC];         /* 场景文字描述 */
    int  choice_count;                  /* 可用选项数量 */
    Choice choices[MAX_CHOICES];        /* 选项数组 */
    int  is_random;                     /* 是否随机场景（1=随机触发子分支） */
    int  is_ending;                     /* 是否结局场景 */
} Scene;

/* ========== 结局条件结构 ========== */
typedef struct {
    int  id;                            /* 结局场景ID */
    EndingType type;                    /* 结局类型 */
    char description[MAX_DESC];         /* 结局叙述文字 */
    int  min_clue;                      /* 最低线索要求 */
    char required_item[MAX_ITEM_NAME];  /* 必须持有的关键道具 */
    int  required_hp;                   /* 最低生命值要求（0=无要求） */
    int  suspect_flags[MAX_SUSPECTS];   /* 指认标记数组（对应嫌疑人索引，全-1=无要求） */
    int  suspect_flag_count;            /* 需要匹配的嫌疑人数量 */
} EndingCondition;

/* ========== 嫌疑人档案结构 ========== */
typedef struct {
    char name[32];                      /* 姓名 */
    char identity[64];                  /* 身份 */
    char motive[MAX_DESC];              /* 动机 */
    char alibi[MAX_DESC];               /* 不在场证明 */
    char secret[MAX_DESC];              /* 隐藏秘密 */
    int  trust_level;                   /* 信任度（0-100） */
} Suspect;

/* ========== 游戏状态结构 ========== */
typedef struct {
    int  current_scene_id;              /* 当前场景ID */
    int  hp;                            /* 生命值（0-100，归零则游戏结束） */
    int  clue;                          /* 线索点数 */
    int  item_count;                    /* 当前持有道具数量 */
    Item items[MAX_ITEMS];              /* 道具数组 */
    int  suspect_trust[MAX_SUSPECTS];   /* 各嫌疑人信任度 */
    Scene scenes[MAX_SCENES];           /* 所有场景数据 */
    int  scene_count;                   /* 场景总数 */
    EndingCondition endings[10];        /* 结局条件数组 */
    int  ending_count;                  /* 结局条件数量 */
    Suspect suspects[MAX_SUSPECTS];     /* 嫌疑人档案 */
    int  suspect_count;                 /* 嫌疑人数量 */
    int  accused_suspects[MAX_SUSPECTS]; /* 玩家最终指认的嫌疑人索引数组（-1=未指认） */
    int  accused_count;                 /* 指认的嫌疑人数量 */
    int  visited_scenes[MAX_SCENES];    /* 已访问场景ID记录 */
    int  visited_count;                 /* 已访问场景数 */
    char story_title[128];              /* 故事标题 */
    time_t start_time;                  /* 游戏开始时间戳 */
} GameState;

/* 结局类型字符串映射（在 engine.c 中使用） */

#endif /* TYPES_H */
