#include "../src/greeting_plus.h"
#include <iostream>

int main() {
    // 调用你 src 里的函数，自己写个简单的判断
    if (add_plus(2, 3) == 5) {
        std::cout << "greeting_plus test passed!" << std::endl;
        return 0; // 返回0代表成功
    } else {
        std::cout << "greeting_plus test failed!" << std::endl;
        return 1; // 返回1代表失败
    }
}
