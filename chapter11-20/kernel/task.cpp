#include "task.hpp"

#include "asmfunc.h"
#include "segment.hpp"
#include "timer.hpp"

namespace {
  template <class T, class U>
  /** @brief 削除する要素を末尾に移動させる */
  void Erase(T& c, const U& value) {
    auto it = std::remove(c.begin(), c.end(), value);
    c.erase(it, c.end());
  }

  void TaskIdle(uint64_t task_id, int64_t data) {
    while (true) __asm__("hlt");
  }
} // namespace

/** @brief Taskクラスのコンストラクタ。指定されたタスクIDをid_に設定する */
Task::Task(uint64_t id) : id_{id}, msgs_{} {
}

// day13a
/**
 * InitContext
 *   タスクのコンテキストを初期化する
 *   stack_やcontext_に値を設定する
 * 
 * @param f    : タスクの開始アドレス（実行する関数ポインタ）
 * @param data : タスクに渡す引数（fの第2引数になる）
 * @return     : Task& 自身への参照
 */
Task& Task::InitContext(TaskFunc* f, int64_t data) {
  const size_t stack_size = kDefaultStackBytes / sizeof(stack_[0]);
  stack_.resize(stack_size);
  uint64_t stack_end = reinterpret_cast<uint64_t>(&stack_[stack_size]);

  memset(&context_, 0, sizeof(context_));
  context_.cr3 = GetCR3();                      // 現在CR3に設定されている値をコピーする（アセンブラ関数）
  context_.rflags = 0x202;                      // TaskXX() を実行する際のRFLAGSの値を指定する。ビット9はIF（割り込みフラグ）でここに1を設定すると割り込みが許可される
  context_.cs = kKernelCS;                      // メイン関数を実行するときと同じセグメントレジスタを設定
  context_.ss = kKernelSS;                      // メイン関数を実行するときと同じセグメントレジスタを設定
  context_.rsp = (stack_end & ~0xflu) - 8;      // スタックポインタの初期値を設定

  context_.rip = reinterpret_cast<uint64_t>(f); // SwitchContext() を最初に実行したとき、ここで設定したアドレスへとジャンプすることになる
  context_.rdi = id_;                           // TaskXX() の引数となる値
  context_.rsi = data;                          // TaskXX() の引数となる値

  // MXCSR のすべての例外をマスクする
  *reinterpret_cast<uint32_t*>(&context_.fxsave_area[24]) = 0x1f80;

  return *this;
}

// day13a
/** 
 * Context
 *   タスクのコンテキスト構造体への参照を返す
 * 
 *   値ではなく参照を返す。なぜなら、コンテキスト構造体のアドレスをSwitchContextに渡す必要があるため
 *   SwitchContext()はコンテキストを切り替える際、そのアドレスが指すメモリ領域にレジスタ値を保存する
 */
TaskContext& Task::Context() {
  return context_;
}

/** @brief タスクのIDを返す */
uint64_t Task::ID() const {
  return id_;
}

Task& Task::Sleep() {
  task_manager->Sleep(this);
  return *this;
}

Task& Task::Wakeup() {
  task_manager->Wakeup(this);
  return *this;
}

// day14b
/** 
 * SendMessage
 *   メッセージキューにメッセージを追加し、寝ていれば起こす
 *   メッセージをキューに追加した後Wakeup()を呼び出すことで、タスクが寝ていた場合に起こす
 */
void Task::SendMessage(const Message& msg) {
  msgs_.push_back(msg);
  Wakeup();
}

/** 
 * ReceiveMessage
 *   メッセージキューからメッセージを1つ取り出す
 */
std::optional<Message> Task::ReceiveMessage() {
  if (msgs_.empty()) {
    return std::nullopt;
  }

  auto m = msgs_.front();
  msgs_.pop_front();
  return m;
}

// day14a
/** 
 * TaskManager
 *   TaskManagerクラスのコンストラクタ
 * 
 *   新しく生成されたタスクの優先度レベルやタスクの状態（実行、実行可能、スリープ）を設定する
 *   コンストラクタを実行し終わった時点で、ランキューにはタスクが1つだけ追加された状態になる
 */
TaskManager::TaskManager() {
  Task& task = NewTask()
    // day14c
    .SetLevel(current_level_)
    .SetRunning(true);
  running_[current_level_].push_back(&task);

  // day14d
  Task& idle = NewTask()
    .InitContext(TaskIdle, 0)
    .SetLevel(0)
    .SetRunning(true);
  running_[0].push_back(&idle);
}

/**
 * NewTask
 *   新しいタスクのインスタンスを生成する
 * 
 *  　最新のタスクIDを表すlatest_id_の値を使ってTaskクラスのインスタンスを生成し、
 *  　tasks_の末尾に追加する
 */
Task& TaskManager::NewTask() {
  ++latest_id_;
  return *tasks_.emplace_back(new Task{latest_id_});
}

// day14c, day13b
/**
 * SwitchTask
 *   現在実行中のタスクとその次のタスクを取得し、次のタスクが持つコンテキストへと実行を切り替える
 *
 *   current_sleep（現在実行中のタスクをスリープさせるかどうか）がFalseの場合のみ、ランキューの末尾にタスクを追加する（Trueならランキューに追加しない）
 *   現在実行中のレベルのランキューが空になったとき（level_changed == True）、
 *   高いレベルのランキューから順に見ていって、タスクが1つ以上登録されているランキューを見つけたらそのレベルをcurrent_level_に設定する
 */
void TaskManager::SwitchTask(bool current_sleep) {
  auto& level_queue = running_[current_level_];
  Task* current_task = level_queue.front();
  level_queue.pop_front();
  if (!current_sleep) {
    level_queue.push_back(current_task);
  }
  if (level_queue.empty()) {
    level_changed_ = true;
  }

  if (level_changed_) {
    level_changed_ = false;
    for (int lv = kMaxLevel; lv >= 0; --lv) {
      if (!running_[lv].empty()) {
        current_level_ = lv;
        break;
      }
    }
  }

  Task* next_task = running_[current_level_].front();
  
  /** @brief アセンブラで定義したレジスタを操作してコンテキストを切り替える関数を呼び出す */
  SwitchContext(&next_task->Context(), &current_task->Context());
}

// day14a
/**
 * Sleep
 *   指定したタスクをスリープさせる
 * 
 *   タスクが実行可能状態かどうかをタスクのrunningフラグによって判定する。実行可能状態であればrunningフラグを下げる
 *   現在実行中のタスク（自分自身）をスリープさせるには、タスクの切り替えをしなければならないため、SwitchTask()を使ってタスクを切り替える
 *   taskが現在進行中のタスクでない（他のタスクをスリープさせる）場合、そのタスクが属するレベルtのランキューからそのタスクを削除する
 */
void TaskManager::Sleep(Task* task) {
  if (!task->Running()) {
    return;
  }

  task->SetRunning(false);

  if (task == running_[current_level_].front()) {
    SwitchTask(true);
    return;
  }

  Erase(running_[task->Level()], task);
}

/** @brief タスクIDで指定できるバージョンのSleep() */
Error TaskManager::Sleep(uint64_t id) {
  auto it = std::find_if(tasks_.begin(), tasks_.end(),
                         [id](const auto& t){ return t->ID() == id; });
  if (it == tasks_.end()) {
    return MAKE_ERROR(Error::kNoSuchTask);
  }

  Sleep(it->get());
  return MAKE_ERROR(Error::kSuccess);
}

/**
 * Wakeup
 *   指定したタスクを起こす（実行可能状態にする）
 * 
 *   指定されたタスクがランキューに存在しなければ（スリープ中であれば）ランキューの末尾に追加する
 *   スリープ中のタスクを起こす（runningフラグを立てる）
 *   動作中タスクのレベルを変える（指定したタスクの現在の状態にかかわらず、指定したレベルで動作させる）
 */
void TaskManager::Wakeup(Task* task, int level) {
  if (task->Running()) {
    ChangeLevelRunning(task, level);
    return;
  }

  if (level < 0) {
    level = task->Level();
  }

  task->SetLevel(level);
  task->SetRunning(true);

  running_[level].push_back(task);
  if (level > current_level_) {
    level_changed_ = true;
  }
  return;
}

/** @brief タスクIDで指定できるバージョンのWakeup() */
Error TaskManager::Wakeup(uint64_t id, int level) {
  auto it = std::find_if(tasks_.begin(), tasks_.end(),
                         [id](const auto& t){ return t->ID() == id; });
  if (it == tasks_.end()) {
    return MAKE_ERROR(Error::kNoSuchTask);
  }

  Wakeup(it->get(), level);
  return MAKE_ERROR(Error::kSuccess);
}

// day14b
/**
 * SendMessage
 *   指定されたタスクをtasks_から探してきて、そのタスクのSendMessage()を呼び出す
 */
Error TaskManager::SendMessage(uint64_t id, const Message& msg) {
  auto it = std::find_if(tasks_.begin(), tasks_.end(),
                         [id](const auto& t){ return t->ID() == id; });
  if (it == tasks_.end()) {
    return MAKE_ERROR(Error::kNoSuchTask);
  }

  (*it)->SendMessage(msg);
  return MAKE_ERROR(Error::kSuccess);
}

// day14b
/** @brief 現在実行中のタスクを返す。すなわち、ランキューの先頭を返す */
Task& TaskManager::CurrentTask() {
  return *running_[current_level_].front();
}

/**
 * ChangeLevelRunning
 *   動作中のタスクのレベルを変える
 * 　
 *   現在実行中のタスクの場合、そのタスクを現在のランキューから取り除き、目的のレベルのランキューに追加する
 *   その後、current_level_を目的のレベルに更新する（SwitchTask()は「current_level_のランキューの先頭にあるタスク」を現在実行中のタスクだと認識する）
 *   他のタスクの実行レベルを変える場合、そのタスクが現在属しているレベルのランキューから削除し、目的のレベルのランキューに追加し直す
 */
void TaskManager::ChangeLevelRunning(Task* task, int level) {
  if (level < 0 || level == task->Level()) {
    return;
  }

  if (task != running_[current_level_].front()) {
    // change level of other task
    Erase(running_[task->Level()], task);
    running_[level].push_back(task);
    task->SetLevel(level);
    if (level > current_level_) {
      level_changed_ = true;
    }
    return;
  }

  // change level myself
  running_[current_level_].pop_front();
  running_[level].push_front(task);
  task->SetLevel(level);
  if (level >= current_level_) {
    current_level_ = level;
  } else {
    current_level_ = level;
    level_changed_ = true;
  }
}

TaskManager* task_manager;

/** @brief TaskManagerのインスタンスを生成し、グローバル変数task_managerに設定 */
void InitializeTask() {
  task_manager = new TaskManager;

  __asm__("cli");
  timer_manager->AddTimer(
      Timer{timer_manager->CurrentTick() + kTaskTimerPeriod, kTaskTimerValue});
  __asm__("sti");
}
