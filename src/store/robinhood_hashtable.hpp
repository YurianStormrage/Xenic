#pragma once

#include <cstdint>
#include <list>
#include <optional>
#include <unordered_map>
#include <vector>

namespace store {

template <typename Key, typename Value, typename Hash = std::hash<Key>>
class RobinhoodHashTable {
public:
  using HashValue = typename std::invoke_result<Hash(Key)>;

private:
  using size_t = std::size_t;

  struct Entry {
    HashValue h_initial; // 初始哈希位置
    uint8_t distance;    // 距离初始位置的位移
    bool occupied;
    Key key;
    Value value;

    Entry() : occupied(false) {}
    Entry(Key &&k, Value &&v, HashValue h, uint8_t d)
        : key(std::move(k)), value(std::move(v)), h_initial(h), distance(d),
          occupied(true) {}
  };

  size_t capacity_;
  size_t Dm_;
  std::vector<Entry> table_;
  std::unordered_map<int, std::list<Entry>> overflow_buckets_;
  Hash hash_fn_;

  // TODO: 处理大对象存储
  auto store_value(Value&& value) {
    return new Value(value); // 实际应使用智能指针并检查大小
  }

public:
  RobinhoodHashTable(int capacity, int Dm)
      : capacity_(capacity), Dm_(Dm), table_(capacity) {}

  void Put(const Key& key, Value&& value) {
    int h = hash_fn_(key) % capacity_;
    Entry current(key, value, h, 0);

    while (true) {
      if (current.distance > Dm_) {
        overflow_buckets_[h].push_back(current);
        return;
      }

      int index = (h + current.distance) % capacity_;
      if (!table_[index].occupied) {
        table_[index] = current;
        return;
      }

      Entry &existing = table_[index];
      if (existing.distance < current.distance) {
        transactional_swap(existing, current);
        h = current.h_initial;
        current.distance += 1;
      } else {
        current.distance += 1;
      }
    }
  }

  auto Get(const Key& key) -> std::optional<Value> {
    int h = hash_fn_(key) % capacity_;

    // 主表查找
    for (int d = 0; d <= Dm_; ++d) {
      int index = (h + d) % capacity_;
      if (table_[index].occupied && table_[index].key == key) {
        return table_[index].value;
      }
    }

    // 溢出桶查找
    if (overflow_buckets_.count(h)) {
      for (auto &entry : overflow_buckets_[h]) {
        if (entry.key == key) {
          return entry.value;
        }
      }
    }

    return {};
  }

  void Remove(const Key& key) {
    int h = hash_fn_(key) % capacity_;

    // 主表删除
    for (int d = 0; d <= Dm_; ++d) {
      int index = (h + d) % capacity_;
      if (table_[index].occupied && table_[index].key == key) {
        // 优先使用溢出桶元素填充
        if (!overflow_buckets_[h].empty()) {
          table_[index] = overflow_buckets_[h].front();
          overflow_buckets_[h].pop_front();
          table_[index].distance = index - table_[index].h_initial;
        } else {
          // 向后移动填充空位
          int current = index;
          while (true) {
            int next = (current + 1) % capacity_;
            if (next == (h + Dm_ + 1) % capacity_ || !table_[next].occupied)
              break;

            if (table_[next].distance > 0) {
              table_[current] = table_[next];
              table_[current].distance--;
              current = next;
            } else {
              break;
            }
          }
          table_[current].occupied = false;
        }
        return;
      }
    }

    // 溢出桶删除
    if (overflow_buckets_.count(h)) {
      auto &bucket = overflow_buckets_[h];
      for (auto it = bucket.begin(); it != bucket.end(); ++it) {
        if (it->key == key) {
          bucket.erase(it);
          return;
        }
      }
    }
  }

  // TODO: 事务内存操作（需硬件支持）
  __attribute__((transaction_safe)) void transactional_swap(Entry &a,
                                                            Entry &b) {
    std::swap(a, b);
  }
};

RobinhoodHashTable<int, int> table(100, 10); // debug

} // namespace store