#include "poller.h"
#include "protocol.h"
#include "udp_transport.h"
#include "device_info.h"

#include <fmt/format.h>
#include <map>


using namespace std::placeholders;


int ts_label(long int ts) {
	return ts/1000%1000;
}


class Connection {
private:
	static const int PING_WAIT_MS = 10000;
	static const int PING_INTERVAL_MS = 10000;

	const UdpTransport& transport;
	Client client;

	long int last_ping_ts = 0;
	bool got_pong = true;

public:
	Connection(const UdpTransport& transport_, Client&& client_) : transport(transport_), client(client_) {}

	const Client& get_client() const {
		return client;
	}

	bool check_state(long int ts) {
		//fmt::print("__deb {} {} {} \n", got_pong, ts - last_ping_ts;
		if (!got_pong && (ts - last_ping_ts) > PING_WAIT_MS) {
			return false;
		}
		if (got_pong && (ts - last_ping_ts) > PING_INTERVAL_MS) {
			send_ping(ts);
		}
		return true;
	}

	void send_ping(long int ts) {
		transport.send(protocol::serialize(pb::PING), client);
		last_ping_ts = ts;
		got_pong = false;
		fmt::print("[INFO] sent ping to {} [{}]\n", client.label, ts_label(ts));
	}

	void on_pong() {
		if (got_pong) {
			fmt::print("[WARNING] unexpected pong from {}\n", client.label);
		}
		got_pong = true;
	}
};


class MsgHandler {
private:
	UdpTransport transport;

	using MessageHandlerType = std::function<void(const pb::Message&, Client&& client)>;
	const std::map<pb::MessageType, MessageHandlerType> handlers;

	std::map<std::string, Connection> connections;

public:
	MsgHandler() :
		transport{std::bind(&MsgHandler::on_data_recieved, this, _1, _2)},
		handlers{
			{pb::CONNECT,      std::bind(&MsgHandler::on_connect,      this, _1, _2)},
			{pb::DISCONNECT,   std::bind(&MsgHandler::on_disconnect,   this, _1, _2)},
			{pb::PONG,         std::bind(&MsgHandler::on_pong,         this, _1, _2)},
			{pb::GET_DEV_INFO, std::bind(&MsgHandler::on_get_dev_info, this, _1, _2)},
		}
	{}

	const Transport& get_transport() const {
		return transport;
	}

	void on_timer(long int ts) {
		for (auto it = connections.begin(); it != connections.end(); ) {
			if (!it->second.check_state(ts)) {
				fmt::print("[INFO] Connection from {} is expired [{}]\n", it->first, ts_label(ts));
				it = connections.erase(it);
			}
			else {
				++it;
			}
		}
	}

	void on_data_recieved(std::string_view data, Client&& client) {
		//fmt::print("recvfrom got message: {} from client {}\n", data, client.label);
		auto msg = protocol::parseFrom(data);
		if (!msg) {
			fmt::print("[ERROR] message parsing fail: {}\n", data);
			return;
		}
		auto it = handlers.find(msg->type());
		if (it == handlers.end()) {
			fmt::print("[ERROR] unsupported message type: {}\n", static_cast<int>(msg->type()));
			return;
		}
		it->second(msg.value(), std::move(client));
	}

	void on_connect(const pb::Message& /*msg*/, Client&& client) {
		auto it = connections.find(client.label);
		if (it != connections.end()) {
			fmt::print("[WARNING] repeated connect from {}\n", client.label);
			return;
		}
		auto new_it = connections.emplace(client.label, Connection(transport, std::move(client))).first;
		auto curr_ts = curr_timestamp_ms();
		fmt::print("[INFO] Added new connection from {} [{}]\n", client.label, ts_label(curr_ts));
		new_it->second.send_ping(curr_ts);
	}

	void on_disconnect(const pb::Message& /*msg*/, Client&& client) {
		auto it = connections.find(client.label);
		if (it == connections.end()) {
			fmt::print("[WARNING] disconnect error, no connect to {}\n", client.label);
			return;
		}
		connections.erase(it);
		fmt::print("[INFO] disconnected from {}\n", client.label);
	}

	void on_pong(const pb::Message& /*msg*/, Client&& client) {
		auto it = connections.find(client.label);
		if (it == connections.end()) {
			fmt::print("[WARNING] pong error, no connect to {}\n", client.label);
			return;
		}
		it->second.on_pong();
		fmt::print("[INFO] got pong from {}\n", client.label);
	}

	void on_get_dev_info(const pb::Message& /*msg*/, Client&& client) {
		auto it = connections.find(client.label);
		if (it == connections.end()) {
			fmt::print("[WARNING] get_dev_info error, no connect to {}\n", client.label);
			return;
		}

		auto dev_info = protocol::serialized_dev_info(device_info::device_name(), device_info::os_version(), device_info::serial_number(), device_info::description());
		transport.send(dev_info, it->second.get_client());
	}
};


int main(int argc, const char** argv) {
	MsgHandler handler;

	Poller::poll_loop(handler.get_transport(), std::bind(&MsgHandler::on_timer, &handler, _1));

	fmt::print("Exiting\n");
	return 0;
}
