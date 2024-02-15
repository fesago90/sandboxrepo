
#include <benchmark/benchmark.h>

#include "example/streams.h"
#include "example/transport.h"

using namespace streams;

static void BM_BufferQueue(benchmark::State& state) {
  BufferQueue buffer_queue;
  for (int i = 0; i < 10; i++) {
    buffer_queue.add_buffer(
        MakeBuffer(0, PacketStream<BufferQueue>::Traits::kChunkSize));
  }

  std::uint64_t total_bytes = 0;
  for (auto _ : state) {
    auto buffers = buffer_queue.get_buffers();
    for (auto buffer : buffers) {
      total_bytes += buffer.size();
      benchmark::DoNotOptimize(total_bytes);
      benchmark::ClobberMemory();
    }
  }
  // fmt::println("total_bytes={}", total_bytes);
}
BENCHMARK(BM_BufferQueue);

static void BM_PacketStream(benchmark::State& state) {
  constexpr auto kChunkSize = PacketStream<BufferQueue>::Traits::kChunkSize;
  constexpr auto kNumPackets = 100;
  // fmt::println("kChunkSize={}", kChunkSize);

  BufferQueue buffer_queue;
  for (int i = 0; i < kNumPackets; i++) {
    buffer_queue.add_buffer(MakeBuffer(0, kChunkSize));
  }

  PacketStream<BufferQueue> packet_stream;
  packet_stream.set_buf_stream(buffer_queue);

  std::uint64_t total_bytes = 0;
  std::uint64_t num_packets = 0;
  for (auto _ : state) {
    PacketStream<BufferQueue> packet_stream;
    packet_stream.set_buf_stream(buffer_queue);
    auto packets = packet_stream.get_packets();
    for (auto p : packets) {
      total_bytes += p.data.size();
      ++num_packets;
      benchmark::DoNotOptimize(total_bytes);
      benchmark::ClobberMemory();
    }
  }
  // fmt::println("total_bytes={}, num_packets={}", total_bytes, num_packets);
}
BENCHMARK(BM_PacketStream);

static void BM_Transport(benchmark::State& state) {
  constexpr auto kChunkSize = PacketStream<BufferQueue>::Traits::kChunkSize;
  constexpr auto kNumPackets = 1;
  // fmt::println("kChunkSize={}", kChunkSize);

  BufferQueue buffer_queue;
  for (int i = 0; i < kNumPackets; i++) {
    buffer_queue.add_buffer(MakeBuffer(0, kChunkSize));
  }

  DummySink dummy_sink;

  // std::uint64_t total_bytes = 0;
  std::uint64_t num_packets = 0;
  for (auto _ : state) {
    Transport<BufferQueue, DummySink> transport(buffer_queue);
    transport.SetSink(dummy_sink);

    num_packets += transport.maybe_send_packets(kNumPackets);
    benchmark::DoNotOptimize(num_packets);
    benchmark::ClobberMemory();
  }

  // fmt::println("num_packets={}", num_packets);
}
BENCHMARK(BM_Transport);