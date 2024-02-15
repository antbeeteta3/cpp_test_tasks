#pragma once

#include <string>
#include <optional>

#include "message.pb.h"


namespace protocol {
	std::optional<pb::Message> parseFrom(std::string_view data);
	std::string serialize(const pb::Message& msg);
	std::string serialize(pb::MessageType type);

	std::string serialized_dev_info(std::string_view device_name, std::string_view os_version, std::string_view serial_number, std::string_view description);
}
