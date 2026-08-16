#include <cmath>
#include <cstdio>
#include <cstring>
#include <unistd.h>

const int W = 80;
const int H = 24;
char screen[W * H];

const float cube[8][3] = {
    {-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1},
    {-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1}
};

const int edges[12][2] = {
    {0,1},{1,2},{2,3},{3,0},
    {4,5},{5,6},{6,7},{7,4},
    {0,4},{1,5},{2,6},{3,7}
};

void drawLine(float x0, float y0, float x1, float y1, char c) {
    int steps = std::max(std::abs(x1-x0), std::abs(y1-y0));
    for (int i = 0; i <= steps; ++i) {
        float t = steps == 0 ? 0 : (float)i / steps;
        int x = (int)(x0 + (x1-x0) * t + 0.5f);
        int y = (int)(y0 + (y1-y0) * t + 0.5f);
        if (x >= 0 && x < W && y >= 0 && y < H) {
            screen[y * W + x] = c;
        }
    }
}

int main() {
    float A = 0, B = 0;
    printf("\033[2J\033[?25l");   // 清屏 + 隐藏光标
    while (true) {
        memset(screen, ' ', sizeof(screen));

        for (int e = 0; e < 12; ++e) {
            int p1 = edges[e][0], p2 = edges[e][1];

            // 取顶点坐标
            float x1 = cube[p1][0], y1 = cube[p1][1], z1 = cube[p1][2];
            float x2 = cube[p2][0], y2 = cube[p2][1], z2 = cube[p2][2];

            // 绕 Y 轴旋转 A
            float cosA = cos(A), sinA = sin(A);
            float rx1 = x1 * cosA - z1 * sinA, rz1 = x1 * sinA + z1 * cosA;
            float rx2 = x2 * cosA - z2 * sinA, rz2 = x2 * sinA + z2 * cosA;

            // 绕 X 轴旋转 B
            float cosB = cos(B), sinB = sin(B);
            float ry1 = y1 * cosB - rz1 * sinB, rz1f = y1 * sinB + rz1 * cosB;
            float ry2 = y2 * cosB - rz2 * sinB, rz2f = y2 * sinB + rz2 * cosB;

            // 透视投影（简单缩放 + 居中），只在 z > 0 时绘制（正面）
            if (rz1f > 0 && rz2f > 0) {
                float scale = 10.0f;
                float px1 = rx1 * scale + W / 2.0f;
                float py1 = ry1 * scale + H / 2.0f;
                float px2 = rx2 * scale + W / 2.0f;
                float py2 = ry2 * scale + H / 2.0f;
                drawLine(px1, py1, px2, py2, '#');
            }
        }

        // 移动光标到左上角并输出整个帧
        printf("\033[H");
        for (int i = 0; i < H; ++i) {
            fwrite(screen + i * W, 1, W, stdout);
            putchar('\n');
        }
        fflush(stdout);

        A += 0.04f;
        B += 0.02f;
        usleep(50000);   // 约 20 FPS
    }
    return 0;
}
//测试git
