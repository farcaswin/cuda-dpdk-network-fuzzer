#ifndef LINUX_SOCKET_SENDER_H
#define LINUX_SOCKET_SENDER_H

#include <cstdint>
#include <string>

namespace fuzzer {

class LinuxSocketSender {
public:
    LinuxSocketSender(const std::string& iface_name);
    ~LinuxSocketSender();
    
    uint32_t send_burst(const uint8_t* data, const uint16_t* lengths, uint32_t count);

private:
    int socket_fd_;
    int ifindex_;
};

} // namespace fuzzer

#endif // LINUX_SOCKET_SENDER_H
