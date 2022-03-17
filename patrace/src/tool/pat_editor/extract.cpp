#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <GLES3/gl32.h>
#include <dirent.h>
#include <limits>
#include <sys/stat.h>
#include "commonData.hpp"
#include "eglstate/common.hpp"
#include "base/base.hpp"

const unsigned int CALL_BATCH_SIZE = 1000000;

using namespace std;

string intToString(int a, int width = 0)
{
    ostringstream oss;
    oss << setfill('0') << setw(width) << a;
    return oss.str();
}

string genIdName(int index, const string &name, int width = 0)
{
    return intToString(index, width) + " " + name;
}

void deleteAllFiles(const string &path)
{
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(path.c_str())) != NULL)
    {
        while ((ent = readdir(dir)) != NULL)
        {
            if (ent->d_name[0] == '.') {
                continue;
            }
            string fullname = path + "/" + ent->d_name;
            if (isDir(fullname)) {
                deleteAllFiles(fullname);
            }
            else if (filenameExtension(ent->d_name) == "json" ||
                    filenameExtension(ent->d_name) == "bin" ||
                    filenameExtension(ent->d_name) == "txt")
            {
                if (remove(fullname.c_str()) != 0)
                    perror(("Error when deleting file " + fullname).c_str());
            }
        }
    }
}

int makeExtractDir(const string &target_name)
{
    if (!isDir(target_name))
    {
        int dir_err = mkdir(target_name.c_str(), 0755);
        if (dir_err == -1)
        {
            PAT_DEBUG_LOG("Failed to create resulting directory: %s\n", target_name.c_str());
            return 1;
        }
    }
    else
    {
        cout << "Target directory \"" << target_name
             << "\" already exist. Deleting all its contents..." << endl;
        deleteAllFiles(target_name);
    }

    string subpath[5] = { "/blob", "/texture", "/shader", "/GLES_calls", "/not_interested_in" };
    for (int i = 0; i < 5; ++i) {
        if (!isDir(target_name + subpath[i])) {
            int dir_err = mkdir((target_name + subpath[i]).c_str(), 0755);
            if (dir_err == -1)
            {
                PAT_DEBUG_LOG("Failed to create resulting directory: %s\n", (target_name + subpath[i]).c_str());
                return 1;
            }
        }
    }
    return 0;
}

int gCallNo_width;
string target_name;

int current_resource_id;
map<unsigned int, unsigned int> target_id_map;

long callId;
int file_counter;

bool open_frame_file(ofstream &fout, bool no_frame_file_opened, int width)
{
    if (no_frame_file_opened) {
        fout.open(target_name + "/GLES_calls/frame_" + intToString(file_counter, width) + ".json");
        if (!fout.is_open()) {
            PAT_DEBUG_LOG("Failed to open file %s when extracting\n", (target_name + "/GLES_calls/frame_" + intToString(file_counter, width) + ".json").c_str());
            return false;
        }
    }
    return true;
}

void setJsonValue(Json::Value &json_value, const string &s, const common::ValueTM &value, const common::CallTM *call, bool append)
{
    switch(value.mType) {
        case common::Void_Type:
            if (append) json_value[s].append(Json::nullValue);
            else        json_value[s] = Json::nullValue;
            break;
        case common::Int8_Type:
            if (append) json_value[s].append(value.mInt8);
            else        json_value[s] = value.mInt8;
            break;
        case common::Uint8_Type:
            if (append) json_value[s].append(value.mUint8);
            else        json_value[s] = value.mUint8;
            break;
        case common::Int16_Type:
            if (append) json_value[s].append(value.mInt16);
            else        json_value[s] = value.mInt16;
            break;
        case common::Uint16_Type:
            if (append) json_value[s].append(value.mUint16);
            else        json_value[s] = value.mUint16;
            break;
        case common::Int_Type:
            if (call->Name() == "glSamplerParameteri" ||
                call->Name() == "glTexParameteri" ||
                call->Name() == "glTexEnvx" ||
                call->Name() == "glTexParameterx") // special cases
            {
                GLenum pname = call->mArgs[1]->GetAsUInt();
                if (pname != GL_TEXTURE_MAX_LOD && pname != GL_TEXTURE_MIN_LOD &&
                    pname != GL_TEXTURE_BASE_LEVEL && pname != GL_TEXTURE_MAX_LEVEL)
                {
                    const char *enumTmp = EnumString(value.mInt, call->Name());
                    if (enumTmp == NULL) {
                        if (append) json_value[s].append(value.mInt);
                        else        json_value[s] = value.mInt;
                    }
                    else {
                        if (append) json_value[s].append(enumTmp);
                        else        json_value[s] = enumTmp;
                    }
                    break;
                }
            }
            if (append) json_value[s].append(value.mInt);
            else        json_value[s] = value.mInt;
            break;
        case common::Enum_Type: {
            const char *enumTmp = EnumString(value.mEnum, call->Name());
            if (enumTmp == NULL) {
                if (append) json_value[s].append(value.mEnum);
                else        json_value[s] = value.mEnum;
            }
            else {
                if (append) json_value[s].append(enumTmp);
                else        json_value[s] = enumTmp;
            }
            break;
        }
        case common::Uint_Type:
            if (append) json_value[s].append(value.mUint);
            else        json_value[s] = value.mUint;
            break;
        case common::Int64_Type:
            if (append) json_value[s].append((Json::Value::Int64)value.mInt64);
            else        json_value[s] = (Json::Value::Int64)value.mInt64;
            break;
        case common::Uint64_Type:
            if (append) json_value[s].append((Json::Value::UInt64)value.mUint64);
            else        json_value[s] = (Json::Value::UInt64)value.mUint64;
            break;
        case common::Float_Type: {
            float v = value.mFloat;
            float result;
            bool abnormal = false;
            string result_string;
            int sign_bit = 0x80000000;
            int *p = reinterpret_cast<int *>(&v);
            // NaN or INF would break the JSON parsing when merging the materials into a pat file
            // So store them as strings
            if (isnan(v) && !(sign_bit & (*p))) {
                abnormal = true;
                result_string = "nan";
            }
            else if (isnan(v) && (sign_bit & (*p))) {
                abnormal = true;
                result_string = "-nan";
            }
            else if (isinf(v) && v > 0.0f) {
                abnormal = true;
                result_string = "inf";
            }
            else if (isinf(v) && v < 0.0f) {
                abnormal = true;
                result_string = "-inf";
            }
            else
                result = v;

            if (abnormal) {
                if (append) json_value[s].append(result_string);
                else        json_value[s] = result_string;
            }
            else {
                if (append) json_value[s].append(result);
                else        json_value[s] = result;
            }
            break;
        }
        case common::String_Type:
            if (append) json_value[s].append(value.mStr);
            else        json_value[s] = value.mStr;
            break;
        case common::Array_Type: {
            Json::Value array_value;
            if (value.mArrayLen == 0)
                array_value["EMPTY_ARRAY"] = Json::arrayValue;     // This is an empty Json array
            else
            {
                if (call->Name() == "glShaderSource" && value.mName == "string")
                {
                    for (unsigned int i = 0; i < value.mArrayLen; ++i) {
                        string temp = "/shader/call" + intToString(call->mCallNo, gCallNo_width) + "_shader" + to_string(current_resource_id) + "." + to_string(i) + ".txt";
                        string shader_file = target_name + temp;
                        string relative_path = ".." + temp;
                        ofstream fout(shader_file);
                        if (!fout.is_open()) {
                            PAT_DEBUG_LOG("Cannot open file %s when extracting\n", shader_file.c_str());
                            break;
                        }
                        fout << value.mArray[i].mStr << endl;
                        fout.close();
                        array_value[value_type[value.mEleType]].append(relative_path);
                    }
                }
                else
                {
                    for (unsigned int i = 0; i < value.mArrayLen; ++i)
                        setJsonValue(array_value, value_type[value.mEleType], value.mArray[i], call, true);
                }
            }
            if (append) json_value[s].append(array_value);
            else        json_value[s] = array_value;
            break;
        }
        case common::Blob_Type: {
            if (value.mBlobLen == 0) {
                if (append) json_value[s].append(Json::nullValue);
                else        json_value[s] = Json::nullValue;
                break;
            }
            string blob_file, relative_path;
            if (call->Name().compare(0, 10, "glTexImage") == 0 ||
                call->Name().compare(0, 13, "glTexSubImage") == 0 ||
                call->Name().compare(0, 15, "glCompressedTex") == 0) {
                string temp = "/texture/call" + intToString(call->mCallNo, gCallNo_width) + "_tex" + to_string(current_resource_id) + ".bin";
                blob_file = target_name + temp;
                relative_path = ".." + temp;
            }
            else {
                string temp = "/blob/call" + intToString(call->mCallNo, gCallNo_width) + "_blob" + to_string(current_resource_id) + ".bin";
                blob_file = target_name + temp;
                relative_path = ".." + temp;
            }
            ofstream fout(blob_file);
            if (!fout.is_open()) {
                PAT_DEBUG_LOG("Cannot open file %s when extracting\n", blob_file.c_str());
            }
            fout.write(value.mBlob, value.mBlobLen);
            fout.close();
            if (append) json_value[s].append(relative_path);
            else        json_value[s] = relative_path;
            break;
        }
        case common::Opaque_Type: {
            Json::Value opaque_value;
            if (value.mOpaqueIns == 0) {
                opaque_value[opaque_value_type[value.mOpaqueType]] = Json::nullValue;
            }
            else {
                if (value.mOpaqueType == common::BufferObjectReferenceType) {
                    opaque_value[opaque_value_type[value.mOpaqueType]] = value.mOpaqueIns->GetAsUInt();
                }
                else if (value.mOpaqueType == common::BlobType) {
                    setJsonValue(opaque_value, opaque_value_type[value.mOpaqueType], *value.mOpaqueIns, call, false);
                }
                else {      // ClientSideBufferObjectReferenceType
                    opaque_value[opaque_value_type[value.mOpaqueType]].append(value.mOpaqueIns->mClientSideBufferName);
                    opaque_value[opaque_value_type[value.mOpaqueType]].append(value.mOpaqueIns->mClientSideBufferOffset);
                }
            }
            if (append) json_value[s].append(opaque_value);
            else        json_value[s] = opaque_value;
            break;
        }
        case common::Pointer_Type: {
            Json::Value pointer_value = Json::nullValue;
            if (value.mPointer) {
                setJsonValue(pointer_value, value_type[value.mPointer->mType], *value.mPointer, call, false);
            }
            if (append) json_value[s].append(pointer_value);
            else        json_value[s] = pointer_value;
            break;
        }
        case common::MemRef_Type: {
            Json::Value opaque_value;
            opaque_value.append(value.mOpaqueIns->mClientSideBufferName);
            opaque_value.append(value.mOpaqueIns->mClientSideBufferOffset);
            if (append) json_value[s].append(opaque_value);
            else        json_value[s] = opaque_value;
            break;
        }
        case common::Unused_Pointer_Type:
            if (append) json_value[s].append(value.mUnusedPointer);
            else        json_value[s] = value.mUnusedPointer;
            break;
    }
}

int get_counts(const string& source_name, bool &multithread, int& callNo, int& frameNo_width, int& callNo_width)
{
    // Load input trace
    common::TraceFileTM source_file(CALL_BATCH_SIZE);
    if (!source_file.Open(source_name.c_str()))
    {
        PAT_DEBUG_LOG("Failed to open pat file %s for extracting.\n", source_name.c_str());
        return 1;
    }

    Json::Value header = source_file.mpInFileRA->getJSONHeader();
    unsigned defaultTid = header["defaultTid"].asInt();
    bool hMultiThread = header["multiThread"].asBool();

    if(multithread){
        cout << "Multithread: enabled" << endl;
    }
    else if(hMultiThread){
        cout<<"Multithread: Auto enabled as found in the pat file"<<endl;
        multithread = true;
    }

    callNo = 0;
    int frameNo = 0;
    common::CallTM *call = NULL;
    for (callNo = 0; (call = source_file.NextCall()); ++callNo) {
        if ((multithread || call->mTid == defaultTid) && call->mCallName.substr(0, 14) == "eglSwapBuffers") {
            ++frameNo;
        }
    }

    cout << "callNo = " << callNo << ", frameNo = " << frameNo << endl;
    int temp_callNo = callNo;
    callNo_width = 0;
    do {
        temp_callNo /= 10;
        callNo_width++;
    } while (temp_callNo != 0);

    int temp_frameNo = frameNo;
    frameNo_width = 0;
    do {
        temp_frameNo /= 10;
        frameNo_width++;
    } while (temp_frameNo != 0);

    return 0;
}

int pat_extract(const string &source_name, const string &target_name_, bool multithread, int begin_call, int end_call)
{
    target_name = target_name_;
    cout << "Extract: " << source_name << " -> " << target_name << "\n" << endl;

    int callNo = 0;
    int frameNo_width = 0;
    int callNo_width = 0;
    get_counts(source_name, multithread, callNo, frameNo_width, callNo_width);
    gCallNo_width = callNo_width;

    if (makeExtractDir(target_name) != 0) {
        PAT_DEBUG_LOG("Cannot create folder %s\n", target_name.c_str());
        return 1;
    }

    // Iterate over all calls
    cout << "Extracting..." << endl;
    ofstream fout(target_name + "/pat_header.json");
    if (!fout.is_open()) {
        PAT_DEBUG_LOG("Failed to open file %s when extracting\n", string(target_name + "/pat_header.json").c_str());
        return 1;
    }

    common::TraceFileTM source_file(CALL_BATCH_SIZE);
    if (!source_file.Open(source_name.c_str()))
    {
        PAT_DEBUG_LOG("Failed to open pat file %s for extracting.\n", source_name.c_str());
        return 1;
    }
    Json::Value header = source_file.mpInFileRA->getJSONHeader();

    Json::StyledWriter writer;
    string strWrite = writer.write(header);
    if (strWrite[strWrite.length() - 1] == '\n')
        strWrite.pop_back();
    fout << strWrite;
    fout.close();
    fout.clear();

    Json::FastWriter fastwriter;
    string strFastWrite = fastwriter.write(header);
    if (strFastWrite[strFastWrite.length() - 1] == '\n')
            strFastWrite.pop_back();

    makeProgress(0, callNo, true);
    bool finished = false, file_beginning = true, no_frame_file_opened = true;
    common::OutFile outputFileBefore, outputFileAfter;
    if (!outputFileBefore.Open((target_name + "/not_interested_in/before.pat").c_str()))
    {
        PAT_DEBUG_LOG("Failed to open file %s for writing extracting result!\n", (target_name + "/not_interested_in/before.pat").c_str());
        return 1;
    }
    if (!outputFileAfter.Open((target_name + "/not_interested_in/after.pat").c_str()))
    {
        PAT_DEBUG_LOG("Failed to open file %s for writing extracting result!\n", (target_name + "/not_interested_in/after.pat").c_str());
        return 1;
    }
    outputFileBefore.mHeader.jsonLength = strFastWrite.size();
    outputFileBefore.WriteHeader(strFastWrite.c_str(), strFastWrite.size());
    outputFileAfter.mHeader.jsonLength = strFastWrite.size();
    outputFileAfter.WriteHeader(strFastWrite.c_str(), strFastWrite.size());
    file_counter = 0;
    unsigned defaultTid = header["defaultTid"].asInt();
    common::CallTM *call = NULL;
    for (callId = 0; (call = source_file.NextCall()); ++callId)
    {
        if (callId < begin_call)           // the call before user interested in
        {
            writeout(outputFileBefore, call);
            if ((multithread || call->mTid == defaultTid) && call->mCallName.substr(0, 14) == "eglSwapBuffers") {
                file_counter++;
            }
        }
        else if (callId > end_call)        // the call after user interested in
        {
            writeout(outputFileAfter, call);
            if ((multithread || call->mTid == defaultTid) && call->mCallName.substr(0, 14) == "eglSwapBuffers") {
                file_counter++;
            }
        }
        else
        {
            if (open_frame_file(fout, no_frame_file_opened, frameNo_width))
                no_frame_file_opened = false;
            else
                return 1;
            Json::Value function_value;
            int index = 0;
            function_value[genIdName(index++, "call_no")] = call->mCallNo;
            function_value[genIdName(index++, "tid")] = call->mTid;
            function_value[genIdName(index++, "func_name")] = call->mCallName;
            function_value[genIdName(index++, "return_type")] = value_type[call->mRet.mType];
            setJsonValue(function_value, genIdName(index++, "return_value"), call->mRet, call, false);
            Json::Value arg_value;
            unsigned int arg_id_width = 1;
            if (call->mArgs.size() >= 10)   // It's impossible that the number of arguments of a GLES call exceeds 99
                arg_id_width = 2;

            if (call->mCallName.substr(0, 12) == "glBindBuffer" ||
                call->mCallName.substr(0, 13) == "glBindTexture") {
                unsigned int target = call->mArgs[0]->GetAsUInt();
                unsigned int id = call->mArgs[1]->GetAsUInt();
                target_id_map[target] = id;
            }
            else if (call->mCallName.substr(0, 12) == "glBufferData" ||
                     call->mCallName.substr(0, 15) == "glBufferSubData" ||
                     call->mCallName.substr(0, 12) == "glTexImage1D" ||
                     call->mCallName.substr(0, 12) == "glTexImage2D" ||
                     call->mCallName.substr(0, 12) == "glTexImage3D" ||
                     call->mCallName.substr(0, 15) == "glTexSubImage1D" ||
                     call->mCallName.substr(0, 15) == "glTexSubImage2D" ||
                     call->mCallName.substr(0, 15) == "glTexSubImage3D" ||
                     call->mCallName.substr(0, 22) == "glCompressedTexImage1D" ||
                     call->mCallName.substr(0, 22) == "glCompressedTexImage2D" ||
                     call->mCallName.substr(0, 22) == "glCompressedTexImage3D" ||
                     call->mCallName.substr(0, 25) == "glCompressedTexSubImage1D" ||
                     call->mCallName.substr(0, 25) == "glCompressedTexSubImage2D" ||
                     call->mCallName.substr(0, 25) == "glCompressedTexSubImage3D" ||
                     call->mCallName.substr(0, 29) == "glCompressedTextureSubImage1D" ||
                     call->mCallName.substr(0, 29) == "glCompressedTextureSubImage2D" ||
                     call->mCallName.substr(0, 29) == "glCompressedTextureSubImage3D" ||
                     call->mCallName.substr(0, 23) == "glPatchClientSideBuffer")
            {
                unsigned int target = call->mArgs[0]->GetAsUInt();
                if (target == GL_TEXTURE_CUBE_MAP_POSITIVE_X ||
                    target == GL_TEXTURE_CUBE_MAP_POSITIVE_Y ||
                    target == GL_TEXTURE_CUBE_MAP_POSITIVE_Z ||
                    target == GL_TEXTURE_CUBE_MAP_NEGATIVE_X ||
                    target == GL_TEXTURE_CUBE_MAP_NEGATIVE_Y ||
                    target == GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
                    )
                {
                    target = GL_TEXTURE_CUBE_MAP;
                }
                auto it = target_id_map.find(target);
                current_resource_id = (it != target_id_map.end() ? it->second : 0);
            }
            else if (call->mCallName.substr(0, 14) == "glShaderSource" ||
                     call->mCallName.substr(0, 17) == "glNamedBufferData" ||
                     call->mCallName.substr(0, 20) == "glNamedBufferSubData")
            {
                current_resource_id = call->mArgs[0]->GetAsUInt();
            }
            else if (call->mCallName == "glClientSideBufferData" ||
                     call->mCallName == "glClientSideBufferSubData")
            {
                current_resource_id = call->mArgs[0]->GetAsUInt();
            }

            for (unsigned int i = 0; i < call->mArgs.size(); ++i)
            {
                int index1 = index;
                function_value[genIdName(index1++, "arg_type")].append(value_type[call->mArgs[i]->mType]);
                Json::Value arg_value;
                setJsonValue(arg_value, genIdName(i, call->mArgs[i]->mName, arg_id_width), *call->mArgs[i], call, false);
                Json::ValueIterator vi = arg_value.begin();
                function_value[genIdName(index1++, "arg_value")][vi.key().asString()] = *vi;
            }
            if (call->mArgs.size() == 0)
            {
                int index1 = index;
                function_value[genIdName(index1++, "arg_type")] = Json::arrayValue;   // This is an empty Json array
                function_value[genIdName(index1++, "arg_value")] = Json::nullValue;   // This is an null Json value
            }
            Json::StyledWriter writer;
            string strWrite = writer.write(function_value);
            if (strWrite[strWrite.length() - 1] == '\n')
                strWrite.pop_back();
            if (!file_beginning) {
                fout << ",\n";
            }
            else {
                fout << "[\n";
                file_beginning = false;
            }
            fout << strWrite;
            if ((multithread || call->mTid == defaultTid) && call->mCallName.substr(0, 14) == "eglSwapBuffers") {
                file_counter++;
                fout << "\n]";
                fout.close();
                fout.clear();
                no_frame_file_opened = true;
                if (callId != callNo - 1) {
                    file_beginning = true;
                }
                else {
                    finished = true;
                }
            }
            function_value.clear();
        }
        makeProgress(callId + 1, callNo);
    }
    if (!finished) {
        fout << "\n]";
        fout.close();
        fout.clear();
    }

    // save extract info
    cout << "Saving extract info..." << endl;
    fout.open(target_name + "/extract_info.json");
    if (!fout.is_open()) {
        PAT_DEBUG_LOG("Failed to open file %s when saving\n", string(target_name + "/extract_info.json").c_str());
        return 1;
    }
    Json::Value info_extract;
    info_extract["multithread"] = Json::Value(multithread);
    string strWriteInfo = writer.write(info_extract);
    if (strWriteInfo[strWriteInfo.length() - 1] == '\n')
        strWriteInfo.pop_back();
    fout<<strWriteInfo;
    fout.close();
    fout.clear();

    cout << "\n\n";
    source_file.Close();
    outputFileBefore.Close();
    outputFileAfter.Close();
    cout << "Pat extraction finished." << endl;
    return 0;
}
