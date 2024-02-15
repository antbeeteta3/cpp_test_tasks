#include "protocol.h"

#include <fmt/format.h>


namespace protocol {

std::optional<pb::Message> parseFrom(std::string_view data) {
	pb::Message ret;

	if (!ret.ParseFromString(std::string(data))) {
		return {};
	}

	return ret;
}

std::string serialize(const pb::Message& msg) {
	std::string ret;

	bool res = msg.SerializeToString(&ret);
	assert(res);

	return ret;
}

std::string serialize(pb::MessageType type) {
	pb::Message msg;
	msg.set_type(type);

	return serialize(msg);
}

std::string serialized_dev_info(std::string_view device_name, std::string_view os_version, std::string_view serial_number, std::string_view description) {
	pb::Message msg;
	msg.set_type(pb::DEV_INFO);

	pb::DeviceInfo dev_info;
	dev_info.set_name(device_name.data(), device_name.size());
	dev_info.set_osversion(os_version.data(), os_version.size());
	dev_info.set_serialnumber(serial_number.data(), serial_number.size());
	dev_info.set_description(description.data(), description.size());

	msg.mutable_data()->PackFrom(dev_info);

	return serialize(msg);
}

} // namespace protocol
