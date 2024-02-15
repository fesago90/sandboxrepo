#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "example/streams.h"
#include "example/transport.h"


namespace streams {

using testing::ElementsAreArray;

TEST(Transport, FillSink) {
  constexpr auto kChunkSize = PacketStreamTraits::kChunkSize;

  DummySink sink;
  BufferQueue buffer_queue;
  Transport<BufferQueue, DummySink> transport(buffer_queue);
  transport.SetSink(sink);

  auto data = MakeBuffer(65, 2 * PacketStreamTraits::kChunkSize + 1);
  buffer_queue.add_buffer(data);

  sink.set_remaining_bytes(kChunkSize - 1);
  transport.maybe_send_packets();
  ASSERT_EQ(sink.packets.size(), 0);

  sink.set_remaining_bytes(kChunkSize);
  transport.maybe_send_packets();
  ASSERT_EQ(sink.packets.size(), 1);

  sink.set_remaining_bytes(kChunkSize - 1);
  transport.maybe_send_packets();
  ASSERT_EQ(sink.packets.size(), 1);

  sink.set_remaining_bytes(kChunkSize);
  transport.maybe_send_packets();
  ASSERT_EQ(sink.packets.size(), 2);

  sink.set_remaining_bytes(1);
  transport.maybe_send_packets();
  ASSERT_EQ(sink.packets.size(), 3);

  std::span<const std::byte> expected_data(data);
  ASSERT_THAT(sink.packets[0].data,
              ElementsAreArray(expected_data.subspan(0, kChunkSize)));
  ASSERT_EQ(sink.packets[0].header.length, kChunkSize);
  ASSERT_EQ(sink.packets[0].header.stream_index, 0);

  ASSERT_THAT(sink.packets[1].data,
              ElementsAreArray(expected_data.subspan(kChunkSize, kChunkSize)));
  ASSERT_EQ(sink.packets[1].header.length, kChunkSize);
  ASSERT_EQ(sink.packets[1].header.stream_index,
            PacketStreamTraits::kChunkSize);

  ASSERT_THAT(sink.packets[2].data,
              ElementsAreArray(expected_data.subspan(2 * kChunkSize, 1)));
  ASSERT_EQ(sink.packets[2].header.length, 1);
  ASSERT_EQ(sink.packets[2].header.stream_index,
            2 * PacketStreamTraits::kChunkSize);
}
}  // namespace streams