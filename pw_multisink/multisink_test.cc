// Copyright 2021 The Pigweed Authors
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License. You may obtain a copy of
// the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations under
// the License.

#include "pw_multisink/multisink.h"

#include <array>
#include <cstdint>
#include <optional>

#include "gtest/gtest.h"
#include "pw_status/status.h"

namespace pw::multisink {
using Drain = MultiSink::Drain;
using Listener = MultiSink::Listener;

class CountingListener : public Listener {
 public:
  void OnNewEntryAvailable() override { notification_count_++; }

  size_t GetNotificationCount() { return notification_count_; }

  void ResetNotificationCount() { notification_count_ = 0; }

 private:
  size_t notification_count_ = 0;
};

class MultiSinkTest : public ::testing::Test {
 protected:
  static constexpr std::byte kMessage[] = {
      (std::byte)0xDE, (std::byte)0xAD, (std::byte)0xBE, (std::byte)0xEF};
  static constexpr std::byte kMessageOther[] = {
      (std::byte)0x12, (std::byte)0x34, (std::byte)0x56, (std::byte)0x78};
  static constexpr size_t kMaxDrains = 3;
  static constexpr size_t kMaxListeners = 3;
  static constexpr size_t kEntryBufferSize = 1024;
  static constexpr size_t kBufferSize = 5 * kEntryBufferSize;

  MultiSinkTest() : multisink_(buffer_) {}

  // Expects the peeked or popped message to equal the provided non-empty
  // message, and the drop count to match. If `expected_message` is empty, the
  // Pop call status expected is OUT_OF_RANGE.
  void ExpectMessageAndDropCount(Result<ConstByteSpan>& result,
                                 uint32_t result_drop_count,
                                 std::optional<ConstByteSpan> expected_message,
                                 uint32_t expected_drop_count) {
    if (!expected_message.has_value()) {
      EXPECT_EQ(Status::OutOfRange(), result.status());
    } else {
      ASSERT_EQ(result.status(), OkStatus());
      if (!expected_message.value().empty()) {
        ASSERT_FALSE(result.value().empty());
        ASSERT_EQ(result.value().size_bytes(),
                  expected_message.value().size_bytes());
        EXPECT_EQ(memcmp(result.value().data(),
                         expected_message.value().data(),
                         expected_message.value().size_bytes()),
                  0);
      }
    }
    EXPECT_EQ(result_drop_count, expected_drop_count);
  }

  void VerifyPopEntry(Drain& drain,
                      std::optional<ConstByteSpan> expected_message,
                      uint32_t expected_drop_count) {
    uint32_t drop_count = 0;
    Result<ConstByteSpan> result = drain.PopEntry(entry_buffer_, drop_count);
    ExpectMessageAndDropCount(
        result, drop_count, expected_message, expected_drop_count);
  }

  void VerifyPeekResult(const Result<Drain::PeekedEntry>& peek_result,
                        uint32_t result_drop_count,
                        std::optional<ConstByteSpan> expected_message,
                        uint32_t expected_drop_count) {
    if (peek_result.ok()) {
      ASSERT_FALSE(peek_result.value().entry().empty());
      Result<ConstByteSpan> verify_result(peek_result.value().entry());
      ExpectMessageAndDropCount(verify_result,
                                result_drop_count,
                                expected_message,
                                expected_drop_count);
      return;
    }
    if (expected_message.has_value()) {
      // Fail since we expected OkStatus.
      ASSERT_EQ(peek_result.status(), OkStatus());
    }
    EXPECT_EQ(Status::OutOfRange(), peek_result.status());
  }

  void ExpectNotificationCount(CountingListener& listener,
                               size_t expected_notification_count) {
    EXPECT_EQ(listener.GetNotificationCount(), expected_notification_count);
    listener.ResetNotificationCount();
  }

  std::byte buffer_[kBufferSize];
  std::byte entry_buffer_[kEntryBufferSize];
  CountingListener listeners_[kMaxListeners];
  Drain drains_[kMaxDrains];
  MultiSink multisink_;
};

TEST_F(MultiSinkTest, SingleDrain) {
  multisink_.AttachDrain(drains_[0]);
  multisink_.AttachListener(listeners_[0]);
  multisink_.HandleEntry(kMessage);

  // Single entry push and pop.
  ExpectNotificationCount(listeners_[0], 1u);
  VerifyPopEntry(drains_[0], kMessage, 0u);

  // Single empty entry push and pop.
  multisink_.HandleEntry(ConstByteSpan());
  ExpectNotificationCount(listeners_[0], 1u);
  VerifyPopEntry(drains_[0], ConstByteSpan(), 0u);

  // Multiple entries with intermittent drops.
  multisink_.HandleEntry(kMessage);
  multisink_.HandleDropped();
  multisink_.HandleEntry(kMessage);
  ExpectNotificationCount(listeners_[0], 3u);
  VerifyPopEntry(drains_[0], kMessage, 0u);
  VerifyPopEntry(drains_[0], kMessage, 1u);

  // Send drops only.
  multisink_.HandleDropped();
  ExpectNotificationCount(listeners_[0], 1u);
  VerifyPopEntry(drains_[0], std::nullopt, 1u);

  // Confirm out-of-range if no entries are expected.
  ExpectNotificationCount(listeners_[0], 0u);
  VerifyPopEntry(drains_[0], std::nullopt, 0u);
}

TEST_F(MultiSinkTest, MultipleDrain) {
  multisink_.AttachDrain(drains_[0]);
  multisink_.AttachDrain(drains_[1]);
  multisink_.AttachListener(listeners_[0]);
  multisink_.AttachListener(listeners_[1]);

  multisink_.HandleEntry(kMessage);
  multisink_.HandleEntry(kMessage);
  multisink_.HandleDropped();
  multisink_.HandleEntry(kMessage);
  multisink_.HandleDropped();

  // Drain one drain entirely.
  ExpectNotificationCount(listeners_[0], 5u);
  ExpectNotificationCount(listeners_[1], 5u);
  VerifyPopEntry(drains_[0], kMessage, 0u);
  VerifyPopEntry(drains_[0], kMessage, 0u);
  VerifyPopEntry(drains_[0], kMessage, 1u);
  VerifyPopEntry(drains_[0], std::nullopt, 1u);
  VerifyPopEntry(drains_[0], std::nullopt, 0u);

  // Confirm the other drain can be drained separately.
  ExpectNotificationCount(listeners_[0], 0u);
  ExpectNotificationCount(listeners_[1], 0u);
  VerifyPopEntry(drains_[1], kMessage, 0u);
  VerifyPopEntry(drains_[1], kMessage, 0u);
  VerifyPopEntry(drains_[1], kMessage, 1u);
  VerifyPopEntry(drains_[1], std::nullopt, 1u);
  VerifyPopEntry(drains_[1], std::nullopt, 0u);
}

TEST_F(MultiSinkTest, LateDrainRegistration) {
  // Drains attached after entries are pushed should still observe those entries
  // if they have not been evicted from the ring buffer.
  multisink_.HandleEntry(kMessage);

  multisink_.AttachDrain(drains_[0]);
  multisink_.AttachListener(listeners_[0]);
  ExpectNotificationCount(listeners_[0], 0u);
  VerifyPopEntry(drains_[0], kMessage, 0u);
  VerifyPopEntry(drains_[0], std::nullopt, 0u);

  multisink_.HandleEntry(kMessage);
  ExpectNotificationCount(listeners_[0], 1u);
  VerifyPopEntry(drains_[0], kMessage, 0u);
  VerifyPopEntry(drains_[0], std::nullopt, 0u);
}

TEST_F(MultiSinkTest, DynamicDrainRegistration) {
  multisink_.AttachDrain(drains_[0]);
  multisink_.AttachListener(listeners_[0]);

  multisink_.HandleDropped();
  multisink_.HandleEntry(kMessage);
  multisink_.HandleDropped();
  multisink_.HandleEntry(kMessage);

  // Drain out one message and detach it.
  ExpectNotificationCount(listeners_[0], 4u);
  VerifyPopEntry(drains_[0], kMessage, 1u);
  multisink_.DetachDrain(drains_[0]);
  multisink_.DetachListener(listeners_[0]);

  // Re-attaching the drain should reproduce the last observed message. Note
  // that notifications are not expected, nor are drops observed before the
  // first valid message in the buffer.
  multisink_.AttachDrain(drains_[0]);
  multisink_.AttachListener(listeners_[0]);
  ExpectNotificationCount(listeners_[0], 0u);
  VerifyPopEntry(drains_[0], kMessage, 1u);
  VerifyPopEntry(drains_[0], kMessage, 1u);
  VerifyPopEntry(drains_[0], std::nullopt, 0u);

  multisink_.HandleEntry(kMessage);
  ExpectNotificationCount(listeners_[0], 1u);
  VerifyPopEntry(drains_[0], kMessage, 0u);
  VerifyPopEntry(drains_[0], std::nullopt, 0u);
}

TEST_F(MultiSinkTest, TooSmallBuffer) {
  multisink_.AttachDrain(drains_[0]);

  // Insert an entry and a drop, then try to read into an insufficient buffer.
  uint32_t drop_count = 0;
  multisink_.HandleDropped();
  multisink_.HandleEntry(kMessage);

  // Attempting to acquire an entry with a small buffer should result in
  // RESOURCE_EXHAUSTED and remove it.
  Result<ConstByteSpan> result =
      drains_[0].PopEntry(std::span(entry_buffer_, 1), drop_count);
  EXPECT_EQ(result.status(), Status::ResourceExhausted());

  VerifyPopEntry(drains_[0], std::nullopt, 2u);
}

TEST_F(MultiSinkTest, Iterator) {
  multisink_.AttachDrain(drains_[0]);

  // Insert entries and consume them all.
  multisink_.HandleEntry(kMessage);
  multisink_.HandleEntry(kMessage);
  multisink_.HandleEntry(kMessage);

  VerifyPopEntry(drains_[0], kMessage, 0u);
  VerifyPopEntry(drains_[0], kMessage, 0u);
  VerifyPopEntry(drains_[0], kMessage, 0u);

  // Confirm that the iterator still observes the messages in the ring buffer.
  size_t iterated_entries = 0;
  for (ConstByteSpan entry : multisink_.UnsafeIteration()) {
    EXPECT_EQ(memcmp(entry.data(), kMessage, sizeof(kMessage)), 0);
    iterated_entries++;
  }
  EXPECT_EQ(iterated_entries, 3u);
}

TEST_F(MultiSinkTest, IteratorNoDrains) {
  // Insert entries with no drains attached. Even though there are no consumers,
  // iterators should still walk from the oldest entry.
  multisink_.HandleEntry(kMessage);
  multisink_.HandleEntry(kMessage);
  multisink_.HandleEntry(kMessage);

  // Confirm that the iterator still observes the messages in the ring buffer.
  size_t iterated_entries = 0;
  for (ConstByteSpan entry : multisink_.UnsafeIteration()) {
    EXPECT_EQ(memcmp(entry.data(), kMessage, sizeof(kMessage)), 0);
    iterated_entries++;
  }
  EXPECT_EQ(iterated_entries, 3u);
}

TEST_F(MultiSinkTest, IteratorNoEntries) {
  // Attach a drain, but don't add any entries.
  multisink_.AttachDrain(drains_[0]);
  // Confirm that the iterator has no entries.
  MultiSink::UnsafeIterationWrapper unsafe_iterator =
      multisink_.UnsafeIteration();
  EXPECT_EQ(unsafe_iterator.begin(), unsafe_iterator.end());
}

TEST_F(MultiSinkTest, PeekEntryNoEntries) {
  multisink_.AttachDrain(drains_[0]);

  // Peek empty multisink.
  uint32_t drop_count = 0;
  auto peek_result = drains_[0].PeekEntry(entry_buffer_, drop_count);
  VerifyPeekResult(peek_result, drop_count, std::nullopt, 0);
}

TEST_F(MultiSinkTest, PeekAndPop) {
  multisink_.AttachDrain(drains_[0]);
  multisink_.AttachDrain(drains_[1]);

  // Peek entry after multisink has some entries.
  multisink_.HandleEntry(kMessage);
  multisink_.HandleEntry(kMessageOther);
  uint32_t drop_count = 0;
  auto first_peek_result = drains_[0].PeekEntry(entry_buffer_, drop_count);
  VerifyPeekResult(first_peek_result, drop_count, kMessage, 0);

  // Multiple peeks must return the front message.
  auto peek_duplicate = drains_[0].PeekEntry(entry_buffer_, drop_count);
  VerifyPeekResult(peek_duplicate, drop_count, kMessage, 0);
  // A second drain must peek the front message.
  auto peek_other_drain = drains_[1].PeekEntry(entry_buffer_, drop_count);
  VerifyPeekResult(peek_other_drain, drop_count, kMessage, 0);

  // After a drain pops a peeked entry, the next peek call must return the next
  // message.
  ASSERT_EQ(drains_[0].PopEntry(first_peek_result.value()), OkStatus());
  auto second_peek_result = drains_[0].PeekEntry(entry_buffer_, drop_count);
  VerifyPeekResult(second_peek_result, drop_count, kMessageOther, 0);
  // Slower readers must be unchanged.
  auto peek_other_drain_duplicate =
      drains_[1].PeekEntry(entry_buffer_, drop_count);
  VerifyPeekResult(peek_other_drain_duplicate, drop_count, kMessage, 0);

  // PopEntry prior to popping the previously peeked entry.
  VerifyPopEntry(drains_[0], kMessageOther, 0);
  // Popping an entry already handled must not trigger errors.
  ASSERT_EQ(drains_[0].PopEntry(second_peek_result.value()), OkStatus());
  // Popping with an old peek context must not trigger errors.
  ASSERT_EQ(drains_[0].PopEntry(first_peek_result.value()), OkStatus());

  // Multisink is empty, pops and peeks should trigger OUT_OF_RANGE.
  VerifyPopEntry(drains_[0], std::nullopt, 0);
  auto empty_peek_result = drains_[0].PeekEntry(entry_buffer_, drop_count);
  VerifyPeekResult(empty_peek_result, drop_count, std::nullopt, 0);

  // // Slower readers must be unchanged.
  auto peek_other_drain_unchanged =
      drains_[1].PeekEntry(entry_buffer_, drop_count);
  VerifyPeekResult(peek_other_drain_unchanged, drop_count, kMessage, 0);
}

}  // namespace pw::multisink
