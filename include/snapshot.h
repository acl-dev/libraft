#pragma once
namespace raft
{
	const static std::string g_magic_string("raft-snapshot-head");

	struct snapshot_callback
	{
		virtual bool receive_snapshot_callback(
			const std::string &filepath) = 0;

		virtual bool make_snapshot_callback(const std::string &path, 
			std::string &filename) = 0;
	};

	bool write(acl::ofstream &file, const version &ver);
	
	bool read(acl::ifstream &file, version &ver);
}