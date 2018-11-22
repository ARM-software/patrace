#include <iostream>
#include <sys/stat.h>
#include "commonData.hpp"

using namespace std;

string value_type[19] = {
    "Unknown",
    "Void",
    "Int8",
    "Uint8",
    "Int16",
    "Uint16",
    "Int",
    "Enum",
    "Uint",
    "Int64",
    "Uint64",
    "Float",
    "String",
    "Array",
    "Blob",
    "Opaque",
    "Pointer",
    "MemRef",
    "Unused_Pointer",
};

string opaque_value_type[4] = {
    "BufferObjectReferenceType",
    "BlobType",
    "ClientSideBufferObjectReferenceType",
    "NoopType",
};

volatile int progress_num;

bool isDir(const string &path)
{
    struct stat buf;
    stat(path.c_str(), &buf);
    return S_ISDIR(buf.st_mode);
}

string filenameExtension(const string &name)
{
    int last_dot_idx = name.find_last_of('.');
    if (last_dot_idx < 0)
        return "";
    return name.substr(last_dot_idx + 1, name.length() - last_dot_idx);
}

void printProgressBar(int bar_width)
{
    float progress = static_cast<float>(progress_num) / 100;

    cout << "[";
    int pos = bar_width * progress;
    for (int i = 0; i < bar_width; ++i) {
        if (i < pos) cout << "=";
        else if (i == pos) cout << ">";
        else cout << " ";
    }
    cout << "] " << int(progress * 100.0) << " %\r";
    cout.flush();
}

void makeProgress(int counter, int total, bool forcePrint)
{
    int percentage = counter * 100 / total;
    if (progress_num != percentage || forcePrint) {
        progress_num = percentage;
        printProgressBar(50);
    }
}

void writeout(common::OutFile &outputFile, common::CallTM *call)
{
    const unsigned int WRITE_BUF_LEN = 150*1024*1024;
    static char buffer[WRITE_BUF_LEN];

    char *dest = call->Serialize(buffer);
    outputFile.Write(buffer, dest-buffer);
}

void GlesFilePath::setId()
{
    int last_slash_idx = rfind('/');
    string temp = substr(last_slash_idx, length() - last_slash_idx);
    int first_digit_id = temp.find_first_of("0123456789");
    if (first_digit_id < 0) {
        PAT_DEBUG_LOG("Found a gles calls json file without digital postfix: %s. This might be an error. This file won't be parsed.", c_str());
        *(static_cast<string *>(this)) = "";
        id = 0;
    }
    id = stoi(temp.substr(first_digit_id, temp.length() - first_digit_id));
}
