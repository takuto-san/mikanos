/**
 * @file task.hpp
 *
 * タスク管理，コンテキスト切り替えのプログラムを集めたファイル。
 */

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <optional>
#include <vector>

#include "error.hpp"
#include "message.hpp"

// day13a
/**
 * TaskContext
 *   コンテキストを保存するための構造体
 *   コンテキストを切り替えるときに値の保存と復帰が必要なレジスタを全て含んでいる
 */
struct TaskContext {
  uint64_t cr3, rip, rflags, reserved1; // offset 0x00
  uint64_t cs, ss, fs, gs; // offset 0x20
  uint64_t rax, rbx, rcx, rdx, rdi, rsi, rsp, rbp; // offset 0x40
  uint64_t r8, r9, r10, r11, r12, r13, r14, r15; // offset 0x80
  std::array<uint8_t, 512> fxsave_area; // offset 0xc0
} __attribute__((packed));

using TaskFunc = void (uint64_t, int64_t);


class TaskManager;

// day13d
/**
 * Task
 *   1つのタスクを表す
 *   タスクは固有のIDとスタック領域、コンテキスト構造体を持つ
 */
class Task {
 public:
  static const int kDefaultLevel = 1;
  static const size_t kDefaultStackBytes = 4096; //タスク用スタックの大きさ

  Task(uint64_t id);
  Task& InitContext(TaskFunc* f, int64_t data);
  TaskContext& Context();
  uint64_t ID() const;
  Task& Sleep();
  Task& Wakeup();
  // day14b
  void SendMessage(const Message& msg);
  std::optional<Message> ReceiveMessage();

  int Level() const { return level_; }
  bool Running() const { return running_; }

 private:
  uint64_t id_;
  std::vector<uint64_t> stack_;
  alignas(16) TaskContext context_;
  // day14b
  /** @brief 
   * main.cppのmsg_queueに割り込み関連のメッセージが格納されていたのを、メッセージキューmsgs_を付け足したことで、
   * メインタスクだけでなく全てのタスクにmain_queueと同様の機能が備わることになった
   */
  std::deque<Message> msgs_;
  unsigned int level_{kDefaultLevel};
  bool running_{false};

  // day14c
  /** @brief level: そのタスクの現在のレベルを表す running_: タスクが実行状態または実行可能状態であれば真となる */
  Task& SetLevel(int level) { level_ = level; return *this; }
  Task& SetRunning(bool running) { running_ = running; return *this; }

  friend TaskManager;
};

// day13d
/**
 * TaskManager
 *   複数のタスクを管理する
 * 
 *   Taskの生成と生成したTaskのインスタンスを保存しておく役目がある
 *   タスク切り替えの機能（SwitchTask()）も持たせる
 */
class TaskManager {
 public:
  // level: 0 = lowest, kMaxLevel = highest
  static const int kMaxLevel = 3;

  TaskManager();
  Task& NewTask();
  void SwitchTask(bool current_sleep = false);

  // day14a
  void Sleep(Task* task);
  Error Sleep(uint64_t id);
  void Wakeup(Task* task, int level = -1);
  Error Wakeup(uint64_t id, int level = -1);
  // day14b
  Error SendMessage(uint64_t id, const Message& msg);
  Task& CurrentTask();

 private:
  std::vector<std::unique_ptr<Task>> tasks_{};
  uint64_t latest_id_{0};
  // day14c
  std::array<std::deque<Task*>, kMaxLevel + 1> running_{};
  int current_level_{kMaxLevel};
  bool level_changed_{false};

  void ChangeLevelRunning(Task* task, int level);
};

extern TaskManager* task_manager;

void InitializeTask();
