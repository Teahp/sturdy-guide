#pragma once

#include <string>

namespace sturdy_guide {

// 根据装甲板编号返回颜色名：0 为蓝色，1 为红色。
// 其他取值返回 "Unknown"。
std::string armor_color_name(int armor_id);

}  // namespace sturdy_guide