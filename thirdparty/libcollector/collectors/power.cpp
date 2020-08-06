#include "power.hpp"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <fcntl.h>
#include <poll.h>
#ifdef ANDROID
/* work around error "'IN6ADDR_LOOPBACK_INIT' was not declared in this scope" that occurs on ndk-r7. */
#define IN6ADDR_LOOPBACK_INIT { { { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 } } }
#endif
#include <arpa/inet.h>
#include <netdb.h>

#include <map>

// assume IEEE-754
typedef double float64;

#define TOTAL_POWER_TRUNC_COEFFICIENT 1000000.0

// assume 8-bit char
struct Rail
{
    char name[16];
    char voltageChannel[16];
    char currentChannel[16];
    uint32_t resistance;
    uint32_t reserved[3];
    char daqSerial[256];
};

PowerDaemon::PowerDaemon()
    : mSocketFD(-1)
    , mCollectRawData(false)
    , mBufferSize(120)
{
}

PowerDaemon::~PowerDaemon()
{
}

bool PowerDaemon::connectToDaemon(const std::string& ip, int port, int timeout)
{
    struct addrinfo hostInfo;
    memset(&hostInfo, 0, sizeof hostInfo);
    hostInfo.ai_family = AF_UNSPEC;
    hostInfo.ai_socktype = SOCK_STREAM;
    struct addrinfo *result;
    struct addrinfo *rp;
    int ret;

    bool successfullyConnected = false;

    char portbuf[16];
    snprintf(portbuf, 15, "%d", port);
    portbuf[15] = 0;

    ret = getaddrinfo(ip.c_str(), portbuf, &hostInfo, &result);
    if (ret != 0)
    {
        DBG_LOG("Power data collector: error on getaddrinfo(): %s\n", strerror(errno));
        return false;
    }
    for (rp = result; rp != NULL; rp = rp->ai_next)
    {
        mSocketFD = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (mSocketFD == -1)
        {
            DBG_LOG("Power data collector: error on socket(): %s\n", strerror(errno));
            continue;
        }

        int flags = fcntl(mSocketFD, F_GETFL, 0);
        if (flags == -1)
        {
            DBG_LOG("Power data collector: error on fcntl(F_GETFL): %s\n", strerror(errno));
            if (close(mSocketFD) != 0)
            {
                perror("close");
            }
            continue;
        }
        ret = fcntl(mSocketFD, F_SETFL, flags | O_NONBLOCK);
        if (ret != 0)
        {
            DBG_LOG("Power data collector: error on fcntl(F_SETFL): %s\n", strerror(errno));
            if (close(mSocketFD) != 0)
            {
                perror("close");
            }
            continue;
        }

        ret = connect(mSocketFD, result->ai_addr, result->ai_addrlen);
        if (ret == 0)
        {
            // connected (we probably never get here due to non-blocking)
            successfullyConnected = true;
            break;
        }
        else
        {
            if (errno == EINPROGRESS)
            {
                // wait for connection until timeout
                struct pollfd fds[1];
                fds[0].fd = mSocketFD;
                fds[0].events = POLLOUT;
                fds[0].revents = 0;
                ret = poll(fds, 1, timeout * 1000);
                if (ret == -1)
                {
                    DBG_LOG("Power data collector: error on poll() during connect: %s\n", strerror(errno));
                    if (close(mSocketFD) != 0)
                    {
                        perror("close");
                    }
                }
                else if (ret == 0)
                {
                    DBG_LOG("Power data collector: timeout when connecting.\n");
                    if (close(mSocketFD) != 0)
                    {
                        perror("close");
                    }
                }
                else
                {
                    // check the connection
                    int error = 0;
                    socklen_t len = sizeof(error);
                    ret = getsockopt(mSocketFD, SOL_SOCKET, SO_ERROR, &error, &len);
                    if (ret != 0)
                    {
                        DBG_LOG("Power data collector: error on getsockopt(): %s.\n", strerror(errno));
                        if (close(mSocketFD) != 0)
                        {
                            perror("close");
                        }
                    }
                    else
                    {
                        if (error == 0)
                        {
                            successfullyConnected = true;
                        }
                        else
                        {
                            DBG_LOG("Power data collector: getsockopt reports error: %s.\n", strerror(error));
                            if (close(mSocketFD) != 0)
                            {
                                perror("close");
                            }
                        }
                    }
                }
                break;
            }
            else
            {
                DBG_LOG("Power data collector: error on connect(): %s.\n", strerror(errno));
                if (close(mSocketFD) != 0)
                {
                    perror("close");
                }
            }
        }
    }
    freeaddrinfo(result);

    if (!successfullyConnected)
    {
        mSocketFD = -1;
    }

    return successfullyConnected;
}

bool PowerDaemon::handshake(int timeoutSeconds)
{
    return simpleSync('H', '2', 'O', 'K', timeoutSeconds * 1000 /*convert to MS*/);
}

bool PowerDaemon::start(const Json::Value& config_root)
{
    if (!sendMsg("RC"))
        return false;

    Json::Value config = config_root["rails"];

    uint32_t numRails = uint32_t(config.size());
    uint32_t d = htonl(numRails);
    DBG_LOG("Sending rail configuration for %d rails\n", numRails);
    if (write(mSocketFD, &d, sizeof(d)) != sizeof(d))
    {
        DBG_LOG("Can't write: %s\n", strerror(errno));
        return false;
    }

    std::string serial;
    if (config_root.isMember("daq_serial"))
        serial = config_root["daq_serial"].asString();

    for (unsigned int i = 0; i < config.size(); i++)
    {
        Rail rail;
        memset(&rail, 0x00, sizeof(rail));
        snprintf(rail.name, 15, "%s", config[i]["name"].asCString());
        snprintf(rail.voltageChannel, 15, "%s", config[i]["voltage_channel"].asCString());
        snprintf(rail.currentChannel, 15, "%s", config[i]["current_channel"].asCString());
        rail.resistance = htonl(config[i]["resistance"].asInt());
        if (!serial.empty())
            snprintf(rail.daqSerial, 255, "%s", serial.c_str());
        if (write(mSocketFD, &rail, sizeof(rail)) != sizeof(rail))
        {
            DBG_LOG("Can't write: %s\n", strerror(errno));
            return false;
        }
    }

    mConfig = config;

    /* use long timeouts here due to NI-DAQ initialisation */
    if (!receiveSyncMsg('O', 'K', 15000))
    {
        return false;
    }

    /* issue start */
    return simpleSync('R', 'T', 'O', 'K', 15000);
}

bool PowerDaemon::stop(const std::vector<int64_t>& timing, CollectorValueResults &results)
{
    // check if the daemon has an error to report
    char recvbuf[2];

    if (receiveMsg(recvbuf, 2, 10) == 2)
    {
        DBG_LOG("Power data collector: message %c%c from client during measurement.\n", recvbuf[0], recvbuf[1]);
        if (!strncmp(recvbuf, "ER", 2))
        {
            return false;
        }
    }

    // large timeout due to daemon blocking during sampling
    if (!simpleSync('O', 'P', 'O', 'K', 3000))
        return false;

    uint32_t numBytes;
    if (receiveMsg(reinterpret_cast<char*>(&numBytes), sizeof(numBytes), 100) != 4)
    {
        return false;
    }

    numBytes = ntohl(numBytes);
    DBG_LOG("Power data collector: %d bytes of measurement data.\n", numBytes);

    if (numBytes > 1024 * 1024 * mBufferSize)
    {
        DBG_LOG("Power data collector: too much data - discarding. Allocated %d MB but would receive %d MB (you can change the allocation at instrumentation_config.buffer_size).\n",
                mBufferSize, numBytes / 1048576);
        return false;
    }

    float64* buf = (float64*)malloc(numBytes);
    if (!buf)
    {
        DBG_LOG("Power data collector: error on malloc().\n");
        return false;
    }

    int ret = receiveMsg(reinterpret_cast<char*>(buf), numBytes, 4000);
    if (ret != (int)numBytes)
    {
        DBG_LOG("Power data collector: Received %d/%d bytes.\n", ret, numBytes);
        free(buf);
        return false;
    }

    std::vector<float64> railResistorCoefficients;

    for (unsigned int i = 0; i < mConfig.size(); i++)
    {
        railResistorCoefficients.push_back(1000.0 / mConfig[i]["resistance"].asInt());
        DBG_LOG("Rail %d resistor coefficient = %3.3f\n", i, railResistorCoefficients[i]);
    }

    unsigned int totalSamples = numBytes / sizeof(float64);
    unsigned int numRails = railResistorCoefficients.size();

    DBG_LOG("Got %u bytes of data (%u samples or %3.3f s) from the daemon.\n", numBytes,
            totalSamples, (float)totalSamples / (10000 * 2 * numRails));

    std::vector<int64_t> frameDurations;
    int64_t prev = 0;
    for (const auto i : timing)
    {
        frameDurations.push_back(i - prev);
        prev = i;
    }

    unsigned int currentTime = 0; // unit = one sample (100 us)

    long long accountedv[numRails];
    long long accountedi[numRails];
    long long accountedp[numRails];
    memset(accountedv, 0x00, sizeof(accountedv));
    memset(accountedi, 0x00, sizeof(accountedi));
    memset(accountedp, 0x00, sizeof(accountedp));
    unsigned int accountedSamples = 0;

    double nextFrameEndTime = 0;
    for (const auto i : frameDurations)
    {
        nextFrameEndTime += i;
        unsigned int nextFrameEndTimeInSamples = (unsigned int)(nextFrameEndTime * 10000);
        unsigned int samples = 0;
        int64_t framev[numRails];
        int64_t framei[numRails];
        int64_t framep[numRails];
        memset(framev, 0x00, sizeof(framev));
        memset(framei, 0x00, sizeof(framei));
        memset(framep, 0x00, sizeof(framep));

        while (currentTime < nextFrameEndTimeInSamples)
        {
            for (unsigned int i = 0; i < numRails; i++)
            {
                if (currentTime * numRails * 2 + i * 2 + 1 >= totalSamples)
                {
                    // no more data
                    DBG_LOG("Power data stopped short.\n");
                    goto finished;
                }
                // not converting big<->little endian here for now
                double voltage = buf[currentTime * numRails * 2 + i * 2];
                double shuntVoltage = buf[currentTime * numRails * 2 + i * 2 + 1];
                double power = voltage * shuntVoltage * railResistorCoefficients[i];
                framev[i] += (int64_t)(voltage * TOTAL_POWER_TRUNC_COEFFICIENT);
                framei[i] += (int64_t)(shuntVoltage * railResistorCoefficients[i] * TOTAL_POWER_TRUNC_COEFFICIENT);
                framep[i] += (int64_t)(power * TOTAL_POWER_TRUNC_COEFFICIENT);
            }
            currentTime++;
            samples++;
        }

        if (samples)
        {
            results["num_samples"].push_back(samples);
            for (unsigned int i = 0; i < numRails; i++)
            {
                std::string railName = mConfig[i]["name"].asString();
                double avgv = (framev[i] / samples) / TOTAL_POWER_TRUNC_COEFFICIENT;
                double avgi = (framei[i] / samples) / TOTAL_POWER_TRUNC_COEFFICIENT;
                double avgp = (framep[i] / samples) / TOTAL_POWER_TRUNC_COEFFICIENT;
                results[railName + "_voltage"].push_back(avgv * 1000.0);
                results[railName + "_current"].push_back(avgi * 1000.0);
                results[railName + "_power"].push_back(avgp * 1000.0);
                accountedv[i] += framev[i];
                accountedi[i] += framei[i];
                accountedp[i] += framep[i];
            }
            accountedSamples += samples;
        }
    }

#if 0
    if (accountedSamples)
    {
        for (unsigned int i = 0; i < numRails; i++)
        {
            float avgv = (accountedv[i] / accountedSamples) / (float)TOTAL_POWER_TRUNC_COEFFICIENT;
            float avgi = (accountedi[i] / accountedSamples) / (float)TOTAL_POWER_TRUNC_COEFFICIENT;
            float avgp = (accountedp[i] / accountedSamples) / (float)TOTAL_POWER_TRUNC_COEFFICIENT;
            TraceExecutor::addResult(mResultNames[0][i].c_str(), avgv * 1000.0f);
            TraceExecutor::addResult(mResultNames[1][i].c_str(), avgi * 1000.0f);
            TraceExecutor::addResult(mResultNames[2][i].c_str(), avgp * 1000.0f);
        }
    }
#endif

    DBG_LOG("%d extra samples (%3.3f s) of power data.\n", totalSamples / (2 * numRails) - currentTime,
            (totalSamples / (2 * numRails) - currentTime) / 10000.0);

finished:

#if 0
    if (mCollectRawData)
    {
        if (!TraceExecutor::addResultFile("power.raw", (const char*)buf, numBytes, false))
        {
            DBG_LOG("Unable to add raw power data to the result!\n");
            free(buf);
            return false;
        }
        else
        {
            DBG_LOG("Successfully added raw power data to the result.\n");
        }
    }
#endif

    free(buf);
    return true;
}

bool PowerDaemon::disconnect()
{
    if (mSocketFD != -1)
    {
        if (close(mSocketFD) != 0)
        {
            DBG_LOG("Power data collector: error on close(): %s.\n", strerror(errno));
        }
        mSocketFD = -1;
    }
    return true;
}

bool PowerDaemon::sendMsg(const char* str)
{
    int len = strlen(str);
    int ret = write(mSocketFD, str, len);

    if (ret != len)
    {
        DBG_LOG("Power data collector: write %d/%d (%s)\n", ret, len, strerror(errno));
        return false;
    }

    return true;
}

int PowerDaemon::receiveMsg(char* buf, size_t len, int timeoutMs)
{
    struct pollfd fds[1];
    fds[0].fd = mSocketFD;
    fds[0].events = POLLIN;
    fds[0].revents = 0;
    unsigned int totalRead = 0;
    int ret;

    while (totalRead < len)
    {
        ret = poll(fds, 1, timeoutMs);
        if (ret == -1)
        {
            DBG_LOG("Power data collector on receive: poll returns %d (%s)\n", ret, strerror(errno));
            return ret;
        }
        else if (ret == 0)
        {
            DBG_LOG("Power data collector on receive: poll returns %d after %d ms timeout\n", ret, timeoutMs);
            return totalRead;
        }

        if (fds[0].revents & (POLLERR | POLLHUP | POLLNVAL))
        {
            if (fds[0].revents & POLLERR)
            {
                DBG_LOG("Power data collector on receive: poll returns POLLERR\n");
            }
            if (fds[0].revents & POLLHUP)
            {
                DBG_LOG("Power data collector on receive: poll returns POLLHUP\n");
            }
            if (fds[0].revents & POLLNVAL)
            {
                DBG_LOG("Power data collector on receive: poll returns POLLNVAL\n");
            }
            return -1;
        }

        size_t toRead = len - totalRead;
        if (toRead > 1024 * 64)
            toRead = 1024 * 64;

        ret = read(mSocketFD, &buf[totalRead], toRead);
        if (ret == -1 && errno != EAGAIN && errno != EWOULDBLOCK)
        {
            DBG_LOG("Power data collector: read returns %s\n", strerror(errno));
            return ret;
        }
        if (ret == 0)
        {
            DBG_LOG("Power data collector: peer has closed the connection.\n");
            return -1;
        }

        totalRead += ret;
    }
    return totalRead;
}

bool PowerDaemon::receiveSyncMsg(char recva, char recvb, int timeoutMs)
{
    char buf[2];
    if (receiveMsg(buf, 2, timeoutMs) != 2)
    {
        return false;
    }

    DBG_LOG("Power data collector: sync gives %c%c\n",
            buf[0], buf[1]);

    if (buf[0] == recva && buf[1] == recvb)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool PowerDaemon::simpleSync(char senda, char sendb, char recva, char recvb, int timeoutMs)
{
    char sendbuf[3];
    sendbuf[0] = senda;
    sendbuf[1] = sendb;
    sendbuf[2] = 0;
    if (!sendMsg(sendbuf))
    {
        return false;
    }

    DBG_LOG("Power data collector: sent sync %c%c\n", senda, sendb);

    return receiveSyncMsg(recva, recvb, timeoutMs);
}

void PowerDaemon::setRawCollection(bool v)
{
    mCollectRawData = v;
}

void PowerDaemon::setBufferSize(int v)
{
    mBufferSize = v;
}

// -------------------------------------

bool PowerDataCollector::available()
{
    if (mConfig.isNull() || mConfig["power"].isNull())
    {
        if (mDebug) DBG_LOG("Power data collector: configuration missing, cannot init.\n");
        return false;
    }
    return true;
}

bool PowerDataCollector::init()
{
    assert(!mConfig.isNull());
    assert(!mConfig["power"].isNull());

    Json::Value config = mConfig["power"];
    std::string pdip;
    int pdport = 0;
    int connectTimeoutSec = 5;

    if (config["power_daemon_ip"].isString())
    {
        pdip = config["power_daemon_ip"].asString();
        if (config["power_daemon_port"].isString())
        {
            pdport = atoi(config["power_daemon_port"].asCString());
        }
    }

    if (!config["rails"].isArray() || config["rails"].size() == 0)
    {
        DBG_LOG("Power data collector: rail configuration missing, cannot init.\n");
        return false;
    }

    for (unsigned int i = 0; i < config["rails"].size(); i++)
    {
        if (!config["rails"][i]["name"].isString())
        {
            DBG_LOG("Power data collector: rail configuration invalid (rail[%d][name] is not a string, cannot init.\n", i);
            return false;
        }
        if (!config["rails"][i]["voltage_channel"].isString())
        {
            DBG_LOG("Power data collector: rail configuration invalid (rail[%d][voltage_channel] is not a string, cannot init.\n", i);
            return false;
        }
        if (!config["rails"][i]["current_channel"].isString())
        {
            DBG_LOG("Power data collector: rail configuration invalid (rail[%d][current_channel] is not a string, cannot init.\n", i);
            return false;
        }
        if (!config["rails"][i]["resistance"].isInt())
        {
            DBG_LOG("Power data collector: rail configuration invalid (rail[%d][resistance] is not an int, cannot init.\n", i);
            return false;
        }
    }

    if (config["connect_timeout"].isInt())
    {
        connectTimeoutSec = config["connect_timeout"].asInt();
    }

    if (pdip.empty() || !pdport)
    {
        DBG_LOG("Power data collector: invalid configuration (IP: %s, port: %d), cannot init.\n",
                pdip.c_str(), pdport);
        return false;
    }

    if (!mPD.connectToDaemon(pdip, pdport, connectTimeoutSec))
    {
        DBG_LOG("Power data collector: could not connect.\n");
        return false;
    }

    // If the server is busy measuring another device, connectToDaemon() will
    // succeed but the handshake will hang until it's our turn.
    // handshakeTimeoutSec is how long we're willing to wait.
    int handshakeTimeoutSec = connectTimeoutSec;

    if (!mPD.handshake(handshakeTimeoutSec))
    {
        DBG_LOG("Power data collector: could not perform handshake.\n");
        mPD.disconnect();
        return false;
    }

    if (config["collect_raw_data"].isBool() && config["collect_raw_data"].asBool() == true)
    {
        mPD.setRawCollection(true);
    }

    if (config["buffer_size"].isInt())
    {
        mPD.setBufferSize(config["buffer_size"].asInt());
    }

    mConfig = config;

    return true;
}

bool PowerDataCollector::deinit()
{
    mPD.disconnect();
    return true;
}

bool PowerDataCollector::start()
{
    if (!mPD.start(mConfig["power"]))
    {
        DBG_LOG("Power data collector: could not start measurement.\n");
        return false;
    }
    mCollecting = true;
    return true;
}

bool PowerDataCollector::postprocess(const std::vector<int64_t>& timing)
{
    if (!mPD.stop(timing, mResults))
    {
        DBG_LOG("Power data collector: could not stop measurement.\n");
    }
    return true;
}

bool PowerDataCollector::collect(int64_t /* now */)
{
    return true;
}
