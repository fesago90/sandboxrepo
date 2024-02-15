#ifndef EXAMPLE_STREAMS_H_
#define EXAMPLE_STREAMS_H_

#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/ranges.h>

#include <algorithm>
#include <cstdint>
#include <ranges>
#include <span>
#include <vector>

namespace streams {

struct Header {
  std::uint8_t version;
  std::uint64_t stream_index;
  std::uint16_t length;
} __attribute__((packed));

auto format_as(const Header& header) {
  return fmt::format("Header {{ version: {}, stream_index: {}, length: {} }}",
                     header.version, header.stream_index, header.length);
}

struct Packet {
  Header header;
  std::span<const std::byte> data;
};

auto format_as(const Packet& packet) {
  return fmt::format("Packet {{ header: {}, data: {} }}", packet.header,
                     packet.data);
}

struct PacketStreamTraits {
  constexpr static std::uint8_t kVersion = 1;
  constexpr static std::uint16_t kChunkSize = 1400;
};

struct BufferQueue {
  constexpr void add_buffer(auto b) noexcept {
    buffers_.emplace_back(std::move(b));
  }

  constexpr auto erase_begin(std::size_t count) noexcept {
    buffers_.erase(buffers_.begin(), buffers_.begin() + count);
  }

  constexpr auto get_buffers() noexcept {
    return buffers_ | std::views::transform([](const auto& buf_variant) {
             return std::visit(
                 [](const auto& b) { return std::span<const std::byte>(b); },
                 buf_variant);
           });
  }

  constexpr std::size_t num_buffers() const noexcept { return buffers_.size(); }

 private:
  using BufferVariant =
      std::variant<std::vector<std::byte>, std::span<const std::byte>>;

  std::vector<BufferVariant> buffers_;
};

auto format_as(const BufferQueue& buf_stream) {
  return fmt::format("BufferQueue {{ num_buffers: {} }}",
                     buf_stream.num_buffers());
}

template <typename BufStreamT, typename TraitsT = PacketStreamTraits>
struct PacketStream {
  using Traits = TraitsT;
  constexpr void set_buf_stream(auto& buf_stream) noexcept {
    buf_stream_ = &buf_stream;
  }

  constexpr auto cleanup() noexcept {
    buf_stream_->erase_begin(buffer_index_);
    buffer_index_ = 0;
  }

  constexpr auto get_packets() noexcept {
    return buf_stream_->get_buffers() | std::views::drop(buffer_index_) |
           std::views::transform([this](const auto& buffer) {
             return buffer | std::views::drop(buffer_offset_) |
                    std::views::chunk(TraitsT::kChunkSize) |
                    std::views::transform([this, buffer](const auto& chunk) {
                      buffer_offset_ += chunk.size();
                      if (buffer_offset_ == buffer.size()) {
                        buffer_index_++;
                        buffer_offset_ = 0;
                      }
                      auto p = Packet{
                          .header = {.version = TraitsT::kVersion,
                                     .stream_index = stream_index_,
                                     .length = static_cast<std::uint16_t>(
                                         chunk.size())},
                          .data = chunk};
                      stream_index_ += chunk.size();
                      return p;
                    });
           }) |
           std::views::join;
  }

  constexpr auto take_packets(std::size_t count) noexcept {
    return get_packets() | std::views::take(count);
  }

 private:
  BufStreamT* buf_stream_ = nullptr;
  std::uint64_t stream_index_ = 0;
  std::size_t buffer_index_ = 0;
  std::size_t buffer_offset_ = 0;
};

auto MakeBuffer(std::uint8_t begin, std::uint32_t size) noexcept {
  std::vector<std::byte> result;
  result.reserve(size);
  for (std::uint32_t i = 0; i < size; ++i) {
    result.push_back(static_cast<std::byte>(begin + i));
  }
  return result;
}

template <typename Fn>
struct OnDestruct {
  OnDestruct(Fn&& fn) : fn(std::forward<Fn>(fn)) {}
  ~OnDestruct() { fn(); }
  Fn fn;
};

enum class SinkSendResult { kDone, kAgain };

struct DummySink {
  SinkSendResult send(const Packet& packet) {
    if (packet.header.length > remaining_bytes_) {
      return SinkSendResult::kAgain;
    }
    remaining_bytes_ -= packet.header.length;
    data_.emplace_back(packet.data.begin(), packet.data.end());
    packets.push_back(packet);
    packets.back().data = data_.back();
    return SinkSendResult::kDone;
  }

  void add_bytes(std::size_t bytes) { remaining_bytes_ += bytes; }
  void set_remaining_bytes(std::size_t remaining) {
    remaining_bytes_ = remaining;
  }

  std::vector<Packet> packets;

 private:
  std::vector<std::vector<std::byte>> data_;
  std::size_t remaining_bytes_ = 0;
};

}  // namespace streams

#endif  // EXAMPLE_STREAMS_H_