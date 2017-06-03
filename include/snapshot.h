#pragma once
namespace raft
{
	const static std::string g_magic_string("raft-snapshot-head");

	

	struct snapshot_callback
	{
		virtual bool make_snapshot(acl::ofstream &file) = 0;
	};

	bool write_version(acl::ofstream &file, const version &ver);
	
	bool read_version(acl::ifstream &file, version &ver);
}