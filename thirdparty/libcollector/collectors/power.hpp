#pragma once

#include <stdint.h>

#include <vector>

#include "interface.hpp"

class PowerDaemon
{
public:
    PowerDaemon();
    ~PowerDaemon();
    bool connectToDaemon(const std::string& ip, int port, int timeout);
    bool handshake(int timeoutInSeconds);
    bool start(const Json::Value& config);
    bool stop(const std::vector<int64_t>& timing, CollectorValueResults &results);

    bool disconnect();
    void setRawCollection(bool v);
    void setBufferSize(int v);

private:
    bool sendMsg(const char* str);
    int receiveMsg(char* buf, size_t len, int timeoutMs);
    bool receiveSyncMsg(char recva, char recvb, int timeoutMs);
    bool simpleSync(char senda, char sendb, char recva, char recvb, int timeoutMs);

    int mSocketFD;
    Json::Value mConfig;
    std::vector<std::string> mResultNames[3];
    bool mCollectRawData;
    unsigned mBufferSize; /* max buffer size reserved for raw power data, in MB */
};

class PowerDataCollector : public Collector
{
public:
    using Collector::Collector;

    virtual bool init() override;
    virtual bool deinit() override;

    virtual bool start() override;
    virtual bool postprocess(const std::vector<int64_t>& timing) override;
    virtual bool collect(int64_t) override;
    virtual bool available() override;

private:
    PowerDaemon mPD;
};
