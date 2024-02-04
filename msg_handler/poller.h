#pragma once

#include <functional>

class Transport;


long int curr_timestamp_ms();


class Poller {

private:
	const static int POLL_TICK_MS = 500;
	const static int EXT_TICK_MS = 1000;

	static bool is_working;

	static void signal_handler(int /*signal*/);

public:
	static void poll_loop(const Transport& transport, const std::function<void(long int)>& on_timer);
};
