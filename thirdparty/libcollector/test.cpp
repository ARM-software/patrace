#include "interface.hpp"

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <jsoncpp/json/writer.h>
#include <unistd.h>

#ifndef DEBUG
// otherwise we will get complaints about assert()ed variables being unused
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

static void test1()
{
	printf("Trying to initialize all functional collectors...\n");
	Json::Value j;
	Collection c(j);
	std::vector<std::string> list = c.available();
	printf("Supported collectors:\n");
	for (const auto& s : list)
	{
		printf("\t%s\n", s.c_str());
	}
	std::vector<std::string> list_bad = c.unavailable();
	printf("Non-supported collectors:\n");
	for (const auto& s : list_bad)
	{
		printf("\t%s\n", s.c_str());
	}
	bool result = c.initialize(list);
	assert(result);
	c.collector("procfs")->useThreading(10);
	printf("Starting...\n");
	c.start({"result1", "result2", "result3"});
	c.collect({0, -1, 0});
	for (int i = 1; i < 4; i++)
	{
		usleep(350000 + random() % 100000);
		c.collect({i, -1, i});
	}
	c.collect({4, -1, 4});
	c.collect({5, -1, 5});
	usleep(random() % 1000);
	printf("Stopping...\n");
	c.stop();
	Json::Value results = c.results();
	Json::StyledWriter writer;
	std::string data = writer.write(results);
	printf("Results:\n%s", data.c_str());
	c.writeCSV_MTV("mtv.csv");
	c.writeCSV("excel.csv");
}

static void test2()
{
	printf("Trying to initialize misnamed collector through API (should not work)...\n");
	Json::Value j;
	Collection c(j);
	bool result = c.initialize({"bad_collector"});
	assert(result);
	c.start();
	usleep(random() % 100);
	for (int i = 0; i < 3; i++)
	{
		c.collect();
		usleep(50 + random() % 100);
	}
	usleep(random() % 100);
	c.stop();
	usleep(random() % 100);
	Json::Value results = c.results();
	Json::StyledWriter writer;
	std::string data = writer.write(results);
	printf("Results:\n%s", data.c_str());
}

static void test3()
{
	printf("Trying to initialize misnamed collector through JSON (should not work)...\n");
	Json::Value j;
	Json::Value v;
	v["required"] = true;
	j["bad_collector"] = v;
	Collection c(j);
	bool result = c.initialize();
	assert(!result); // should fail!
	c.start();
	for (int i = 0; i < 3; i++)
	{
		c.collect();
		usleep(50 + random() % 100);
	}
	c.stop();
	Json::Value results = c.results();
	Json::StyledWriter writer;
	std::string data = writer.write(results);
	printf("Results:\n%s", data.c_str());
}

static void test4()
{
	printf("Trying to initialize collector through JSON (should work)...\n");
	Json::Value j;
	Json::Value v;
	Json::Value p;
	Json::Value perf;
	v["required"] = true;
	v["threaded"] = true;
	v["sample_rate"] = 5;
	j["procfs"] = v;
	j["streamline"] = Json::objectValue;
	j["cpufreq"] = v;
	p["info"] = "test";
	j["provenance"] = p;
	Collection c(j);
	bool result = c.initialize();
	assert(result);
	c.start();
	for (int i = 0; i < 3; i++)
	{
		c.collect();
		usleep(random() % 10000);
	}
	c.stop();
	Json::Value results = c.results();
	Json::StyledWriter writer;
	std::string data = writer.write(results);
	printf("Results (1):\n%s", data.c_str());
	usleep(100 + random() % 1000000);
	c.start(); // go again (clears previous results)
	for (int i = 0; i < 3; i++)
	{
		c.collect();
		usleep(25000 + random() % 1000000);
	}
	c.stop();
	results = c.results();
	data = writer.write(results);
	printf("Results (2):\n%s", data.c_str());
	assert(results.isMember("provenance"));
	for (const std::string& s : results.getMemberNames()) // verify that we get exactly 3 results, no duplicates
	{
		if (s == "provenance" || s == "streamline")
		{
			continue;
		}
		Json::Value collector = results[s];
		assert(collector.isObject());
		for (const std::string& k : collector.getMemberNames())
		{
			assert(results[s][k].isArray());
			assert(results[s][k].size() == 3);
		}
	}
}

static void test5()
{
	printf("Trying to initialize collector through both API and JSON (should work)...\n");
	Json::Value j;
	Json::Value v;
	v["required"] = true;
	j["procfs"] = v;
	Collection c(j);
	bool result = c.initialize({"procfs"});
	assert(result);
	c.start({"result1", "result2"});
	for (int i = 0; i < 3; i++)
	{
		c.collect({i, -1});
		usleep(50 + random() % 100);
	}
	c.stop();
	Json::Value results = c.results();
	Json::StyledWriter writer;
	std::string data = writer.write(results);
	printf("Results:\n%s", data.c_str());
	for (const std::string& s : results.getMemberNames()) // verify that we get exactly 3 results, no duplicates
	{
		Json::Value collector = results[s];
		assert(collector.isObject());
		for (const std::string& k : collector.getMemberNames())
		{
			assert(results[s][k].isArray());
			assert(results[s][k].size() == 3);
		}
	}
}

static void test6()
{
	printf("Trying to initialize collector through JSON without anything set (should work)...\n");
	Json::Value v;
	Collection c(v);
	c.start();
	for (int i = 0; i < 3; i++)
	{
		c.collect();
		usleep(50 + random() % 100);
	}
	c.stop();
	Json::Value results = c.results();
	Json::StyledWriter writer;
	std::string data = writer.write(results);
	printf("Results:\n%s", data.c_str());
	for (const std::string& s : results.getMemberNames()) // verify that we get exactly 3 results, no duplicates
	{
		Json::Value collector = results[s];
		assert(collector.isObject());
		for (const std::string& k : collector.getMemberNames())
		{
			assert(results[s][k].isArray());
			assert(results[s][k].size() == 3);
		}
	}
}

static void test7()
{
	printf("Trying to initialize all functional collectors with summaries...\n");
	Json::Value j;
	Collection c(j);
	std::vector<std::string> list = c.available();
	printf("Supported collectors:\n");
	for (const auto& s : list)
	{
		printf("\t%s\n", s.c_str());
	}
	std::vector<std::string> list_bad = c.unavailable();
	printf("Non-supported collectors:\n");
	for (const auto& s : list_bad)
	{
		printf("\t%s\n", s.c_str());
	}
	bool result = c.initialize(list);
	assert(result);
	c.collector("procfs")->useThreading(10);
	printf("Starting...\n");
	c.start({"result1", "result2", "result3"});
	for (int j = 0; j < 10; j++)
	{
		c.collect({0, -1, 0});
		for (int i = 1; i < 4; i++)
		{
			usleep(35000);
			c.collect({i, -1, i});
		}
		c.collect({4, -1, 4});
		c.collect({5, -1, 5});
		usleep(10000);
		c.summarize();
	}
	printf("Stopping...\n");
	c.stop();
	Json::Value results = c.results();
	Json::StyledWriter writer;
	std::string data = writer.write(results);
	printf("Results:\n%s", data.c_str());
	c.writeCSV_MTV("mtv.csv");
	c.writeCSV("excel.csv");
}

int main()
{
	srandom(time(NULL));

	test1();
	test2();
	test3();
	test4();
	test5();
	test6();
	test7(); // summarized results
	printf("ALL DONE!\n");
	return 0;
}
