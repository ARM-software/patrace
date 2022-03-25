#include <iostream>
#include <fstream>
#include <dirent.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include "eglstate/common.hpp"
#include "base/base.hpp"
#include "commonData.hpp"

using namespace std;

unordered_map<string, unsigned int> egl_enum_map;
unordered_map<string, unsigned int> egl_gles_enum_map;
unordered_map<string, int> type_to_enum_map;

void createStringToEglEnum()
{
    for (unsigned int i = 0; i < 0xFFFF; ++i)
    {
        if (const char *s = EnumString(i, "egl"))
        {
            if (string(s).compare(0, 3, "EGL") == 0)
                egl_enum_map[s] = i;
        }
    }
}

void createStringToEglGlesEnum()
{
    for (unsigned int i = 0; i < 0xFFFF; ++i)
    {
        const char *s = EnumString(i, "glDraw");
        if (s)
        {
            egl_gles_enum_map[s] = i;
        }
    }
    egl_gles_enum_map["GL_NONE"] = 0;
    egl_gles_enum_map["GL_ZERO"] = 0;
    egl_gles_enum_map["GL_ONE"] = 1;
    egl_gles_enum_map["GL_NO_ERROR"] = 0;
    for (auto it = egl_enum_map.begin(); it != egl_enum_map.end(); ++it)
        egl_gles_enum_map.insert(*it);
}

void findAllGlesFiles(const string &path, vector<GlesFilePath> &result)
{
    string s = path + "/GLES_calls/";
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(s.c_str())) != NULL)
    {
        while ((ent = readdir(dir)) != NULL)
        {
            if (filenameExtension(ent->d_name) == "json")
            {
                string fullname = s + ent->d_name;
                result.push_back(fullname);
            }
        }
        sort(result.begin(), result.begin() + result.size());
    }
}

string source_name;

void setValueTM(common::ValueTM *&pValue, const string &type_string, const Json::Value &json_value, const string & func_name)
{
    int type = type_to_enum_map[type_string];
    switch (type) {
        case common::Void_Type:
            pValue = new common::ValueTM();
            break;
        case common::Int8_Type:
            pValue = new common::ValueTM();
            pValue->mType = common::Int8_Type;
            pValue->mInt8 = json_value.asInt();
            break;
        case common::Uint8_Type:
            pValue = new common::ValueTM();
            pValue->mType = common::Uint8_Type;
            pValue->mUint8 = json_value.asUInt();
            break;
        case common::Int16_Type:
            pValue = new common::ValueTM();
            pValue->mType = common::Int16_Type;
            pValue->mInt16 = json_value.asInt();
            break;
        case common::Uint16_Type:
            pValue = new common::ValueTM();
            pValue->mType = common::Uint16_Type;
            pValue->mUint16 = json_value.asUInt();
            break;
        case common::Int_Type:
            if (json_value.isString()) {
                auto it = egl_gles_enum_map.find(json_value.asString());
                if (it != egl_gles_enum_map.end()) {
                    pValue = new common::ValueTM(it->second);
                }
                else {
                    PAT_DEBUG_LOG("Cannot find enum %s\n", json_value.asString().c_str());
                }
            }
            else {
                pValue = new common::ValueTM(json_value.asInt());
            }
            break;
        case common::Enum_Type: {
            pValue = new common::ValueTM();
            pValue->mType = common::Enum_Type;
            if (json_value.isString()) {
                auto it = egl_gles_enum_map.find(json_value.asString());
                if (it != egl_gles_enum_map.end()) {
                    pValue->mEnum = it->second;
                }
                else {
                    PAT_DEBUG_LOG("Cannot find enum %s\n", json_value.asString().c_str());
                }
            }
            else {       // an unsigned int value, needn't be converted
                pValue->mEnum = json_value.asUInt();
            }
            break;
        }
        case common::Uint_Type:
            pValue = new common::ValueTM(json_value.asUInt());
            break;
        case common::Int64_Type:
            pValue = new common::ValueTM((long long)json_value.asInt64());
            break;
        case common::Uint64_Type:
            pValue = new common::ValueTM();
            pValue->mType = common::Uint64_Type;
            pValue->mUint64 = json_value.asUInt64();
            break;
        case common::Float_Type:
            pValue = new common::ValueTM();
            pValue->mType = common::Float_Type;
            try {
                pValue->mFloat = json_value.asFloat();
            }
            catch (const Json::LogicError &err) {   // exception when json_value is string "nan", "-nan", "inf" or "-inf" instead of a floating number
                string s = json_value.asString();
                if (s == "nan") pValue->mFloat = NAN;
                else if (s == "-nan") pValue->mFloat = -NAN;
                else if (s == "inf")    pValue->mFloat = INFINITY;
                else if (s == "-inf")   pValue->mFloat = -INFINITY;
            }
            break;
        case common::String_Type:
            pValue = new common::ValueTM(json_value.asString());
            break;
        case common::Array_Type: {
            auto only_value = json_value.begin();
            string array_element_type = only_value.key().asString();
            pValue = new common::ValueTM();
            pValue->mType = common::Array_Type;
            pValue->mEleType = static_cast<common::Value_Type_TM>(type_to_enum_map[array_element_type]);
            pValue->mArrayLen = (*only_value).size();
            pValue->mArray = new common::ValueTM [(*only_value).size()];
            if (array_element_type == "String" && func_name == "glShaderSource") {  // This is a shader
                for (unsigned int i = 0; i < (*only_value).size(); ++i) {
                    pValue->mArray[i].mType = common::String_Type;
                    ifstream fin(source_name + "/GLES_calls/" + (*only_value)[i].asString());
                    if (!fin.is_open()) {
                        PAT_DEBUG_LOG("Cannot open shader file %s when merging\n", (*only_value).asString().c_str());
                        break;
                    }
                    string line;
                    for (int counter = 0; getline(fin, line); ++counter) {
                        if (counter != 0)
                            pValue->mArray[i].mStr += "\n";
                        pValue->mArray[i].mStr += line;
                    }
                    fin.close();
                }
            }
            else {
                for (unsigned int i = 0; i < (*only_value).size(); ++i) {
                    common::ValueTM *pTemp;
                    setValueTM(pTemp, array_element_type, (*only_value)[i], func_name);
                    pValue->mArray[i] = *pTemp;
                    delete pTemp;
                }
            }
            break;
        }
        case common::Blob_Type: {
            char *blob_value = 0;
            streamsize size = 0;
            if (json_value != Json::nullValue) {
                ifstream fin(source_name + "/GLES_calls/" + (json_value).asString());
                if (!fin.is_open()) {
                    PAT_DEBUG_LOG("Cannot open file %s when merging\n", json_value.asString().c_str());
                    break;
                }
                fin.ignore(numeric_limits<streamsize>::max());
                size = fin.gcount();
                fin.clear();
                fin.seekg(0, ios_base::beg);
                blob_value = new char[size];
                fin.read(blob_value, size);
                fin.close();
            }
            pValue = new common::ValueTM(blob_value, size);
            break;
        }
        case common::Opaque_Type: {
            pValue = new common::ValueTM();
            pValue->mType = common::Opaque_Type;
            auto only_value = json_value.begin();
            if (only_value.key().asString() == opaque_value_type[0]) {      // BufferObjectReferenceType
                pValue->mOpaqueType = common::BufferObjectReferenceType;
                setValueTM(pValue->mOpaqueIns, "Uint", *only_value, func_name);
            }
            else if (only_value.key().asString() == opaque_value_type[1]) { // BlobType
                pValue->mOpaqueType = common::BlobType;
                setValueTM(pValue->mOpaqueIns, "Blob", *only_value, func_name);
            }
            else {             // ClientSideBufferObjectReferenceType
                pValue->mOpaqueType = common::ClientSideBufferObjectReferenceType;
                setValueTM(pValue->mOpaqueIns, "MemRef", *only_value, func_name);
            }
            break;
        }
        case common::Pointer_Type: {
            pValue = new common::ValueTM();
            pValue->mType = common::Pointer_Type;
            if (json_value == Json::nullValue) {
                pValue->mPointer = 0;
            }
            else {
                auto only_value = json_value.begin();
                setValueTM(pValue->mPointer, only_value.key().asString(), *only_value, func_name);
            }
            break;
        }
        case common::MemRef_Type:
            pValue = new common::ValueTM();
            pValue->mType = common::MemRef_Type;
            pValue->mClientSideBufferName = json_value[0].asInt();
            pValue->mClientSideBufferOffset = json_value[1].asInt();
            break;
        case common::Unused_Pointer_Type:
            pValue = new common::ValueTM();
            pValue->mType = common::Unused_Pointer_Type;
            if (json_value.isUInt())
                pValue->mUnusedPointer = reinterpret_cast<void *>(json_value.asUInt());
            else if (json_value.isUInt64())
                pValue->mUnusedPointer = reinterpret_cast<void *>(json_value.asUInt64());
            else
                PAT_DEBUG_LOG("Meet an abnormal Unused_Pointer_Type!\n");
            break;
    }
}

int merge_to_pat(const string &source_name_, const string &target_name, bool multithread)
{
    source_name = source_name_;
    for (unsigned int i = 0; i < sizeof(value_type) / sizeof(string); ++i)
        type_to_enum_map[value_type[i]] = i;

    createStringToEglEnum();
    createStringToEglGlesEnum();

    vector<GlesFilePath> gles_files;
    findAllGlesFiles(source_name, gles_files);
    cout << "extracted frame = " << gles_files.size() << endl;

    common::OutFile target_file;
    if (!target_file.Open(target_name.c_str()))
    {
        PAT_DEBUG_LOG("Failed to open pat file %s merging to.\n", target_name.c_str());
        return 1;
    }

    cout << "Merge: " << source_name << " -> " << target_name << "\n" << endl;
    Json::Reader reader;

    ifstream fin(source_name + "/pat_header.json");
    if (!fin.is_open()) {
        PAT_DEBUG_LOG("Cannot open file %s when mergin\n", (source_name + "/pat_header.json").c_str());
        return 1;
    }
    Json::Value json_value;
    reader.parse(fin, json_value, false);
    unsigned defaultTid = json_value["defaultTid"].asInt();
    bool hMultiThread = json_value["multiThread"].asBool();
    Json::FastWriter header_writer;
    const std::string json_header = header_writer.write(json_value);

    target_file.mHeader.jsonLength = json_header.size();
    target_file.WriteHeader(json_header.c_str(), json_header.size());
    fin.close();
    fin.clear();

    // get multithread info from extract_info
    fin.open(source_name + "/extract_info.json");
    if (!fin.is_open()) {
        PAT_DEBUG_LOG("Cannot open file extract_info.json when merging\n");
        return 1;
    }
    Json::Value json_value_info;
    reader.parse(fin, json_value_info, false);
    bool jMultiThread = json_value_info["multithread"].asBool();
    if(multithread){
        cout << "Multithread: enabled" << endl;
    }
    else if(hMultiThread){
        cout<<"Multithread: Auto enabled as found in the pat file"<<endl;
        multithread = true;
    }
    else if(jMultiThread){
        cout<<"Multithread: Auto enabled as found in the extract info"<<endl;
        multithread = true;
    }
    fin.close();
    fin.clear();

    // calculate the frame number of before.pat
    common::TraceFileTM file_before, file_after;
    if (!file_before.Open((source_name + "/not_interested_in/before.pat").c_str()))
    {
        PAT_DEBUG_LOG("Failed to open pat file %s for extracting.\n", (source_name + "/not_interested_in/before.pat").c_str());
        return 1;
    }
    common::CallTM *call = NULL;
    int frame_num = 0;
    while ((call = file_before.NextCall()))
    {
        if ((multithread || call->mTid == defaultTid) && call->mCallName.substr(0, 14) == "eglSwapBuffers")
        {
            frame_num++;
        }
    }

    // calculate the frame number of after.pat
    if (!file_after.Open((source_name + "/not_interested_in/after.pat").c_str()))
    {
        PAT_DEBUG_LOG("Failed to open pat file %s for extracting.\n", (source_name + "/not_interested_in/after.pat").c_str());
        return 1;
    }
    while ((call = file_after.NextCall()))
    {
        if ((multithread || call->mTid == defaultTid) && call->mCallName.substr(0, 14) == "eglSwapBuffers")
        {
            frame_num++;
        }
    }
    frame_num += gles_files.size();

    makeProgress(0, frame_num, true);

    // write all calls in before.pat to the target
    int counter = 0;
    while ((call = file_before.NextCall()))
    {
        writeout(target_file, call);
        if ((multithread || call->mTid == defaultTid) && call->mCallName.substr(0, 14) == "eglSwapBuffers")
        {
            counter++;
            makeProgress(counter, frame_num);
        }
    }
    file_before.Close();

    // write all calls in GLES_calls/frame_xxxx.pat to the target
    for (vector<string>::size_type file_counter = 0; file_counter < gles_files.size(); ++file_counter)
    {
        if (gles_files[file_counter] == "")
            continue;
        fin.open(gles_files[file_counter]);
        if (!fin.is_open()) {
            PAT_DEBUG_LOG("Cannot open file %s when merging\n", gles_files[file_counter].c_str());
            break;
        }
        while ((fin.rdstate() & std::ifstream::failbit ) == 0) {
            string s;
            char c;
            bool started = false;
            int curved_bracket_stack = 0;
            // Calculate the start and end points of the next Json value.
            while (fin.get(c)) {
                if (c == '{') {
                    started = true;
                    ++curved_bracket_stack;
                }
                else if (c == '}') {
                    --curved_bracket_stack;
                    if (curved_bracket_stack == 0) {
                        s.push_back(c);
                        break;
                    }
                }
                if (started == true)
                    s.push_back(c);
            }
            if ((fin.rdstate() & std::ifstream::failbit ) != 0)
                break;
            // Parse the next Json value
            if (reader.parse(&s[0], &s[s.length()] , json_value, false)) {
                int tid = json_value["1 tid"].asInt();
                string func_name = json_value["2 func_name"].asString();
                string return_type = json_value["3 return_type"].asString();

                common::CallTM func(func_name.c_str());
                func.mTid = tid;

                common::ValueTM *ret_value;
                setValueTM(ret_value, return_type, json_value["4 return_value"], func_name);
                func.mRet = *ret_value;

                auto vi = json_value["6 arg_value"].begin();
                for (unsigned int j = 0; j < json_value["5 arg_type"].size(); ++j)
                {
                    common::ValueTM *pValueTM;
                    setValueTM(pValueTM, json_value["5 arg_type"][j].asString(), *vi, func_name);
                    func.mArgs.push_back(pValueTM);
                    ++vi;
                }

                writeout(target_file, &func);
                delete ret_value;
            }
            else {
                PAT_DEBUG_LOG("The json file %s cannot be parsed for an unknown reason.\n", gles_files[file_counter].c_str());
            }
        }
        fin.close();
        fin.clear();
        makeProgress(counter + file_counter + 1, frame_num);
    }
    // write all calls in after.pat to the target
    int counter2 = 0;
    while ((call = file_after.NextCall()))
    {
        writeout(target_file, call);
        if ((multithread || call->mTid == defaultTid) && call->mCallName.substr(0, 14) == "eglSwapBuffers")
        {
            counter2++;
            makeProgress(counter + gles_files.size() + counter2, frame_num);
        }
    }
    cout << endl;
    file_after.Close();
    target_file.Close();
    return 0;
}
