#ifndef EXAMPLE_UDP_SINK_H_
#define EXAMPLE_UDP_SINK_H_

#include <arpa/inet.h>
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <netinet/in.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#include "example/streams.h"

auto format_as(const sockaddr_in& saddr) {
  return fmt::format(
      "sockaddr_in {{ sin_family: {}, sin_port: {}, sin_addr: {} }}",
      saddr.sin_family, saddr.sin_port,
      std::as_bytes(std::span(&saddr.sin_addr.s_addr, 1)));
}

auto format_as(const iovec& iov) {
  return fmt::format("iovec {{ iov_base: {}, iov_len: {} }}",
                     (void*)iov.iov_base, iov.iov_len);
}

// format_as for msghdr
auto format_as(const msghdr& msg) {
  return fmt::format(
      "msghdr {{ msg_name: {}, msg_namelen: {}, msg_iov: {}, msg_iovlen: {} }}",
      *(sockaddr_in*)msg.msg_name, msg.msg_namelen, (void*)msg.msg_iov,
      msg.msg_iovlen);
}

namespace streams {

struct UdpSink {
  UdpSink() {
    bool error = true;
    OnDestruct on_destruct = [this, &error] {
      if (error) {
        // close(fd_);
        // fd_ = -1;
        fmt::println("UdpSink error");
        std::abort();
      }
    };

    fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd_ == -1) {
      return;
    }

    // make fd_ non-blocking
    int flags = fcntl(fd_, F_GETFL, 0);
    if (flags == -1) {
      return;
    }

    if (fcntl(fd_, F_SETFL, flags | O_NONBLOCK) == -1) {
      return;
    }

    error = false;
  }
  UdpSink(const UdpSink&) = delete;
  UdpSink(UdpSink&& other) : fd_(other.fd_), destination_(other.destination_) {
    other.fd_ = -1;
  }
  UdpSink& operator=(const UdpSink&) = delete;
  UdpSink& operator=(UdpSink&& other) {
    fd_ = other.fd_;
    destination_ = other.destination_;
    other.fd_ = -1;
    return *this;
  }

  ~UdpSink() {
    close(fd_);
    fd_ = -1;
  }

  void set_destination(std::string_view ip, std::uint16_t port) {
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.data(), &addr.sin_addr);
    destination_ = addr;
  }

  constexpr SinkSendResult send(const Packet& packet) noexcept {
    return send(packet, destination_);
  }

 private:
  constexpr SinkSendResult send(const Packet& packet,
                                sockaddr_in& dest) noexcept {
    auto header_span = std::as_bytes(std::span(&packet.header, 1));
    iovec iov[2] = {
        {.iov_base =
             const_cast<void*>(static_cast<const void*>(header_span.data())),
         .iov_len = header_span.size()},
        {.iov_base =
             const_cast<void*>(static_cast<const void*>(packet.data.data())),
         .iov_len = packet.data.size()},
    };
    msghdr msg = {};
    msg.msg_iov = iov;
    msg.msg_iovlen = 2;
    msg.msg_name = const_cast<void*>(static_cast<const void*>(&dest));
    msg.msg_namelen = sizeof(dest);
    if (sendmsg(fd_, &msg, 0) == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return SinkSendResult::kAgain;
      }
      fmt::println("sendmsg error: {}", strerror(errno));
      std::abort();
    }
    return SinkSendResult::kDone;
  }

  int fd_ = -1;
  sockaddr_in destination_;
};

}  // namespace streams

#endif