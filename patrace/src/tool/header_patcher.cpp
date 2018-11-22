#include <retracer/config.hpp> //version info
#include <common/file_format.hpp>

#include <jsoncpp/include/json/writer.h>
#include <jsoncpp/include/json/reader.h>

#include <fstream>
#include <iostream>
#include <string>

using namespace common;

static void
usage(const char *argv0) {
#if PATRACE_VERSION_PATCH
    fprintf(stderr,
        "Usage: %s <path_to_json> <path_to_trace_file>\n"
        "Version: r%dp%d.%d\n"
        "Pa-Trace Header patcher, works only tracefiles with version V3 or above.\n"
        "\n"
        , argv0, PATRACE_VERSION_MAJOR, PATRACE_VERSION_MINOR, PATRACE_VERSION_PATCH);
#else
    fprintf(stderr,
        "Usage: %s <path_to_json> <path_to_trace_file>\n"
        "Version: r%dp%d\n"
        "Pa-Trace Header patcher, works only tracefiles with version V3 or above.\n"
        "\n"
        , argv0, PATRACE_VERSION_MAJOR, PATRACE_VERSION_MINOR);
#endif
}

struct CmdOptions
{
    std::string fileName;
    std::string fileNameJson;
};

bool ParseCommandLine(int argc, char** argv, CmdOptions& cmdOpts )
{
    // Parse all except first (executable name)
    for (int i = 1; i < argc; ++i) {
        const char *arg = argv[i];

        // Assume last arg is filename
        if (i==argc-1 && arg[0] != '-') {
            cmdOpts.fileName = arg;
            break;
        }

        // Assume second last arg js-file
        if (i==argc-2) {
            cmdOpts.fileNameJson = arg;
            continue;
        }

        if (!strcmp(arg, "-js")) {
            cmdOpts.fileNameJson = argv[++i];
        } else if (!strcmp(arg, "--help") || !strcmp(arg, "-h")) {
            usage(argv[0]);
            return false;
        } else {
            DBG_LOG("error: unknown option %s\n", arg);
            usage(argv[0]);
            return false;
        }
    }

    return true;
}

void WriteHeader(std::fstream &mStream, BHeaderV3 header, const char* jsonCString)
{
    int jsonMaxLength = header.jsonFileEnd - header.jsonFileBegin;
    if ( jsonMaxLength >= 0 && header.jsonLength > static_cast<unsigned int>(jsonMaxLength) ) {
        DBG_LOG("Error: json file too long for header, %d > %d\n", header.jsonLength, jsonMaxLength);
        os::abort();
    } else {
        // Write JS Header
        mStream.seekp(header.jsonFileBegin, std::ios_base::beg);
        mStream.write(jsonCString,header.jsonLength);
        DBG_LOG("wrote json header,length=%d\n", header.jsonLength );

        // Write fixed-size header
        mStream.seekp(0, std::ios_base::beg);
        mStream.write((char*)&header, sizeof(BHeaderV3));
        mStream.flush();
    }
}

bool fileExists(std::string filepath) {
  std::ifstream f(filepath.c_str());
  return f.good();
}

bool modHeader(const std::string& mFileName, const std::string& fileNameJson)
{
    // 1. open file as read write
    // 2. check that its V3, if not exit
    // 3. read new js info from disk
    // 4. modify
    // 5. write trace
    // 6. close
    std::ios_base::openmode fmode = std::fstream::binary |
                                    std::fstream::in |
                                    std::fstream::out|
                                    std::fstream::ate ;

    std::fstream traceFileStream;
    traceFileStream.open(mFileName.c_str(), fmode);
    if (!traceFileStream.is_open()) {
        DBG_LOG("Failed to open file %s\n", mFileName.c_str());
        return false;
    }

    size_t bytesAte = traceFileStream.tellg();
    DBG_LOG("%d kB trace file\n", (int) bytesAte/1024);
    traceFileStream.seekp(0, std::ios_base::beg);

    // Read Base Header that is common for all header versions
    common::BHeader bHeader;
    traceFileStream.read((char*)&bHeader, sizeof(bHeader));
    if (bHeader.magicNo != 0x20122012) {
        DBG_LOG("Warning: %s seems to be an invalid trace file!\n", mFileName.c_str());
        return false;
    }


    if (bHeader.version >= HEADER_VERSION_3 || bHeader.version <= HEADER_VERSION_4) {
        DBG_LOG("### .pat file format Version %d ###\n", bHeader.version - HEADER_VERSION_1 + 1);
    } else {
        DBG_LOG("Unsupported file version: %d\n", bHeader.version - HEADER_VERSION_1 + 1);
        return false;
    }

    // Read new JSON from File
    Json::Value mJsonHeader;
    std::ifstream jsonFileStream(fileNameJson.c_str());
    Json::Reader reader;
    bool parsingSuccessful = reader.parse( jsonFileStream, mJsonHeader );
    if ( !parsingSuccessful )
    {
        // report to the user the failure and their locations in the document.
        std::cerr  << "Failed to parse configuration\n"
                   << reader.getFormattedErrorMessages();
        return false;
    }

    // Write!
    Json::FastWriter fastWriter;
    std::string jsonData = fastWriter.write(mJsonHeader);
    BHeaderV3 hdrV3;
    traceFileStream.seekp(0, std::ios_base::beg);
    traceFileStream.read((char*)&hdrV3, sizeof(BHeaderV3));

    hdrV3.jsonLength = jsonData.length();

    if ( hdrV3.jsonLength > 0) {
        WriteHeader(traceFileStream, hdrV3, jsonData.c_str());
    } else {
        DBG_LOG("Error: no jsonData to write\n");
        return false;
    }

    traceFileStream.close();
    return true;
}

int main(int argc, char** argv)
{
    CmdOptions cmdOptions;
    if(!ParseCommandLine(argc, argv, cmdOptions)) {
        return 1;
    }

    if (cmdOptions.fileName.empty() || cmdOptions.fileNameJson.empty()) {
        usage(argv[0]);
        return 1;
    }

    if(!fileExists(cmdOptions.fileName) ) {
        std::cerr << "Specified trace does not exist\n";
        return 1;
    }

    if(!fileExists(cmdOptions.fileNameJson) ) {
        std::cerr << "Specified json does not exist\n";
        return 1;
    }

    if ( modHeader(cmdOptions.fileName, cmdOptions.fileNameJson) ) {
        const char* greenOnBlack = "\033[32;40m";
        const char* resetColor = "\033[00;00m";
        printf("%sMod tool succeeded!\n%s", greenOnBlack, resetColor);
    } else {
        printf("Mod tool failed.\n");
        return 1;
    }

    return 0;
}


