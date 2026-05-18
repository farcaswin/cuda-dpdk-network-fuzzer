#include "LinuxSocketSender.h"
#include "Logger.h"
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <cstring>
#include <errno.h>
#include <arpa/inet.h>

namespace fuzzer {

LinuxSocketSender::LinuxSocketSender(const std::string& iface_name) {
    socket_fd_ = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (socket_fd_ < 0) {
        LOG_ERROR("Failed to create raw socket: {}", strerror(errno));
        throw std::runtime_error("Socket creation failed");
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, iface_name.c_str(), IFNAMSIZ - 1);
    if (ioctl(socket_fd_, SIOCGIFINDEX, &ifr) < 0) {
        LOG_ERROR("Failed to get interface index for {}: {}", iface_name, strerror(errno));
        close(socket_fd_);
        throw std::runtime_error("IOCTL failed");
    }
    ifindex_ = ifr.ifr_ifindex;
}

LinuxSocketSender::~LinuxSocketSender() {
    if (socket_fd_ >= 0) {
        close(socket_fd_);
    }
}

uint32_t LinuxSocketSender::send_burst(const uint8_t* data, const uint16_t* lengths, uint32_t count) {
    uint32_t sent_count = 0;
    struct sockaddr_ll sll;
    memset(&sll, 0, sizeof(sll));
    sll.sll_family = AF_PACKET;
    sll.sll_ifindex = ifindex_;
    sll.sll_halen = ETH_ALEN;

    for (uint32_t i = 0; i < count; ++i) {
        const uint8_t* pkt = data + (static_cast<size_t>(i) * 1514);
        ssize_t sent = sendto(socket_fd_, pkt, lengths[i], 0, (struct sockaddr*)&sll, sizeof(sll));
        if (sent > 0) {
            sent_count++;
        }
    }
    return sent_count;
}

} // namespace fuzzer
