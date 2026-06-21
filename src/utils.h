/**
 * utils.h - 工具函数头文件
 * 
 * 提供清屏、安全输入、彩色文字输出、横幅打印等功能。
 * 兼容 Windows 控制台（使用 ANSI 转义序列实现彩色输出）。
 */

#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ========== 颜色常量（ANSI 转义序列） ========== */
#define COLOR_RESET     "\033[0m"
#define COLOR_RED       "\033[31m"
#define COLOR_GREEN     "\033[32m"
#define COLOR_YELLOW    "\033[33m"
#define COLOR_BLUE      "\033[34m"
#define COLOR_MAGENTA   "\033[35m"
#define COLOR_CYAN      "\033[36m"
#define COLOR_WHITE     "\033[37m"
#define COLOR_BOLD      "\033[1m"
#define COLOR_DIM       "\033[2m"

/* ========== 函数声明 ========== */

/**
 * 初始化控制台环境
 * Windows: 设置代码页为 UTF-8（解决中文乱码）
 * Linux/Mac: 无操作
 */
void initConsole(void);

/**
 * 清空控制台屏幕
 * Windows: 使用 system("cls")
 * Linux/Mac: 使用 system("clear")
 */
void clearScreen(void);

/**
 * 安全整数输入函数
 * 使用 fgets + sscanf 替代裸 scanf，防止缓冲区溢出
 * @param prompt    提示文字
 * @param min       最小值
 * @param max       最大值
 * @return          用户输入的有效整数
 */
int getIntInput(const char *prompt, int min, int max);

/**
 * 打印带颜色的文字
 * @param color   ANSI 颜色代码
 * @param text    要打印的文字
 */
void printColored(const char *color, const char *text);

/**
 * 打印游戏横幅标题
 * @param title  标题文字
 */
void printBanner(const char *title);

/**
 * 打印分隔线
 * @param ch      分隔线字符
 * @param length  分隔线长度
 */
void printDivider(char ch, int length);

/**
 * 打印带颜色的分隔线
 * @param color   颜色代码
 * @param ch      分隔线字符
 * @param length  分隔线长度
 */
void printColoredDivider(const char *color, char ch, int length);

/**
 * 暂停并等待玩家按回车继续
 */
void pressEnterToContinue(void);

/**
 * 逐字打印文字（打字机效果）
 * @param text  要打印的文字
 * @param delay 每个字符之间的延迟（毫秒），0=立即输出
 */
void typeWriterPrint(const char *text, int delay);

#endif /* UTILS_H */
