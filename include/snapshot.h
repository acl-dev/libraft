#pragma once
namespace raft
{
	const static std::string g_magic_string("raft-snapshot-head");

	struct snapshot_callback
	{
		virtual bool load_snapshot(
			const std::string &filepath) = 0;

		virtual bool make_snapshot(const std::string &path, 
			std::string &filepath) = 0;
	};
	
	struct version;

	inline bool write(acl::ostream &file, const version &ver);
	
	inline bool read(acl::istream &file, version &ver);
}