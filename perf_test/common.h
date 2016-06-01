#pragma once

enum test_mode_t {
    test_mode_rpc_call,
    test_mode_sub_pub,
    test_mode_default = -1
};

enum test_category_t {
    test_cate_qps,
    test_cate_throughput,
    test_cate_default = -1
};

extern int g_test_mode;
extern int g_test_category;

extern std::string g_test_mode_str;
extern std::string g_test_category_str;

extern std::string g_server_ip;
extern std::string g_server_port;
extern std::string g_client_ip;
extern std::string g_client_port;
extern std::string g_rpc_topic;
