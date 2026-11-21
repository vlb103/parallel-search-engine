#ifndef SRC_WEBSOCKET_H_
#define SRC_WEBSOCKET_H_

#include <optional>
#include <string>

#include "http.h"

bool WsHandshake(int fd, const HttpRequest& request);

std::optional<std::string> WsReadText(int fd);

bool WsSendText(int fd, const std::string& message);

#endif  // SRC_WEBSOCKET_H_
