#include "raft.hpp"
namespace raft
{

	struct snapshot_head
	{
		std::string magic_string_;
		snapshot_info info_;
	};

	bool write_version(acl::ofstream &file, const version &ver)
	{
		snapshot_head head;

		head.magic_string_ = g_magic_string;
		head.info_.set_last_included_term(ver.term_);
		head.info_.set_last_snapshot_index(ver.index_);

		if (!write(file, head.magic_string_))
			return false;

		std::string buffer = head.info_.SerializeAsString();
		if (!write(file, buffer))
			return false;

		return true;
	}

	bool read_version(acl::ifstream &file, version &ver)
	{
		std::string magic_string;
		std::string buffer;
		snapshot_info info;

		if (!read(file, magic_string) || magic_string != g_magic_string)
		{
			logger_error("read snapshot error,%s",
				magic_string.c_str());
			return false;
		}

		if (!read(file, buffer) || !info.ParseFromString(buffer))
		{
			logger_error("read snapshot error");
			return false;
		}

		ver.index_ = info.last_snapshot_index();
		ver.term_ = info.last_included_term();
		return true;
	}

}