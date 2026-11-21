#include "websocket.h"

#include <openssl/evp.h>

#include <cstdint>

namespace {

const char kMagicGuid[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
const size_t kMaxMessageSize = 64 * 1024;
const unsigned char kOpcodeText = 0x1;
const unsigned char kOpcodeClose = 0x8;
const unsigned char kFinalFrame = 0x80;
const unsigned char kMaskBit = 0x80;

}  // namespace

bool WsHandshake(int fd, const HttpRequest& request) {
  const auto key = request.headers.find("sec-websocket-key");
  if (key == request.headers.end()) return false;

  const std::string source = key->second + kMagicGuid;
  unsigned char digest[EVP_MAX_MD_SIZE];
  unsigned int digest_size = 0;
  EVP_Digest(source.data(), source.size(), digest, &digest_size, EVP_sha1(),
             nullptr);
  unsigned char accept[64];
  EVP_EncodeBlock(accept, digest, static_cast<int>(digest_size));

  const std::string response =
      "HTTP/1.1 101 Switching Protocols\r\n"
      "Upgrade: websocket\r\n"
      "Connection: Upgrade\r\n"
      "Sec-WebSocket-Accept: " +
      std::string(reinterpret_cast<char*>(accept)) + "\r\n\r\n";
  return SendAll(fd, response.data(), response.size());
}

std::optional<std::string> WsReadText(int fd) {
  while (true) {
    unsigned char header[2];
    if (!RecvAll(fd, header, sizeof(header))) return std::nullopt;
    const unsigned char opcode = header[0] & 0x0F;
    uint64_t length = header[1] & 0x7F;

    if (length == 126) {
      unsigned char extended[2];
      if (!RecvAll(fd, extended, sizeof(extended))) return std::nullopt;
      length = (extended[0] << 8) | extended[1];
    } else if (length == 127) {
      unsigned char extended[8];
      if (!RecvAll(fd, extended, sizeof(extended))) return std::nullopt;
      length = 0;
      for (unsigned char byte : extended) length = (length << 8) | byte;
    }
    if ((header[1] & kMaskBit) == 0 || length > kMaxMessageSize) {
      return std::nullopt;
    }

    unsigned char mask[4];
    std::string payload(length, '\0');
    if (!RecvAll(fd, mask, sizeof(mask)) ||
        !RecvAll(fd, payload.data(), payload.size())) {
      return std::nullopt;
    }
    for (size_t i = 0; i < payload.size(); ++i) payload[i] ^= mask[i % 4];

    if (opcode == kOpcodeClose) return std::nullopt;
    if (opcode == kOpcodeText) return payload;
  }
}

bool WsSendText(int fd, const std::string& message) {
  std::string frame;
  frame.push_back(static_cast<char>(kFinalFrame | kOpcodeText));
  const size_t size = message.size();
  if (size <= 125) {
    frame.push_back(static_cast<char>(size));
  } else if (size <= 0xFFFF) {
    frame.push_back(126);
    frame.push_back(static_cast<char>(size >> 8));
    frame.push_back(static_cast<char>(size & 0xFF));
  } else {
    frame.push_back(127);
    for (int shift = 56; shift >= 0; shift -= 8) {
      frame.push_back(
          static_cast<char>((static_cast<uint64_t>(size) >> shift) & 0xFF));
    }
  }
  frame += message;
  return SendAll(fd, frame.data(), frame.size());
}
