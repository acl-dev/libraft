#include "raft.hpp"
#include "addr_info.h"
#include "memkv_proto.h"
#include "cluster_config.h"
#include "raft_config.h"
#include "memkv_service.h"
#include "memkv.h"




int main(int argc, char *argv[])
{
	memkv kv;

	acl::acl_cpp_init();

    const char* logfile = "test.log";
    const char* pro_name = "memkv";
    const char* cfg = "1:5; 10:5; 11:5";

    // 在程序初始化时打开日志
    logger_open(logfile, pro_name, cfg);
    acl::log::stdout_open(true);

	//remove old log.
	remove(logfile);
	//default addr
	const char* addr = "127.0.0.1:11081";

	if (argc >= 2 && strcmp(argv[1], "alone") == 0)
	{
		if (argc == 3)
		{
			printf("listen on: %s\r\n", argv[2]);
			kv.run_alone(argv[2], NULL, 0, 100);
		}
		else if (argc == 4)
		{
			printf("listen on: %s\r\n", argv[2]);
			kv.run_alone(argv[2], argv[3], 0, 100);
		}
		else
		{
			printf("listen on: %s\r\n", addr);
			kv.run_alone(addr, NULL, 0, 100);
		}

		printf("Enter any key to exit now\r\n");
		getchar();
	}
	else
	{
		kv.run_daemon(argc, argv);
	}
	return 0;
}