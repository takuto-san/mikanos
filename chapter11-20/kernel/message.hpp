#pragma once

enum class LayerOperation {
  Move, MoveRelative, Draw, DrawArea
};

// day11d, day11b
/**
 * Message
 *   タスク間通信で用いるメッセージの構造体
 *
 *   タイマーのタイムアウト、キー入力、レイヤー操作など、
 *   OS内から送られてくる割り込み関連のメッセージを保持する
 *   `type` メンバーでメッセージの種類を識別する
 */
struct Message {
  enum Type {
    kInterruptXHCI,
    kTimerTimeout,
    kKeyPush,
    kLayer,
    kLayerFinish,
  } type;

  uint64_t src_task;

  // day11d
  union {
    struct {
      unsigned long timeout;
      int value;
    } timer;

    struct {
      uint8_t modifier;
      uint8_t keycode;
      char ascii;
    } keyboard;

    struct {
      LayerOperation op;
      unsigned int layer_id;
      int x, y;
      int w, h;
    } layer;
  } arg;
};
