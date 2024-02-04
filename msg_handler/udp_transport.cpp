#include "udp_transport.h"

#include <cassert>
#include <cerrno>
#include <fmt/format.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>


int listened_port() {
	const int DEFAULT_PORT = 10123;
	const char* PORT_ENV_VAR = "MSG_HANDLER_UDP_PORT";

	static int port = -1;
	if (port < 0) {
		auto* ch_port = getenv(PORT_ENV_VAR);
		if (ch_port) {
			std::string str_port(ch_port);
			std::size_t pos = 0;
			try {
				port = std::stoi(str_port, &pos);
			}
			catch (...) {}
			if (pos == 0 || pos != str_port.size()) {
				throw std::runtime_error(fmt::format("'{}' is wrong port value", ch_port));
			}
		}
		else {
			port = DEFAULT_PORT;
		}
		fmt::print("[INFO] listened UDP port {}\n", port);
	}

	return port;
}


UdpTransport::UdpTransport(Transport::DataHandlerType data_handler) : Transport(std::move(data_handler)) {
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		throw std::runtime_error(fmt::format("socket fail: {}", strerror(errno)));
	}

	struct sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(listened_port());

	if (bind(sock, reinterpret_cast<struct sockaddr*>(&serv_addr), sizeof(serv_addr)) < 0) {
		throw std::runtime_error(fmt::format("bind fail: {}", strerror(errno)));
	}
}

UdpTransport::~UdpTransport() {
	if (sock > 0) {
		close(sock);
	}
}

int UdpTransport::get_fd() const {
	return sock;
}

void UdpTransport::on_data_ready() const {
	struct sockaddr_in client_addr;
	memset(&client_addr, 0, sizeof(client_addr));
	socklen_t addr_len = sizeof(client_addr);

	char buf[BUF_SIZE];
	int recv_n = recvfrom(sock, buf, BUF_SIZE, MSG_DONTWAIT, reinterpret_cast<struct sockaddr*>(&client_addr), &addr_len);

	if (recv_n < 0) {
		fmt::print("[ERROR] recvfrom fail: {} ({})\n", strerror(errno), errno);
	}
	else if (recv_n >= static_cast<int>(BUF_SIZE)) {
		fmt::print("[ERROR] recvfrom got too long message (length={}), dropping it\n", recv_n);
	}
	else {
		std::string_view data(buf, recv_n);
		char addr_buf[128];
		on_data_received(
			data,
			Client{
				.label = fmt::format("{}:{}", inet_ntop(client_addr.sin_family, &client_addr.sin_addr, addr_buf, sizeof(addr_buf)), ntohs(client_addr.sin_port)),
				.addr = client_addr
			}
		);
	}
}

bool UdpTransport::send(std::string_view data, const Client& client) const {
	auto& addr = std::any_cast<const struct sockaddr_in&>(client.addr);
	int snt_n = sendto(sock, data.data(), data.size(), 0, reinterpret_cast<const struct sockaddr*>(&addr), sizeof(struct sockaddr_in));
	if (snt_n < 0) {
		fmt::print("[ERROR] sendto fail: {} ({})\n", strerror(errno), errno);
		return false;
	}
	//fmt::print("sent {}\n", snt_n);
	assert(snt_n == static_cast<int>(data.size()));
	return true;
}
