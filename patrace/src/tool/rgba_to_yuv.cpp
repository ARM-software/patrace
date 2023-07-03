// This tool converts the following code segment:
//
// 4718 : ret=void glGenTextures(n=1, textures={9})
// 4719 : ret=void glBindTexture(target=GL_TEXTURE_2D, texture=9)
// 4720 : ret=void glTexImage2D(target=GL_TEXTURE_2D, level=0, internalformat=GL_RGBA, width=1280, height=720, border=0, format=GL_RGBA, type=GL_UNSIGNED_BYTE, pixels=common::BlobType/*BlobSize:3686400*/)
// 4721 : ret=void glBindTexture(target=GL_TEXTURE_2D, texture=0)
// 4722 : ret=-952401992 eglCreateImageKHR(dpy=-217044640, ctx=-647737464, target=0x30B1, buffer=9, attrib_list={12498, 1, 12344}) // 0x30B1=EGL_GL_TEXTURE_2D_KHR
//
// to this:
//
// 4718 : ret=1 glGenGraphicBuffer_ARM(width=1280, height=720, format=842094169, usage=403763)                                     // 842094169=HAL_PIXEL_FORMAT_YV12
// 4719 : ret=void glGraphicBufferData_ARM(name=1, size=1382400, data=_binary_blob_0_bin_start/*BlobSize1382400*/)
// 4720 : ret=-952401992 eglCreateImageKHR(dpy=-217044640, ctx=0, target=0x3140, buffer=1, attrib_list={12498, 1, 12344})          // 0x3140=EGL_NATIVE_BUFFER_ANDROID
//
//
// and converts the following code segment:
//
// 4726 : ret=void glGenTextures(n=1, textures={10})
// 4727 : ret=void glBindTexture(target=GL_TEXTURE_2D, texture=10)
// 4728 : ret=void glTexImage2D(target=GL_TEXTURE_2D, level=0, internalformat=GL_RGBA, width=1280, height=720, border=0, format=GL_RGBA, type=GL_UNSIGNED_BYTE, pixels=common::BlobType/*BlobSize:3686400*/)
// 4729 : ret=void glBindTexture(target=GL_TEXTURE_2D, texture=0)
// 4730 : ret=0x1 eglDestroyImageKHR(dpy=-217044640, image=-952401992)
// 4731 : ret=-952401992 eglCreateImageKHR(dpy=-217044640, ctx=-647737464, target=0x30B1, buffer=10, attrib_list={12498, 1, 12344})
// 4732 : ret=void glEGLImageTargetTexture2DOES(target=GL_TEXTURE_EXTERNAL_OES, image=-952401992)
//
// to this:
//
// 4724 : ret=void glGenTextures(n=1, textures={10})
// 4725 : ret=void glGraphicBufferData_ARM(name=1, size=1382400, data=_binary_blob_0_bin_start/*BlobSize1382400*/)
// 4726 : ret=void glEGLImageTargetTexture2DOES(target=GL_TEXTURE_EXTERNAL_OES, image=-952401992)
//
// In commit 36d5d9a3956b30bf4e59c7a08347f5d9ecda4700, we designed 2 pseudo OpenGL ES API calls "glGenGraphicBuffer_ARM" and "glGraphicBufferData_ARM" to utilise Android GraphicBuffer
// to store the data of external textures. And we decided to inject some statements before each glEGLImageTargetTextures2DOES in order to refresh the data of external texture.
// For example, call 4726-4730 are injected by our tracer when it meets a "glEGLImageTargetTextures2DOES" call.
// This commit is the successor of r2p9 commit. Any pat files traced before that are called "old version file" and after that are called "new version file" in the following code.
//
// So, the second conversion only applies to new version pat file

#include <iostream>
#include <map>
#include <unordered_map>
#include <utility>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include "common/trace_model.hpp"
#include "common/out_file.hpp"
#include "graphic_buffer/GraphicBuffer.hpp"
#include "retracer/dma_buffer/dma_buffer.hpp"
#include "eglstate/common.hpp"
#include "common/image.hpp"
#include "tool/config.hpp"

using namespace std;

static void printHelp()
{
    cout <<
        "Usage : rgba_to_yuv [OPTIONS] <source trace> <target trace>\n"
        "\n"
        "Options:\n"
        "  -f PIXEL_FORMAT      Specify the pixelFormat of the target. The following values can be used:\n"
        "     REMAIN            The pixelFormat of the target won't be converted.\n"
        "     YV12              YV12 format\n"
        "     NV12              NV12 format\n"
        "  -u USAGE             Specify the usage of the target. USAGE must be a decimal integer.\n"
        "  -no_crop             Abandon all the attribs of eglCreateImageKHR related to EGL_ANDROID_image_crop extension"
        "  -h                   Print help.\n"
        "  -v                   Print version.\n"
        ;
}

static void printVersion()
{
    cout << PATRACE_VERSION << endl;
}

static void writeout(common::OutFile &outputFile, common::CallTM *call)
{
    const unsigned int WRITE_BUF_LEN = 150*1024*1024;
    static char buffer[WRITE_BUF_LEN];

    char *dest = call->Serialize(buffer);
    outputFile.Write(buffer, dest-buffer);
}

int truncate(int num, int min = 0, int max = 255)
{
    if (num < min)
        return min;
    else if (num > max)
        return max;
    return num;
}

enum Format
{
    REMAIN = 0,
    YV12 = 1,
    NV12 = 2,
};

int NoConvert(unsigned char *rgba, unsigned char *yv12, int width, int height)
{
    return 0;
}

int RGBAtoYV12(unsigned char *rgba, unsigned char *yv12, int width, int height)
{
    int y_stride = (width + 15) / 16 * 16;
    int y_height= height;
    int y_size = y_stride * y_height;
    int c_stride = (y_stride / 2 + 15) / 16 * 16;
    int c_size = c_stride * y_height / 2;
    int yuv_size = y_size + c_size * 2;

    // y =  0.299r + 0.587g + 0.114b;
    // u = -0.169r - 0.331g + 0.500b + 128;
    // v =  0.500r - 0.419g - 0.081b + 128;
    for (int i = 0; i < y_stride * y_height; ++i) {
        int y_x = i % y_stride;
        if (y_x >= width)
            continue;
        int y_y = i / y_stride;

        int r = rgba[(y_y * width + y_x) * 4];
        int g = rgba[(y_y * width + y_x) * 4 + 1];
        int b = rgba[(y_y * width + y_x) * 4 + 2];
        int y = 0.299 * r + 0.587 * g + 0.114 * b;
        yv12[y_y * y_stride + y_x] = truncate(y);
        if (((y_x & 0x1) == 0) && ((y_y & 0x1) == 0)) {
            int u = -0.169 * r - 0.331 * g + 0.500 * b + 128;
            int v =  0.500 * r - 0.419 * g - 0.081 * b + 128;
            int u_x = y_x / 2;
            int u_y = y_y / 2;
            yv12[y_size + u_y * c_stride + u_x] = truncate(v);
            yv12[y_size + c_size + u_y * c_stride + u_x] = truncate(u);
        }
    }

    return yuv_size;
}

int RGBAtoNV12(unsigned char *rgba, unsigned char *nv12, int width, int height)
{
    //int y_stride = width;
    int y_stride = (width + 15) / 16 * 16;
    int y_height= height;
    int y_size = y_stride * y_height;
    int c_stride = y_stride;
    int c_size = c_stride * y_height / 2;
    int yuv_size = y_size + c_size;

    // y =  0.299r + 0.587g + 0.114b;
    // u = -0.169r - 0.331g + 0.500b + 128;
    // v =  0.500r - 0.419g - 0.081b + 128;
    for (int i = 0; i < y_stride * y_height; ++i) {
        int y_x = i % y_stride;
        if (y_x >= width)
            continue;
        int y_y = i / y_stride;

        int r = rgba[(y_y * width + y_x) * 4];
        int g = rgba[(y_y * width + y_x) * 4 + 1];
        int b = rgba[(y_y * width + y_x) * 4 + 2];
        int y = 0.299 * r + 0.587 * g + 0.114 * b;
        nv12[y_y * y_stride + y_x] = truncate(y);
        if (((y_x & 0x1) == 0) && ((y_y & 0x1) == 0)) {
            int u = -0.169 * r - 0.331 * g + 0.500 * b + 128;
            int v =  0.500 * r - 0.419 * g - 0.081 * b + 128;
            int u_x = y_x / 2;
            int u_y = y_y / 2;
            nv12[y_size + u_y * c_stride + u_x * 2] = truncate(u);
            nv12[y_size + u_y * c_stride + u_x * 2 + 1] = truncate(v);
        }
    }

    return yuv_size;
}

typedef int (*CONVERT_FUNCTION)(unsigned char *rgba, unsigned char *yuv, int width, int height);

map<Format, PixelFormat> androidFormatMap;
map<PixelFormat, CONVERT_FUNCTION> functionMap;
map<int, string> AndroidImageCropAttribToNameMap;

void makeMap()
{
    androidFormatMap[REMAIN] = PIXEL_FORMAT_NONE;
    androidFormatMap[YV12] = HAL_PIXEL_FORMAT_YV12;
    androidFormatMap[NV12] = MALI_GRALLOC_FORMAT_INTERNAL_NV12;

    functionMap[PIXEL_FORMAT_NONE] = NoConvert;
    functionMap[HAL_PIXEL_FORMAT_YV12] = RGBAtoYV12;
    functionMap[MALI_GRALLOC_FORMAT_INTERNAL_NV12] = RGBAtoNV12;

    AndroidImageCropAttribToNameMap[0x3148] = "EGL_IMAGE_CROP_LEFT_ANDROID";
    AndroidImageCropAttribToNameMap[0x3149] = "EGL_IMAGE_CROP_TOP_ANDROID";
    AndroidImageCropAttribToNameMap[0x314A] = "EGL_IMAGE_CROP_RIGHT_ANDROID";
    AndroidImageCropAttribToNameMap[0x314B] = "EGL_IMAGE_CROP_BOTTOM_ANDROID";
}

unsigned char yuv_data[16384*16384];        // hope it's large enough for a texture

struct CallWithName
{
    int callNo;
    string name;
    CallWithName(int c, const string &s)
    {
        callNo = c;
        name = s;
    }
};

struct CallWithExclude
{
    int callNo;
    bool exclude;
    CallWithExclude(int c, bool e)
    {
        callNo = c;
        exclude = e;
    }
};

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

void process_eglCreateImageKHR(common::CallTM *call, bool no_crop)
{
    if (no_crop) {      // user wants to abandon EGL_ANDROID_image_crop related attribs
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
    else {
        bool foundCropAttrib = false;
        for (unsigned int i = 0; i < call->mArgs[4]->mArrayLen; i += 2) {
            int attrib = call->mArgs[4]->mArray[i].GetAsInt();
            if (isAndroidImageCropAttrib(attrib)) {
                foundCropAttrib = true;
                cout << "WARNING: eglCreateImageKHR attrib[" << i <<  ", " << i + 1 << "] = (" << AndroidImageCropAttribToNameMap[attrib]
                     << ", " << call->mArgs[4]->mArray[i + 1].GetAsInt() << ") "
                     << "in Call " << call->mCallNo << " needs EGL_ANDROID_image_crop extension which may not be supported by all devices. "
                     << "You can use \"-no_crop\" parameter to abandon it." << endl;
            }
        }
        if (foundCropAttrib)
            cout << endl;
    }
}

int convert(const string &source_name, const string &target_name, PixelFormat format, unsigned usage, bool no_crop, bool inject_delete)
{
    common::TraceFileTM source_file;
    CONVERT_FUNCTION convert_function;
    auto iter = functionMap.find(format);
    if (iter != functionMap.end())
    {
        convert_function = iter->second;
    }
    else
    {
        DBG_LOG("Cannot find the convert function for PixelFormat %x, "
                "Please insert it in function void makeMap()\n", format);
        exit(1);
    }
    if (!source_file.Open(source_name.c_str()))
    {
        PAT_DEBUG_LOG("Failed to open source pat file %s.\n", source_name.c_str());
        return 1;
    }
    cout << "convert: " << source_name << "(RGBA) -> " << target_name << "(YUV)\n" << endl;

    common::OutFile target_file;
    if (!target_file.Open(target_name.c_str()))
    {
        PAT_DEBUG_LOG("Failed to open the target pat file %s.\n", target_name.c_str());
        return 1;
    }
    cout << "Converting..." << endl;

    Json::Value header = source_file.mpInFileRA->getJSONHeader();
    Json::FastWriter writer;
    const string json_header = writer.write(header);
    target_file.mHeader.jsonLength = json_header.size();
    target_file.WriteHeader(json_header.c_str(), json_header.size());

    if (format != PIXEL_FORMAT_NONE) {
        // 1st step, check whether this file is a new version file or old version one
        map<int, string> last_callname;     // tid -> callName
        for (int i = 0; i < 100; ++i)       // 100 is large enough for the thread amount
            last_callname[i] = "";
        bool new_version_file = true;
        common::CallTM *call = NULL;
        while ((call = source_file.NextCall()))
        {
            int tid = call->mTid;
            if (call->mCallName == "glEGLImageTargetTexture2DOES") {
                // The previous call of "glEGLImageTargetTexture2DOES" must be "eglCreateImageKHR" in the new version pat file
                // Because our new tracer injected an "eglCreateImageKHR" just before "glEGLImageTargetTextures2DOES"
                // Otherwise, this must be an old version pat file
                if (last_callname[tid] != "eglCreateImageKHR") {
                    new_version_file = false;
                }
            }
            last_callname[tid] = call->mCallName;
        }
        if (new_version_file)
            DBG_LOG("This is a new version pat file traced by a tracer newer than r2p9, because the data of each glEGLImageTargetTexture2DOES are refreshed.\n");
        else
            DBG_LOG("This is an old version pat file traced by r2p9 or earlier tracer, because the data of some glEGLImageTargetTexture2DOES are not refreshed.\n");

        // 2nd step, pick up all the calls need to be excluded
        map<int, vector<CallWithExclude> > exclude_map;         // tid -> vector of excluded calls
        map<int, vector<CallWithName> >    call_no_map;         // tid -> vector of all calls
        source_file.ResetCurFrameIndex();
        while ((call = source_file.NextCall()))
        {
            int tid = call->mTid;
            int cur_call_no = call->mCallNo;
            call_no_map[tid].push_back(CallWithName(cur_call_no, call->mCallName));
            int end_id = call_no_map[tid].size() - 1;
            if (call->mCallName == "eglCreateImageKHR") {
                exclude_map[tid].push_back(CallWithExclude(call_no_map[tid][end_id - 4].callNo, true));
                exclude_map[tid].push_back(CallWithExclude(call_no_map[tid][end_id - 3].callNo, true));
                exclude_map[tid].push_back(CallWithExclude(call_no_map[tid][end_id - 2].callNo, true));
                exclude_map[tid].push_back(CallWithExclude(call_no_map[tid][end_id - 1].callNo, true));
            }
            else if (new_version_file && call->mCallName == "glEGLImageTargetTexture2DOES") {
                exclude_map[tid].push_back(CallWithExclude(call_no_map[tid][end_id - 1].callNo, true));
            }
        }

        // 3rd step, find the last call of glBindBuffer for each texture in the exclude_map
        map<int, int> idx_map;      // tid -> idx of the next excluded call
        for (auto it = exclude_map.begin(); it != exclude_map.end(); ++it) {
            idx_map.insert(make_pair(it->first, 0));
        }
        bool firstBindTexture = true;
        map<int, unordered_map<unsigned int, unsigned int> > tex_image_map; // tid -> tex_id -> image_id
        map<int, unsigned int> last_call_id_map; // tid -> last_call_id
        map<int, unsigned int> image_counter_map; // tid-> image_counter
        map<int, unsigned int> image_id_map; // tid -> image_id of the current thread
        source_file.ResetCurFrameIndex();
        while ((call = source_file.NextCall()))
        {
            int tid = call->mTid;
            int id = idx_map[tid];

            if (image_counter_map.find(tid) == image_counter_map.end()) {
                image_counter_map[tid] = 1;
            }

            if ((id < (long)exclude_map[tid].size()) && ((long)call->mCallNo == exclude_map[tid][id].callNo)) {
                ++idx_map[tid];
                if (call->mCallName == "glGenTextures") {
                    common::ValueTM *value = call->mArgs[1];
                    int tex_id = value->mArray[0].GetAsInt();
                    tex_image_map[tid][tex_id] = image_counter_map[tid]++;
                }
                else if (call->mCallName == "glBindTexture") {
                    if (firstBindTexture) {
                        int tex_id = call->mArgs[1]->GetAsInt();
                        auto it = tex_image_map[tid].find(tex_id);
                        if (it == tex_image_map[tid].end()) {
                            DBG_LOG("Cannot find tex %d in texToImage_map!\n", tex_id);
                            exit(EXIT_FAILURE);
                        }
                        else {
                            image_id_map[tid] = it->second;
                            firstBindTexture = false;
                            last_call_id_map[tid] = call->mCallNo + 1;
                            DBG_LOG("bind texture call = %d, last of %d\n", call->mCallNo + 1, image_id_map[tid]);
                        }
                    }
                    else {
                        firstBindTexture = true;
                    }
                }
            }
            else if (call->mCallName == "eglCreateImageKHR") {
                last_call_id_map[tid] = call->mCallNo;
            }
        }

        tex_image_map.clear();
        image_counter_map.clear();
        image_id_map.clear();
        bool needToGenGraphicBuffer = false;
        firstBindTexture = true;

        // 4th step, substiture all the excluded calls to the result
        idx_map.clear();
        for (auto it = exclude_map.begin(); it != exclude_map.end(); ++it) {
            idx_map.insert(make_pair(it->first, 0));
        }
        source_file.ResetCurFrameIndex();
        while ((call = source_file.NextCall())) {
            int tid = call->mTid;
            int id = idx_map[tid];

            if (image_counter_map.find(tid) == image_counter_map.end()) {
                image_counter_map[tid] = 1;
            }

            if ((id < (long)exclude_map[tid].size()) && ((long)call->mCallNo == exclude_map[tid][id].callNo)) {
                ++idx_map[tid];
                if (call->mCallName == "glGenTextures") {
                    common::ValueTM *value = call->mArgs[1];
                    int tex_id = value->mArray[0].GetAsInt();
                    tex_image_map[tid][tex_id] = image_counter_map[tid]++;
                    needToGenGraphicBuffer = true;
                    if (!exclude_map[tid][id].exclude) {
                        writeout(target_file, call);
                    }
                }
                else if (call->mCallName == "glBindTexture") {
                    if (firstBindTexture) {
                        int tex_id = call->mArgs[1]->GetAsInt();
                        auto it = tex_image_map[tid].find(tex_id);
                        if (it == tex_image_map[tid].end()) {
                            DBG_LOG("Cannot find tex %d in texToImage_map!\n", tex_id);
                            exit(EXIT_FAILURE);
                        }
                        else {
                            image_id_map[tid] = it->second;
                            firstBindTexture = false;
                        }
                    }
                    else {
                        firstBindTexture = true;
                    }
                }
                else if (call->mCallName == "glTexImage2D") {
                    int width = call->mArgs[3]->GetAsInt();
                    int height = call->mArgs[4]->GetAsInt();
                    unsigned char *blob = reinterpret_cast<unsigned char *>(call->mArgs[8]->mOpaqueIns->mBlob);
                    int yuv_size = convert_function(blob, yuv_data, width, height);

                    if (needToGenGraphicBuffer) {
                        common::CallTM genGraphicBuffer("glGenGraphicBuffer_ARM");
                        genGraphicBuffer.mTid = tid;
                        genGraphicBuffer.mArgs.push_back(new common::ValueTM(width));       // width
                        genGraphicBuffer.mArgs.push_back(new common::ValueTM(height));      // height
                        genGraphicBuffer.mArgs.push_back(new common::ValueTM(format));      // format
                        genGraphicBuffer.mArgs.push_back(new common::ValueTM(usage));       // usage
                        genGraphicBuffer.mRet = common::ValueTM(image_id_map[tid]);         // id
                        writeout(target_file, &genGraphicBuffer);
                        needToGenGraphicBuffer = false;
                    }

                    common::CallTM graphicBufferData("glGraphicBufferData_ARM");
                    graphicBufferData.mTid = tid;
                    graphicBufferData.mArgs.push_back(new common::ValueTM(image_id_map[tid]));   // name
                    graphicBufferData.mArgs.push_back(new common::ValueTM(yuv_size));   // size
                    graphicBufferData.mArgs.push_back(new common::ValueTM(reinterpret_cast<char *>(yuv_data), yuv_size));    // data
                    graphicBufferData.mRet = common::ValueTM();          // void
                    writeout(target_file, &graphicBufferData);

                    if (inject_delete && last_call_id_map.at(tid) == call->mCallNo) {
                        common::CallTM deleteGraphicBuffer("glDeleteGraphicBuffer_ARM");
                        deleteGraphicBuffer.mTid = tid;
                        deleteGraphicBuffer.mArgs.push_back(new common::ValueTM(image_id_map[tid]));       // name
                        deleteGraphicBuffer.mRet = common::ValueTM();          // void
                        writeout(target_file, &deleteGraphicBuffer);
                    }
                }
                else if (call->mCallName == "eglCreateImageKHR") {
                    if (inject_delete && last_call_id_map.at(tid) == call->mCallNo) {
                        common::CallTM deleteGraphicBuffer("glDeleteGraphicBuffer_ARM");
                        deleteGraphicBuffer.mTid = tid;
                        deleteGraphicBuffer.mArgs.push_back(new common::ValueTM(image_id_map[tid]));       // name
                        deleteGraphicBuffer.mRet = common::ValueTM();          // void
                        writeout(target_file, &deleteGraphicBuffer);
                    }
                }
            }
            else if (call->mCallName == "eglCreateImageKHR") {
                call->mArgs[1] = new common::ValueTM(0);
                call->mArgs[2] = new common::ValueTM(0x3140);       // 0x3140=EGL_NATIVE_BUFFER_ANDROID
                call->mArgs[3] = new common::ValueTM(image_id_map[tid]);
                process_eglCreateImageKHR(call, no_crop);
                writeout(target_file, call);

                if (inject_delete && last_call_id_map.at(tid) == call->mCallNo) {
                    common::CallTM deleteGraphicBuffer("glDeleteGraphicBuffer_ARM");
                    deleteGraphicBuffer.mTid = tid;
                    deleteGraphicBuffer.mArgs.push_back(new common::ValueTM(image_id_map[tid]));       // name
                    deleteGraphicBuffer.mRet = common::ValueTM();          // void
                    writeout(target_file, &deleteGraphicBuffer);
                }
            }
            else {
                writeout(target_file, call);
            }
        }
    }
    else {          // PIXEL_FORMAT_NONE means we don't want to convert the format
        common::CallTM *call = NULL;
        while ((call = source_file.NextCall())) {
            if (call->mCallName == "eglCreateImageKHR") {
                process_eglCreateImageKHR(call, no_crop);
                writeout(target_file, call);
            }
            else {
                writeout(target_file, call);
            }
        }
    }
    source_file.Close();
    target_file.Close();
    cout << "\nPat convertion from RGBA to YUV finished." << endl;

    return 0;
}

int main(int argc, char **argv)
{
    makeMap();
    Format format = YV12;
    PixelFormat pixelFormat = HAL_PIXEL_FORMAT_YV12;
    int argIndex = 1;
    unsigned int usage = GraphicBuffer::USAGE_SW_READ_NEVER | GraphicBuffer::USAGE_SW_WRITE_RARELY;
    bool no_crop = false;
    bool inject_delete = false;
    for (; argIndex < argc; ++argIndex)
    {
        string arg = argv[argIndex];

        if (arg[0] != '-')
            break;

        if (arg == "-h")
        {
            printHelp();
            return 1;
        }
        else if (arg == "-v")
        {
            printVersion();
            return 1;
        }
        else if (arg == "-injectdelete")
        {
            inject_delete = true;                       // inject glDeleteGraphicBuffer call, default off
        }
        else if (arg == "-f" && argIndex + 1 < argc)
        {
            string format_string = argv[argIndex + 1];
            if (format_string == "REMAIN")
            {
                format = REMAIN;
            }
            else if (format_string == "YV12")
            {
                format = YV12;
            }
            else if (format_string == "NV12")
            {
                format = NV12;
            }
            else
            {
                DBG_LOG("Invalid format: %s\n", format_string.c_str());
                exit(1);
            }

            auto iter = androidFormatMap.find(format);
            if (iter != androidFormatMap.end())
            {
                pixelFormat = iter->second;
            }
            else
            {
                DBG_LOG("Cannot find the corresponding pixel format of %s. "
                        "Please insert it in function void makeMap()\n", format_string.c_str());
                exit(1);
            }
            ++argIndex;
        }
        else if (arg == "-u" && argIndex + 1 < argc)
        {
            string usage_string = argv[argIndex + 1];
            usage = stoi(usage_string);
            cout << "usage = " << usage << endl;
            ++argIndex;
        }
        else if (arg == "-no_crop")
        {
            no_crop = true;
        }
        else
        {
            DBG_LOG("Error: Unknow option %s\n", arg.c_str());
            printHelp();
            return 1;
        }
    }

    if (argIndex + 2 > argc)
    {
        printHelp();
        return 1;
    }

    const char* source_trace_filename = argv[argIndex++];
    const char* target_trace_filename = argv[argIndex++];

    convert(source_trace_filename, target_trace_filename, pixelFormat, usage, no_crop, inject_delete);

    return 0;
}
