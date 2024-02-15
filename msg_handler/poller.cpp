#include "poller.h"
#include "transport.h"

#include <cassert>
#include <chrono>
#include <csignal>
#include <cerrno>
#include <fmt/format.h>
#include <sys/poll.h>


void Poller::signal_handler(int /*signal*/) {
	//fmt::print("Got signal {}\n", signal);
	is_working = false;
}

long int curr_timestamp_ms() {
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

void Poller::poll_loop(const Transport& transport, const std::function<void(long int)>& on_timer) {
	assert(!is_working);	// run once
	is_working = true;

	std::signal(SIGINT, Poller::signal_handler);
	std::signal(SIGTERM, Poller::signal_handler);

	long int last_ts_ms = 0;
	struct pollfd fds{.fd = transport.get_fd(), .events = POLLIN, .revents = 0};
	while (is_working) {
		fds.revents = 0;
		int res = poll(&fds, 1, POLL_TICK_MS);
		if (is_working && res < 0) {
			throw std::runtime_error(fmt::format("poll fail: {}", strerror(errno)));
		}

		if (res > 0 && fds.revents & POLL_IN) {
			transport.on_data_ready();
		}

		auto curr_ts_ms = curr_timestamp_ms();
		if (curr_ts_ms - last_ts_ms > EXT_TICK_MS) {
			on_timer(curr_ts_ms);
			last_ts_ms = curr_ts_ms;
		}
	}
}

bool Poller::is_working = false;
