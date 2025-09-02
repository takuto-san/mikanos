// day11b
#include "timer.hpp"

#include "acpi.hpp"
#include "interrupt.hpp"
#include "task.hpp"

namespace {
  /** @brief 定数とメモリマップドレジスタの定義 */
  const uint32_t kCountMax = 0xffffffffu;
  volatile uint32_t& lvt_timer = *reinterpret_cast<uint32_t*>(0xfee00320);
  volatile uint32_t& initial_count = *reinterpret_cast<uint32_t*>(0xfee00380);
  volatile uint32_t& current_count = *reinterpret_cast<uint32_t*>(0xfee00390);
  volatile uint32_t& divide_config = *reinterpret_cast<uint32_t*>(0xfee003e0);
}

// day12b, day11d, day11c
/**
 * InitializeLAPICTimer
 *   ローカルAPICタイマを初期化する
 *
 *   1秒間隔でタイムアウトする
 *   割り込みベクタは InterruptVector::kLAPICTimer を使用する
 *   initial_countはInitial Countレジスタに設定する値
 */
void InitializeLAPICTimer() {
  timer_manager = new TimerManager;

  divide_config = 0b1011; // divide 1:1
  lvt_timer = 0b001 << 16; // masked, one-shot

  StartLAPICTimer();
  acpi::WaitMilliseconds(100);
  const auto elapsed = LAPICTimerElapsed();
  StopLAPICTimer();

  lapic_timer_freq = static_cast<unsigned long>(elapsed) * 10;

  divide_config = 0b1011; // divide 1:1
  lvt_timer = (0b010 << 16) | InterruptVector::kLAPICTimer; // not-masked, periodic
  initial_count = lapic_timer_freq / kTimerFreq;
}

/**
 * StartLAPICTimer
 *   ローカルAPICタイマを開始する
 *   initial_countに最大値を設定することでカウンタを動作させる
 */
void StartLAPICTimer() {
  initial_count = kCountMax;
}

/**
 * LAPICTimerElapsed
 *   タイマ開始からの経過時間を取得する
 *
 * @return kCountMaxから現在のカウント値を引いた値（経過時間）
 */
uint32_t LAPICTimerElapsed() {
  return kCountMax - current_count;
}

/**
 * StopLAPICTimer
 *   ローカルAPICタイマを停止する
 *   initial_countに0を設定することでカウンタを停止させる
 */
void StopLAPICTimer() {
  initial_count = 0;
}

// day11d
/**
 * Timer 
 *   Timerクラスのコンストラクタ
 */
Timer::Timer(unsigned long timeout, int value)
    : timeout_{timeout}, value_{value} {
}

/**
 * TimerManager 
 *   TimerManagerクラスのコンストラクタ
 */
TimerManager::TimerManager() {
  timers_.push(Timer{std::numeric_limits<unsigned long>::max(), -1});
}

/**
 * AddTimer 
 *   指定されたタイマをtimers_に追加する
 */
void TimerManager::AddTimer(const Timer& timer) {
  timers_.push(timer);
}

// day11c, day11d
/**
 * Tick
 *   割り込み回数を1だけ増やす
 *   タイムアウト処理を行う
 * 
 * @return タスク切り替え用タイマがタイムアウトした場合はtrueを返す
 */
bool TimerManager::Tick() {
  ++tick_;

  bool task_timer_timeout = false;
  while (true) {
    const auto& t = timers_.top();
    if (t.Timeout() > tick_) {
      break;
    }

    if (t.Value() == kTaskTimerValue) {
      task_timer_timeout = true;
      timers_.pop();
      timers_.push(Timer{tick_ + kTaskTimerPeriod, kTaskTimerValue});
      continue;
    }

    Message m{Message::kTimerTimeout};
    m.arg.timer.timeout = t.Timeout();
    m.arg.timer.value = t.Value();
    // day14b
    task_manager->SendMessage(1, m);

    timers_.pop();
  }

  return task_timer_timeout;
}

/** @brief タイマー管理クラスのグローバルインスタンス */
TimerManager* timer_manager;
unsigned long lapic_timer_freq;

/**
 * LAPICTimerOnInterrupt
 *   タイマ割り込みのハンドラの中からTick()を呼び出す
 *   タスク切り替えがタイムアウトならSwitchTask()を呼び出す
 */
void LAPICTimerOnInterrupt() {
  const bool task_timer_timeout = timer_manager->Tick();
  NotifyEndOfInterrupt();

  if (task_timer_timeout) {
    task_manager->SwitchTask();
  }
}
