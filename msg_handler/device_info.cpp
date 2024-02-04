#include "device_info.h"

#include <algorithm>
#include <cstdio>
#include <fmt/format.h>
#include <string>


std::string exec(std::string_view cmd) {
    std::string res;

    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.data(), "r"), pclose);
    std::array<char, 128> buf;

    if (!pipe) {
		fmt::print("[ERROR] popen fail: {} ({})\n", strerror(errno), errno);
    }
    while (fgets(buf.data(), buf.size(), pipe.get()) != nullptr) {
        res += buf.data();
    }
    if (!feof(pipe.get())) {
		fmt::print("[ERROR] popen read fail: {}\n", ferror(pipe.get()));
	}


    return res;
}

void rtrim(std::string& s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}


namespace device_info {

std::string device_name() {
	auto ret = exec("hostname");
	rtrim(ret);
	return ret;
}

std::string os_version() {
	auto ret = exec("cat /etc/os-release");
	rtrim(ret);
	return ret;
}

std::string serial_number() {
	return "some_serial_number";
}

std::string description() {
	return "some_description";
}

} // namespace device_info
