#pragma once
struct get_req
{
	std::string key;
};
struct get_resp
{
	std::string status;
	std::string value;
};

struct set_req
{
	std::string key;
	std::string value;
};

struct set_resp
{
	std::string status;
};


struct del_req
{
	std::string key;
};

struct del_resp
{
	std::string status;
};

struct exist_req
{
	std::string key;
};

struct exist_resp
{
	std::string status;
};