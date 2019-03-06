#include <newfastforwarder/parser.hpp>
#include "common/os_time.hpp"
#include "common/out_file.hpp"
#include "jsoncpp/include/json/reader.h"
#include "jsoncpp/include/json/writer.h"
#include "newfastforwarder/framestore.hpp"

extern bool retraceAndTrim(common::OutFile &, const CmdOptions& );

using namespace retracer;

const unsigned int FIRST_10_BIT = 1000000000;
const unsigned int NO_TEXTURE_IDX = 4294967295;

static void
usage(const char *argv0)
{
    fprintf(stderr,
        "Usage: %s [OPTIONS] input_trace\n"
        "Fastforward tracefile.\n"
        "\n"
        "  -fastforward <beginFrame> <endFrame> The frame number that should be fastforwarded to [REQUIRED]\n"
        "  -clearmask <mask> Decide mask to clear framebuffer. GL_COLOR_BUFFER_BIT = 16384; GL_DEPTH_BUFFER_BIT = 256; GL_STENCIL_BUFFER_BIT = 1024. The default value is 17664. \n"
        "  -outputname <name> specify the name of the fastforward file. The default name is fastforwardFile.pat \n"
        "\nfastforwardFile.pat in the execution path\n"
        "\n"
        , argv0);
}

int readValidValue(char* v)
{
    char* endptr;
    errno = 0;
    int val = strtol(v, &endptr, 10);
    if(errno)
    {
        perror("strtol");
        exit(1);
    }
    if(endptr == v || *endptr != '\0')
    {
        fprintf(stderr, "Invalid parameter value: %s\n", v);
        exit(1);
    }

    return val;
}

bool ParseCommandLine(int argc, char** argv, CmdOptions& cmdOpts)
{
    // Parse all except first (executable name)
    for (int i = 1; i < argc; ++i)
    {
        const char *arg = argv[i];

        // Assume last arg is filename
        if (i==argc-1 && arg[0] != '-')
        {
            cmdOpts.fileName = arg;
            break;
        }

        //for fastforwad
        if (!strcmp(arg, "-fastforward"))
        {
            cmdOpts.fastforwadFrame0 = readValidValue(argv[++i]);
            cmdOpts.fastforwadFrame1 = readValidValue(argv[++i]);
        }
        else if (!strcmp(arg, "-alldraws"))
        {
            cmdOpts.allDraws = true;
        }
        else if (!strcmp(arg, "-clearmask"))
        {
            cmdOpts.clearMask = readValidValue(argv[++i]);
        }
        else if (!strcmp(arg, "-outputname"))
        {
            cmdOpts.mOutputFileName = argv[++i];
        }
        else
        {
                usage(argv[0]);
                return false;
        }
    }

    return true;
}

extern "C"
int main(int argc, char** argv)
{
    long long start_time_system = os::getTime();

    //printf("Warnning: It's a testing version and not be released!\n");

    CmdOptions cmdOptions;
    if(!ParseCommandLine(argc, argv, cmdOptions))
    {
        return 1;
    }
    if(cmdOptions.fileName.empty())
    {
        std::cerr << "No trace file name specified.\n";
        usage(argv[0]);
        return 1;
    }

    // Register Entries before opening tracefile as sigbook is read there
    common::gApiInfo.RegisterEntries(gles_callbacks);
    common::gApiInfo.RegisterEntries(egl_callbacks);

    if ( !gRetracer.OpenTraceFile( cmdOptions.fileName.c_str() ))
    {
        DBG_LOG("Failed to open %s\n", cmdOptions.fileName.c_str() );
        return 1;
    }

    // 2. Now that tracefile is opened and defaults loaded, override
    if ( !gRetracer.overrideWithCmdOptions( cmdOptions ) )
    {
        DBG_LOG("Failed to override Cmd Options\n");
        return -1;
    }

    //    long long start_time = os::getTime();

    printf("FileFormatVersion %d\n", gRetracer.getFileFormatVersion());
    unsigned int FileFormatVersion = gRetracer.getFileFormatVersion();

    //pre-execution
    if(gRetracer.mOptions.allDraws == false)
    {
    gRetracer.Retrace();

    if(gRetracer.curPreExecuteState.endPreExecute == false){
        if(gRetracer.mCurFrameNo > gRetracer.mOptions.fastforwadFrame1)
        {
            gRetracer.curPreExecuteState.newInsertCallIntoList(false, NO_TEXTURE_IDX);
            gRetracer.curPreExecuteState.endPreExecute=true;
        }
    }

    gRetracer.preExecute=true;
    gRetracer.curPreExecuteState.finalTextureNoListDraw();
    gRetracer.curPreExecuteState.newFinalTextureNoListDraw();

    //gRetracer.curPreExecuteState.test();

    gRetracer.curContextState->clear();
    gRetracer.defaultContextState.clear();
    gRetracer.defaultContextJudge = false;
    gRetracer.multiThreadContextState.clear();
    gRetracer.curContextState = &(gRetracer.defaultContextState);
    gRetracer.curContextState->clear();
    gRetracer.curPreExecuteState.finalPotr = gRetracer.curPreExecuteState.finalTextureNoList.begin();

    gRetracer.curPreExecuteState.newFinalPotr = gRetracer.curPreExecuteState.newFinalTextureNoList.begin();

    if(*(gRetracer.curPreExecuteState.newFinalPotr) == 0)
    {
        gRetracer.curPreExecuteState.newFinalPotr++;
    }

    gRetracer.curPreExecuteState.beginDrawCallNo = *(gRetracer.curPreExecuteState.newFinalPotr)/FIRST_10_BIT;
    gRetracer.curPreExecuteState.endDrawCallNo = *(gRetracer.curPreExecuteState.newFinalPotr)%FIRST_10_BIT;
    //real-execution
    if ( !gRetracer.OpenTraceFile( cmdOptions.fileName.c_str() )) {
        DBG_LOG("Failed to open %s\n", cmdOptions.fileName.c_str() );
        return 1;
    }

    //printf("time 0.0 = %f\n", gRetracer.curPreExecuteState.timeCount);
    //printf("time 0.1 = %d\n", gRetracer.curPreExecuteState.loopTime);
    //printf("time 0.2 = %d\n", gRetracer.curPreExecuteState.drawCallNo);
    }

    gRetracer.preExecute = true;
    gRetracer.Retrace();
    //    long long end_time = os::getTime();
    //    float duration = static_cast<float>(end_time - start_time) / os::timeFrequency;
    //    printf("time 1 = %f\n", duration);
    //printf("read time = %f\n", gRetracer.timeCount);
    //printf("time 1.2 = %f\n", gRetracer.curFrameStoreState.timeCount);

    //cutter.......
    std::unordered_set<unsigned int>::iterator set_iter = gRetracer.callNoList.begin();
    while(set_iter != gRetracer.callNoList.end())
    {
        gRetracer.callNoListOut.insert(*set_iter);
        set_iter++;
    }

    printf("callNoList size  %ld\n", (long)gRetracer.callNoList.size());

    if(gRetracer.callNoList.size() == 0)
    {
        printf("no draw in this frame!!\n");
        exit(1);
    }
    else
    {
        printf("%ld gles apis in new pat file\n", (long)gRetracer.callNoList.size());
    }
    if ( !gRetracer.OpenTraceFile( cmdOptions.fileName.c_str() ))
    {
        DBG_LOG("Failed to open %s\n", cmdOptions.fileName.c_str() );
        return 1;
    }
    if ( !gRetracer.overrideWithCmdOptions( cmdOptions ) )
    {
        DBG_LOG("Failed to override Cmd Options\n");
        return -1;
    }


    common::OutFile out(cmdOptions.mOutputFileName.c_str());

    //    gRetracer.curContextState.curBufferState.readCurDeleteBuffers(gRetracer.callNoList);
    retraceAndTrim(out, cmdOptions);
    Json::Value jsonRoot = gRetracer.mFile.getJSONHeader();
    // Serialize header
    Json::FastWriter writer;
    std::string jsonData = writer.write(jsonRoot);
    out.mHeader.jsonLength = jsonData.size();//for version 3
    // Write header to file
    out.WriteHeader(jsonData.c_str(), jsonData.length());
    if(FileFormatVersion <= 5)
    {
        out.mHeader.version = 5;//version 3
    }

    // Close and cleanup
    out.Close();
    long long end_time_system = os::getTime();
    float duration_system = static_cast<float>(end_time_system - start_time_system) / os::timeFrequency;
    printf("time all = %f\n", duration_system);
    return 0;
}
