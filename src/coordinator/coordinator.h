#pragma once

#include <cstdint>
#include <vector>

class Coordinator {
  // TODO: 修改Object的定义
  using Object = std::pair<uint64_t, uint64_t>;

public:
  /**
   * @brief 生成并执行事务
   * @param read_set
   * @param write_set
   * @return auto
   */
  void HandleTransaction(std::vector<Object> read_set,
                         std::vector<Object> write_set);

private:
  /**
   * 用于生成 transaction id
   */
  uint64_t node_id_;
  uint64_t sequence_;
};
