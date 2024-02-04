#pragma once

#include "transport.h"


class UdpTransport: public Transport {
private:
	const static size_t BUF_SIZE = 1000;
	int sock = -1;

public:
	UdpTransport(Transport::DataHandlerType data_handler);
	~UdpTransport();

	int get_fd() const override;
	void on_data_ready() const override;
	bool send(std::string_view data, const Client& client) const override;
};
