#include <string>
#include <vector>
#include <EGL/egl.h>
#include <set>
#include <unordered_map>
#include "base/base.hpp"
#include "common/trace_model.hpp"
#include "common/out_file.hpp"
#include "tool/config.hpp"

using namespace std;

static void printVersion()
{
    cout << PATRACE_VERSION << endl;
}

static void printHelp(const char *argv0)
{
    cout << "Now this tool can remove some shader related useless calls.\n"
         << "Usage: " << argv0 << " <SOURCE> <TARGET>\n"
         << "Version: " << PATRACE_VERSION << "\n"
         << "Options:\n"
         << "  -v    Print version\n"
         << "  -h    Print help\n";
}

static void writeout(common::OutFile &outputFile, common::CallTM *call, int overrideFuncID)
{
    const unsigned int WRITE_BUF_LEN = 150*1024*1024;
    static char buffer[WRITE_BUF_LEN];

    char *dest = call->Serialize(buffer, overrideFuncID);
    outputFile.Write(buffer, dest-buffer);
}

map<int, string> AndroidImageCropAttribToNameMap;

void makeMap()
{
    AndroidImageCropAttribToNameMap[0x3148] = "EGL_IMAGE_CROP_LEFT_ANDROID";
    AndroidImageCropAttribToNameMap[0x3149] = "EGL_IMAGE_CROP_TOP_ANDROID";
    AndroidImageCropAttribToNameMap[0x314A] = "EGL_IMAGE_CROP_RIGHT_ANDROID";
    AndroidImageCropAttribToNameMap[0x314B] = "EGL_IMAGE_CROP_BOTTOM_ANDROID";
}

bool isAndroidImageCropAttrib(int attrib)
{
    // 0x3148 = EGL_IMAGE_CROP_LEFT_ANDROID
    // 0x3149 = EGL_IMAGE_CROP_TOP_ANDROID
    // 0x314A = EGL_IMAGE_CROP_RIGHT_ANDROID
    // 0x314B = EGL_IMAGE_CROP_BOTTOM_ANDROID
    // All of them need EGL_ANDROID_image_crop extension
    if (attrib >= 0x3148 && attrib <= 0x314B)
        return true;
    return false;
}

void process_eglCreateImageKHR(common::CallTM *call)        // remove EGL_ANDROID_image_crop related attribs
{
    vector<int> available_attrib_list;
    bool foundAndroidImageCropAttrib = false;
    for (unsigned int i = 0; i < call->mArgs[4]->mArrayLen; i += 2) {
        int attrib = call->mArgs[4]->mArray[i].GetAsInt();
        if (attrib != EGL_NONE) {
            if (!isAndroidImageCropAttrib(attrib)) {
                available_attrib_list.push_back(attrib);
                available_attrib_list.push_back(call->mArgs[4]->mArray[i + 1].GetAsInt());
            }
            else {
                foundAndroidImageCropAttrib = true;
                cout << "eglCreateImageKHR attrib[" << i << ", " << i + 1 <<  "] = (" << AndroidImageCropAttribToNameMap[attrib]
                     << ", " << call->mArgs[4]->mArray[i + 1].GetAsInt() << ") "
                     << "in Call " << call->mCallNo << " needs EGL_ANDROID_image_crop. "
                     << "Abandon it according to user's requirement." << endl;
            }
        }
        else {
            available_attrib_list.push_back(attrib);
            break;
        }
    }
    if (foundAndroidImageCropAttrib) {
        cout << endl;
    }
    call->mArgs[4]->mArrayLen = available_attrib_list.size();
    for (unsigned int i = 0; i < call->mArgs[4]->mArrayLen; ++i) {
        call->mArgs[4]->mArray[i] = common::ValueTM(available_attrib_list[i]);
    }
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printHelp(argv[0]);
        return -1;
    }

    int argIndex = 1;
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
        else
        {
            printf("Error: Unknown option %s\n", arg);
            printHelp(argv[0]);
            return 0;
        }
    }
    if (argIndex + 2 > argc)
    {
        printHelp(argv[0]);
        return 0;
    }

    makeMap();
    string source_name = argv[argIndex++];
    string target_name = argv[argIndex++];

    common::TraceFileTM source_file;
    if (!source_file.Open(source_name.c_str()))
    {
        PAT_DEBUG_LOG("Failed to open pat file %s for extracting.\n", source_name.c_str());
        return 1;
    }
    Json::Value json_value = source_file.mpInFileRA->getJSONHeader();
    Json::FastWriter header_writer;
    const std::string json_header = header_writer.write(json_value);

    vector<string> sigbook;
    unordered_map<string, unsigned int> NameToId;
    source_file.mpInFileRA->copySigBook(sigbook);
    for (vector<string>::size_type i = 0; i < sigbook.size(); ++i) {
        NameToId[sigbook[i]] = i;
    }

    /**********************************************************************/

    common::OutFile target_file;
    if (!target_file.Open(target_name.c_str(), true, &sigbook))
    {
        PAT_DEBUG_LOG("Failed to open pat file %s merging to.\n", target_name.c_str());
        return 1;
    }
    target_file.mHeader.jsonLength = json_header.size();
    target_file.WriteHeader(json_header.c_str(), json_header.size());

    common::CallTM *call = NULL;
    while ((call = source_file.NextCall())) {
        auto iter = NameToId.find(call->mCallName);
        int oldFuncId = -1;
        if (iter != NameToId.end()) {
            oldFuncId = iter->second;
        }
        else {
            PAT_DEBUG_LOG("Cannot find function (%s), %s, %d\n", call->mCallName.c_str(), iter->first.c_str(), iter->second);
        }

        if (call->mCallName == "eglCreateImageKHR") {
            process_eglCreateImageKHR(call);
        }
        writeout(target_file, call, oldFuncId);
    }

    source_file.Close();
    target_file.Close();
    cout << "eglCreateImageKHR attribs who needs EGL_ANDROID_image_crop extension have all been removed." << endl;

    return 0;
}
