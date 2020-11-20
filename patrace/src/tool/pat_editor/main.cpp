#include <string>
#include <sys/stat.h>
#include <iostream>
#include <limits>
#include "tool/config.hpp"
#include "common/os_time.hpp"
#include "eglstate/common.hpp"
#include "commonData.hpp"

using namespace std;

bool isFile(const string &name)
{
    struct stat buf;
    return (stat(name.c_str(), &buf) == 0);
}

static void printVersion()
{
    cout << PATRACE_VERSION << endl;
}

static void printHelp(const char *argv0)
{
    cout << "Usage: " << argv0 << " [OPTIONS] <SOURCE> [TARGET]\n"
         << "Version: " << PATRACE_VERSION << "\n"
         << "Options:\n"
         << "  -e : Extract OpenGL ES calls, textures, shaders, data, etc. from a SOURCE file to a TARGET directory,\n"
         << "  -m : Merge OpenGL ES calls, textures, shaders, data, etc. in a SOURCE directory to a TARGET file.\n"
         << "  -v : Print version\n"
         << "  -h : Print help\n"
         << "  -call BEGIN_CALL END_CALL : Specify the call range user wants to extract.\n";
}

int pat_extract(const string &source_name, const string &target_name, int begin_call = 0, int end_call = numeric_limits<int>::max());
int merge_to_pat(const string &source_name, const string &target_name);

enum Operation {
    UNKNOWN_OPERATION = 0,
    EXTRACT,
    MERGE
};

int main(int argc, char **argv)
{
    long long start_time = os::getTime();
    if (argc < 2)
    {
        printHelp(argv[0]);
        return -1;
    }

    int argIndex = 1;
    int begin_call = 0, end_call = numeric_limits<int>::max();
    Operation operation = UNKNOWN_OPERATION;
    for (; argIndex < argc; ++argIndex)
    {
        const char *arg = argv[argIndex];

        if (arg[0] != '-')
            break;

        if (!strcmp(arg, "-h"))
        {
            printHelp(argv[0]);
            return 0;
        }
        else if (!strcmp(arg, "-v"))
        {
            printVersion();
            return 0;
        }
        else if (!strcmp(arg, "-e"))
        {
            operation = EXTRACT;
        }
        else if (!strcmp(arg, "-m"))
        {
            operation = MERGE;
        }
        else if (!strcmp(arg, "-call"))
        {
            if (argIndex + 2 < argc)
            {
                try {
                    begin_call = stoi(argv[argIndex + 1]);
                }
                catch(const invalid_argument&) {
                    cout << "Error: the 1st parameter of call range is invalid, -call option needs two integer parameters." << endl;
                    return 1;
                }
                try {
                    end_call = stoi(argv[argIndex + 2]);
                }
                catch(const invalid_argument&) {
                    cout << "Error: the 2nd parameter of call range is invalid, -call option needs two integer parameters." << endl;
                    return 1;
                }
                argIndex += 2;
            }
            else
            {
                cout << "Error: -call option needs two integer parameters" << endl;
                printHelp(argv[0]);
                return 0;
            }
        }
        else
        {
            cout << "Error: Unknown option " << arg << endl;
            printHelp(argv[0]);
            return 0;
        }
    }
    if (argIndex + 1 > argc)
    {
        printHelp(argv[0]);
        return 0;
    }
    if (operation == UNKNOWN_OPERATION)
    {
        cout << "The operation is unknown!" << endl;
        printHelp(argv[0]);
        return 0;
    }

    string source_name = argv[argIndex++];
    string target_name;
    if (argIndex != argc)
        target_name = argv[argIndex++];
    else {                              // User didn't specify the target pat file or target directory
        if (operation == EXTRACT) {     // We are going to extract a pat file to a directory
            int last_slash_idx = source_name.find_last_of('/');
            int last_dot_idx = source_name.find_last_of('.');
            target_name = source_name.substr(last_slash_idx + 1, last_dot_idx - last_slash_idx - 1) + "_data";
        }
        else {                          // We are going to merge a directory to a pat file
            int last_not_slash_idx = source_name.find_last_not_of('/');
            target_name = source_name.substr(0, last_not_slash_idx + 1);
            if (target_name.substr(target_name.length() - 5, 5) == "_data") // remove postfix "_data"
                target_name = target_name.substr(0, target_name.length() - 5);
            int last_slash_idx = target_name.find_last_of('/');
            target_name = target_name.substr(last_slash_idx + 1, target_name.length() - last_slash_idx - 1) + "_modified.pat";
        }
    }

    if (operation == EXTRACT) {
        if (!isFile(source_name)) {  
            cout << source_name << " is not a pat file!" << endl;
            return 1;
        }
        if (pat_extract(source_name, target_name, begin_call, end_call) != 0)
            return 1;
    }
    else {
        if (!isDir(source_name)) {
            cout << source_name << " is not a directory!" << endl;
            return 1;
        }
        if (merge_to_pat(source_name, target_name) != 0)
            return 1;
    }
    long long end_time = os::getTime();
    float duration = static_cast<float>(end_time - start_time) / os::timeFrequency;
    cout << "duration = " << duration << "s" << endl;
    return 0;
}
