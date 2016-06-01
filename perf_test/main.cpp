
#include <stdlib.h>
#include <stdio.h>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <iomanip>
#include <string>

#include <thread>
#include <chrono>
#include <fstream>
#include <sstream>
#include <kapok/Kapok.hpp>
#include <boost/program_options.hpp>

#include "common.h"
#include "base64.hpp"
#include "stop_watch.h"
#include "../client_proxy/client_proxy.hpp"

#ifndef _NOEXCEPT
#define _NOEXCEPT   noexcept
#endif

namespace app_opts = boost::program_options;

int g_test_mode = 0;
int g_test_category = 0;

std::string g_test_mode_str;
std::string g_test_category_str;

std::string g_server_ip;
std::string g_server_port;
std::string g_client_ip;
std::string g_client_port;

std::string g_rpc_topic;

// result要么是基本类型，要么是结构体；当请求成功时，code为0, 如果请求是无返回类型的，则result为空; 
// 如果是有返回值的，则result为返回值。response_msg会序列化为一个标准的json串，回发给客户端。 
// 网络消息的格式是length+body，由4个字节的长度信息（用来指示包体的长度）加包体组成。 
template <typename T>
struct response_msg
{
	int code;
	T result; // json格式字符串，基本类型或者是结构体.
	META(code, result);
};

enum result_code
{
	OK = 0,
	FAIL = 1,
	EXCEPTION = 2,
};

struct person
{
	int age;
	std::string name;

	META(age, name);
};

int get_test_mode(const std::string & test_mode)
{
    int n_test_mode = test_mode_default;
    if (test_mode == "rpc-call")
        n_test_mode = test_mode_rpc_call;
    else if (test_mode == "sub-pub")
        n_test_mode = test_mode_sub_pub;
    return n_test_mode;
}

int get_test_category(const std::string & test_category)
{
    int n_test_category = test_cate_default;
    if (test_category == "qps")
        n_test_category = test_cate_qps;
    else if (test_category == "throughput")
        n_test_category = test_cate_throughput;
    return n_test_category;
}

std::string get_test_mode_str(const std::string & test_mode)
{
    return ((!test_mode.empty()) ? test_mode : "rpc-call");
}

std::string get_test_category_str(const std::string & test_category)
{
    return ((!test_category.empty()) ? test_category : "qps");
}

bool try_deserialize_int(DeSerializer & dr) _NOEXCEPT
{
    bool is_success = false;
    try
    {
		response_msg<int> response = {};
		dr.Deserialize(response);
		if (response.code == result_code::OK)
		{
			std::cout << response.result << std::endl;
            is_success = true;
		}
    }
	catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    return is_success;
}

bool try_deserialize_string(DeSerializer & dr) _NOEXCEPT
{
    bool is_success = false;
    try
    {
		response_msg<std::string> response = {};
		dr.Deserialize(response);
		if (response.code == result_code::OK)
		{
			std::cout << response.result.c_str() << std::endl;
            is_success = true;
		}
    }
	catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    return is_success;
}

void test_client() _NOEXCEPT
{
	try
	{
		boost::asio::io_service io_service;
		DeSerializer dr;
		client_proxy client(io_service);
		client.connect(g_server_ip, g_server_port);
		person p = { 20, "aa" };
		//auto str = client.make_json("fun1", p, 1);
		//client.call(str);
		
		std::string result;
        if (g_rpc_topic == "add") {
            result = client.call(g_rpc_topic.c_str(), p, 1);
        }
        else if (g_rpc_topic == "fun1") {
            result = client.call(g_rpc_topic.c_str(), p, 1);
        }
        else if (g_rpc_topic == "fun2") {
            result = client.call("fun2", p, 1);
        }
        else {
            // g_rpc_topic is empty or unknown topic.
            if (g_rpc_topic.empty())
                std::cout << "Rpc topic is empty." << std::endl;
            else
                std::cout << "Unknown rpc topic: " << g_rpc_topic.c_str() << std::endl;
            return;
        }
		dr.Parse(result);

        bool is_success = try_deserialize_int(dr);
        if (!is_success) {
            try_deserialize_string(dr);
        }
		io_service.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << std::endl;
	}
}

void dispaly_qps(StopWatch & sw, std::uint32_t & counter)
{
    if (sw.getElapsedMillisec() > 1000.0) {
        sw.stop();
        double elapsed_time = sw.getSecond();
        std::cout << "mode: " << g_test_mode_str.c_str() << ",  test: " << g_test_category_str.c_str() << ",  "
                  << "topic: " << g_rpc_topic.c_str() << ",  ";
        //std::cout << "count = " << std::right << std::setw(7) << counter << ",  ";
        std::cout << "qps = "
                  << std::right << std::setw(9) << std::setiosflags(std::ios::fixed) << std::setprecision(1)
                  << ((double)counter / elapsed_time);
        /*
        std::cout << ",  time elapsed: "
                  << std::right << std::setw(8) << std::setiosflags(std::ios::fixed) << std::setprecision(3)
                  << elapsed_time * 1000.0 << " ms.";
        //*/
        std::cout << std::endl;
        counter = 0;
        sw.start();
    }
}

void dispaly_throughput(StopWatch & sw, std::uint32_t & counter, uint32_t package_size, uint32_t times = 1)
{
    if (sw.getElapsedMillisec() > 1000.0) {
        sw.stop();
        package_size = (package_size + 1) + 4;
        double elapsed_time = sw.getSecond();
        std::cout << "mode: " << g_test_mode_str.c_str() << ",  test: " << g_test_category_str.c_str() << ",  "
                  << "topic: " << g_rpc_topic.c_str() << ",  ";
        //std::cout << "count = " << std::right << std::setw(7) << counter << ",  ";
        std::cout << "qps = "
                  << std::right << std::setw(9) << std::setiosflags(std::ios::fixed) << std::setprecision(1)
                  << ((double)counter * times / elapsed_time)
                  << ",  throughput = "
                  << std::right << std::setw(8) << std::setiosflags(std::ios::fixed) << std::setprecision(3)
                  << ((double(counter * times * package_size) / 1024.0 / 1024.0) * 8.0 / elapsed_time) << " Mb/s";
        /*
        std::cout << ",  time elapsed: "
                  << std::right << std::setw(8) << std::setiosflags(std::ios::fixed) << std::setprecision(3)
                  << elapsed_time * 1000.0 << " ms.";
        //*/
        std::cout << std::endl;
        counter = 0;
        sw.start();
    }
}

void test_rpc_call_qps()
{
    uint32_t call_count = 0;
    StopWatch sw;
	try
	{
		boost::asio::io_service io_service;
		
		client_proxy client(io_service);
		client.connect(g_server_ip, g_server_port);

		std::string str;
        if (g_rpc_topic == "add")
            str = client.make_json("add", 1, 2);
        else if (g_rpc_topic == "translate")
            str = client.make_json("translate", "test");
        else if (g_rpc_topic == "upload")
            str = client.make_json("upload", "test");
        else {
            std::cout << "Error: Unknown rpc call topic. rpc topic = [" << g_rpc_topic.c_str() << "] ." << std::endl;
            return;
        }
            
		std::thread thd([&io_service] { io_service.run(); });
		std::this_thread::sleep_for(std::chrono::milliseconds(200));

        sw.start();
		while (true)
		{
			std::string response = client.call(str);
            if (!response.empty()) {
                call_count++;
                if ((call_count & 0x7F) == 0) {
                    // Display qps status
                    dispaly_qps(sw, call_count);
                }
            }
		}
        if (thd.joinable())
            thd.join();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << std::endl;
	}
}

void test_rpc_call_throughput()
{
    static const uint32_t repeat_times = 200;
	try
	{
        int n_rpc_topic = -1;
        uint32_t call_count = 0;
        StopWatch sw;

        if (g_rpc_topic == "add") {
            n_rpc_topic = 1;
        }
        else if (g_rpc_topic == "translate") {
            n_rpc_topic = 2;
        }
        else if (g_rpc_topic == "upload") {
            n_rpc_topic = 3;
        }

        boost::asio::io_service io_service;
        client_proxy client(io_service);
	    std::string json_str;
        if (n_rpc_topic == 1)
            json_str = client.make_json("add", 1, 2);
        else if (n_rpc_topic == 2)
            json_str = client.make_json("translate", "test");
        else if (n_rpc_topic == 3)
            json_str = client.make_json("upload", "upload data");
        else {
            std::cout << "Error: Unknown rpc call topic. rpc topic = [" << g_rpc_topic.c_str() << "] ." << std::endl;
            return;
        }
        client.connect(g_server_ip, g_server_port);

        std::thread thd([&io_service] { io_service.run(); });
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        sw.start();
        std::string result;
        int sum = 0;
        while (true) {
            result = client.call(json_str, repeat_times, sum);
            if (!result.empty()) {
                call_count++;
                if ((call_count & 0x7F) == 0) {
                    // Display throughput status
                    dispaly_throughput(sw, call_count, json_str.length(), repeat_times);
                }
            }
        }
        if (thd.joinable())
            thd.join();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << std::endl;
	}
}

void test_rpc_call_throughput2()
{
	try
	{
		boost::asio::io_service io_service;
		boost::asio::spawn(io_service, [&io_service] (boost::asio::yield_context yield)
		{
            uint32_t call_count = 0;
            int n_rpc_topic = -1;
            StopWatch sw;

            if (g_rpc_topic == "add") {
                n_rpc_topic = 1;
            }
            else if (g_rpc_topic == "translate") {
                n_rpc_topic = 2;
            }
            else if (g_rpc_topic == "upload") {
                n_rpc_topic = 3;
            }

            client_proxy client(io_service);
		    std::string json_str;
            if (n_rpc_topic == 1)
                json_str = client.make_json("add", 1, 2);
            else if (n_rpc_topic == 2)
                json_str = client.make_json("translate", "test");
            else if (n_rpc_topic == 3)
                json_str = client.make_json("upload", "upload data");
            else {
                std::cout << "Error: Unknown rpc call topic. rpc topic = [" << g_rpc_topic.c_str() << "] ." << std::endl;
                return;
            }
            client.async_connect(g_server_ip, g_server_port, yield);

            sw.start();
            std::string result;
            while (true) {
                if (n_rpc_topic == 1)
                    result = client.async_call("add", yield, 1, 2);
                else if (n_rpc_topic == 2)
                    result = client.async_call("translate", yield, "test");
                else if (n_rpc_topic == 3)
                    result = client.async_call("upload", yield, "upload data");
                else
                    return;
                //client.async_call(json_str, yield);
                //handle_result<int>(result.c_str());
                if (!result.empty()) {
                    call_count++;
                    if ((call_count & 0x7F) == 0) {
                        // Display throughput status
                        dispaly_throughput(sw, call_count, json_str.length());
                    }
                }
            }
        });
        io_service.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << std::endl;
	}
}

int main(int argc, char * argv[])
{
    std::string app_name, test_category, test_mode, rpc_topic;
    std::string client_ip, client_port, server_ip, server_port;

    app_opts::options_description desc("Command list");
    desc.add_options()
        ("help,h", "usage info")
        ("mode,m", app_opts::value<std::string>(&test_category)->default_value("rpc-call"), "test mode")
        ("test,t", app_opts::value<std::string>(&test_mode)->default_value("qps"), "test category")
        ("rpc-topic,r", app_opts::value<std::string>(&rpc_topic)->default_value("add"), "rpc call's topic")
        ("server-ip,s", app_opts::value<std::string>(&server_ip)->default_value("127.0.0.1"), "server ip address")
        ("server-port,o", app_opts::value<std::string>(&server_port)->default_value("9000"), "server port")
        ("ip,i", app_opts::value<std::string>(&client_ip)->default_value("127.0.0.1"), "client ip address")
        ("port,p", app_opts::value<std::string>(&client_port)->default_value("9010"), "client port")
        ;

    app_opts::variables_map vars_map;
    try {
        app_opts::store(app_opts::parse_command_line(argc, argv, desc), vars_map);
    }
    catch (const std::exception & e) {
        std::cout << "Exception is: " << e.what() << std::endl;
    }
    app_opts::notify(vars_map);    

    if (vars_map.count("help") > 0) {
        std::cout << desc << std::endl;
        ::exit(EXIT_FAILURE);
    }

    if (vars_map.count("mode") > 0) {
        //std::cout << "Test mode is: " << vars_map["mode"].as<std::string>().c_str() << "." << std::endl;;
        test_mode = vars_map["mode"].as<std::string>();
    }
    else {
        //std::cout << "Test mode was not set.\n";
    }

    if (vars_map.count("test") > 0) {
        //std::cout << "Test category is: " << vars_map["test"].as<std::string>().c_str() << "." << std::endl;;
        test_category = vars_map["test"].as<std::string>();
    }
    else {
        //std::cout << "Test category was not set.\n";
    }

    g_test_mode = get_test_mode(test_mode);
    g_test_category = get_test_category(test_category);
    g_test_mode_str = get_test_mode_str(test_mode);
    g_test_category_str = get_test_category_str(test_category);
    g_rpc_topic = rpc_topic;

    g_server_ip = server_ip;
    g_server_port = server_port;
    g_client_ip = client_ip;
    g_client_port = client_port;    

    if (g_test_mode == test_mode_rpc_call) {
        if (g_test_category == test_cate_qps)
            test_rpc_call_qps();
        else if (g_test_category == test_cate_throughput)
            test_rpc_call_throughput();
    }
    else if (g_test_mode == test_mode_sub_pub) {
        //
    }
    else {
        test_client();
    }

    ::system("pause");
    return 0;
}
