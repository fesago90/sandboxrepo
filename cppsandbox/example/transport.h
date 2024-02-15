#ifndef EXAMPLE_TRANSPORT_H_
#define EXAMPLE_TRANSPORT_H_

#include "example/streams.h"

namespace streams {

template <typename BufStreamT, typename SinkT>
struct Transport {
  Transport(BufStreamT& buf_stream) {
    packet_stream_.set_buf_stream(buf_stream);
  }

  void SetSink(SinkT& sink) { sink_ = &sink; }

  std::size_t maybe_send_packets(std::size_t count = 1024) {
    auto flush_packets = [this, count](auto&& packets) {
      std::size_t num_sent = 0;
      for (const auto& p : packets) {
        auto result = sink_->send(p);
        if (result == SinkSendResult::kAgain) {
          maybe_packet_ = p;
          return std::pair(num_sent, false);
        }

        ++num_sent;
      }
      return std::pair(num_sent, true);
    };

    std::uint64_t total_num_sent = 0;
    bool stop = false;
    do {
      std::pair<int, bool> flush_result;
      if (maybe_packet_.has_value()) {
        auto p = std::move(*maybe_packet_);
        maybe_packet_.reset();
        flush_result = flush_packets(std::span(&p, 1));
      } else {
        auto packets = packet_stream_.take_packets(count - total_num_sent);
        flush_result = flush_packets(packets);
        if (packets.begin() == packets.end()) {
          break;
        }
      }
      auto&& [num_sent, all_sent] = flush_result;
      total_num_sent += num_sent;
      stop = !all_sent;

    } while (!stop);

    if (!maybe_packet_.has_value()) {
      packet_stream_.cleanup();
    }

    return total_num_sent;
  }

 private:
  std::optional<Packet> maybe_packet_;
  PacketStream<BufStreamT> packet_stream_;
  SinkT* sink_;
};

}  // namespace streams

#endif  // EXAMPLE_TRANSPORT_H_