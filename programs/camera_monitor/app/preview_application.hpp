#pragma once

#include "camera/camera_session.hpp"

#include <opencv2/core.hpp>

#include <cstddef>
#include <exception>
#include <string>
#include <string_view>

namespace sturdy_guide::camera {

// 主线程 UI 组件：显示画面、叠加帧号与帧率、处理按键（S 存图、Q/关窗退出），
// 并把后台采集错误打印给用户（用户可见错误）。
//
// 与 CameraSession 的分工：本类只通过会话的公共 API（take_latest /
// take_error）消费帧与错误，从不直接触碰会话内部的 mutex 或工作线程；
// 采集由后台线程完成，所有 cv::imshow / cv::waitKey / cv::imwrite 都在
// 本类所在的主线程上、锁外执行。main 负责组装并保证 session 活得比本对象久。
class PreviewApplication {
 public:
  PreviewApplication(CameraSession& session, std::string_view window_name);

  // 主循环：取最新帧并显示，响应按键与窗口关闭；后台采集出错时打印并结束。
  // 正常 / 后台错误退出返回 0；保存截图等操作若失败会向上抛出，由调用方处理。
  int run();

 private:
  void report_capture_error(const std::exception_ptr& error) const;
  void save_screenshot(const cv::Mat& frame);

  CameraSession& session_;
  std::string window_name_;
  std::size_t capture_number_ = 0;  // 截图序号：跨多次 S 按键递增，跳过已存在文件。
};

}  // namespace sturdy_guide::camera
