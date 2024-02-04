#pragma once

#include <any>
#include <functional>
#include <string>


struct Client {
	std::string label;
	std::any addr;
};


class Transport {
public:
	using DataHandlerType = std::function<void(std::string_view, Client&&)>;
	Transport(DataHandlerType data_handler) : on_data_received(std::move(data_handler)) {}

	virtual ~Transport() = default;
	Transport(const Transport&) = delete;
    Transport& operator=(const Transport&) = delete;
	Transport(const Transport&&) = delete;
    Transport& operator=(const Transport&&) = delete;

	virtual int get_fd() const = 0;
	virtual void on_data_ready() const = 0;
	virtual bool send(std::string_view data, const Client& client) const = 0;

protected:
	DataHandlerType on_data_received;
};
