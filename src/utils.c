/**
 * utils.c - 工具函数实现
 * 
 * 提供清屏、安全输入、彩色文字输出、横幅打印等功能。
 * 兼容 Windows 控制台。
 */

#include "utils.h"

#ifdef _WIN32
#include <windows.h>
#endif

/* ========== 初始化控制台（UTF-8 编码） ========== */
void initConsole(void) {
#ifdef _WIN32
    /* 设置控制台输出代码页为 UTF-8 */
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    /* 启用 ANSI 转义序列支持 */
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }
#else
    /* Linux/Mac 默认 UTF-8，无需操作 */
#endif
}

/* ========== 清屏 ========== */
void clearScreen(void) {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

/* ========== 安全整数输入 ========== */
int getIntInput(const char *prompt, int min, int max) {
    char buffer[64];
    int value;
    int valid = 0;

    while (!valid) {
        printf("%s", prompt);
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            /* 输入流错误 */
            printf(COLOR_RED "输入错误，请重试。\n" COLOR_RESET);
            continue;
        }

        /* 检查是否为空输入 */
        int is_empty = 1;
        for (int i = 0; buffer[i] != '\0'; i++) {
            if (buffer[i] != '\n' && buffer[i] != '\r' && buffer[i] != ' ') {
                is_empty = 0;
                break;
            }
        }
        if (is_empty) {
            printf(COLOR_RED "输入不能为空，请重试。\n" COLOR_RESET);
            continue;
        }

        /* 使用 sscanf 解析整数 */
        if (sscanf(buffer, "%d", &value) == 1) {
            if (value >= min && value <= max) {
                valid = 1;
            } else {
                printf(COLOR_RED "请输入 %d 到 %d 之间的数字。\n" COLOR_RESET, min, max);
            }
        } else {
            printf(COLOR_RED "输入无效，请输入数字。\n" COLOR_RESET);
        }
    }

    return value;
}

/* ========== 彩色输出 ========== */
void printColored(const char *color, const char *text) {
    printf("%s%s%s", color, text, COLOR_RESET);
}

/* ========== 横幅 ========== */
void printBanner(const char *title) {
    int title_len = (int)strlen(title);
    int total_width = 60;
    int padding = (total_width - title_len) / 2;

    printf("\n");
    printf(COLOR_CYAN);
    for (int i = 0; i < total_width; i++) printf("=");
    printf("\n");
    for (int i = 0; i < padding; i++) printf(" ");
    printf("%s%s%s\n", COLOR_BOLD, title, COLOR_RESET COLOR_CYAN);
    for (int i = 0; i < total_width; i++) printf("=");
    printf(COLOR_RESET "\n\n");
}

/* ========== 分隔线 ========== */
void printDivider(char ch, int length) {
    for (int i = 0; i < length; i++) {
        putchar(ch);
    }
    putchar('\n');
}

void printColoredDivider(const char *color, char ch, int length) {
    printf("%s", color);
    for (int i = 0; i < length; i++) {
        putchar(ch);
    }
    printf("%s\n", COLOR_RESET);
}

/* ========== 暂停 ========== */
void pressEnterToContinue(void) {
    printf(COLOR_DIM "\n[按回车键继续...]" COLOR_RESET);
    getchar();
}

/* ========== 打字机效果 ========== */
void typeWriterPrint(const char *text, int delay) {
#ifdef _WIN32
    /* Windows 下使用 Sleep */
    while (*text) {
        putchar(*text++);
        fflush(stdout);
        if (delay > 0) {
            Sleep((DWORD)delay);
        }
    }
#else
    /* Unix 下使用 usleep */
    #include <unistd.h>
    while (*text) {
        putchar(*text++);
        fflush(stdout);
        if (delay > 0) {
            usleep((unsigned int)(delay * 1000));
        }
    }
#endif
}
