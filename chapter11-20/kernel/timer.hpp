// day11b
#pragma once

#include <cstdint>
// day11d
#include <queue>
#include <vector>
#include <limits>
#include "message.hpp"

// day11d
void InitializeLAPICTimer();
void StartLAPICTimer();
uint32_t LAPICTimerElapsed();
void StopLAPICTimer();

// day11d
/**
 * Timer
 *   論理的なタイマを表す
 *   timeout変数はタイムアウト時刻を表し、value_変数はタイムアウト時に送信する値を格納する
 */
class Timer {
 public:
  Timer(unsigned long timeout, int value);
  unsigned long Timeout() const { return timeout_; }
  int Value() const { return value_; }

 private:
  unsigned long timeout_;
  int value_;
};

/** @brief タイマー優先度を比較する。タイムアウトが遠いほど優先度低。 */
inline bool operator<(const Timer& lhs, const Timer& rhs) {
  return lhs.Timeout() > rhs.Timeout();
}

// day11c
/**
 * TimerManager
 *   Local APICタイマ割り込みの回数を数える
 * 
 *   CurrentTick()は現在の割り込み回数を返す
 *   timers_変数は複数の論理タイマを保持し、msg_queue_参照変数はタイマのタイムアウトを通知するためのメッセージキューを指す
 */
class TimerManager {
 public:
  TimerManager();
  void AddTimer(const Timer& timer);
  bool Tick();
  unsigned long CurrentTick() const { return tick_; }

 private:
  volatile unsigned long tick_{0};
  std::priority_queue<Timer> timers_{};
};

// day12a
/** @brief Local APICタイマの1カウントの時間を計り、その結果を記憶しておくためのグローバル変数 */
extern TimerManager* timer_manager;
extern unsigned long lapic_timer_freq;
const int kTimerFreq = 100;

const int kTaskTimerPeriod = static_cast<int>(kTimerFreq * 0.02);
const int kTaskTimerValue = std::numeric_limits<int>::min();

void LAPICTimerOnInterrupt();
