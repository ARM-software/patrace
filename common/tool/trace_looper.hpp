#pragma once
#ifndef TRACE_LOOPER_HPP
#define TRACE_LOOPER_HPP

#include <ctime>
#include <vector>
#include <string>
#include <utility>
#include <unistd.h>
#include <unordered_map>
#include <unordered_set>

#include <common/memory.hpp>
#include <common/out_file.hpp>
#include <common/trace_model.hpp>
#include <json/writer.h>

#define TRACE_LOOPER_VERSION "r0p3"

void log_message(
    const char* filename,
    uint32_t linenr,
    const std::string& level,
    const std::string& message);

void log_error(
    const char* filename,
    uint32_t linenr,
    const std::string& level,
    const std::string& message);


#define DEBUGLOG(message) log_message(__FILE__, __LINE__, "DEBUG: ", message);
#define OKLOG(message) log_message(__FILE__, __LINE__, "SUCCESS: ", message);
#define ILOG(message) log_message(__FILE__, __LINE__, "INFO: ", message);
#define WLOG(message) log_message(__FILE__, __LINE__, "WARNING: ", message);
#define ELOG(message) log_error(__FILE__, __LINE__, "ERROR: ", message);
#define FELOG(message) log_error(__FILE__, __LINE__, "FATAL_ERROR: ", message);

bool convert_string_to_int(const std::string& str, int* result);
const std::vector<std::string> split(const std::string& str, const char& delimiter);
const std::string join(const std::vector<std::string>& tokens, char joiner);


class TraceLooper {
 public:
    TraceLooper(int argc, char* argv[]);
    ~TraceLooper();

    void load();
    void process();
    void save();

    void add_call(const common::CallTM* incall);
    void add_frame_loop_calls(int target_frame, int num_loops, bool reset_loop_state);
    void add_range_loop_calls(int start_call, int end_call, int num_loops);

    void print_trace_info();

    static bool call_is_state_changer(const common::CallTM* call);
    static bool call_is_draw(const common::CallTM* call);
    static bool call_is_swap(const common::CallTM* call);
    static const std::unordered_set<std::string> GL_DRAW_CALL_NAMES;
    static const std::unordered_set<std::string> EXCLUDED_PRE_FRAME_CALLS;
    static const std::unordered_set<std::string> SWAP_CALL_NAMES;

 private:
    TraceLooper() = delete;

    int main_thread_ID;

    std::string full_cmd_line;
    std::string timestamp_string;
    std::string username;

    std::unordered_map<std::string, int> int_args;
    std::unordered_map<std::string, std::string> str_args;
    std::unordered_map<std::string, bool> bool_args;

    std::vector<std::pair<int, int>> frame_ranges;
    std::vector<common::CallTM*> calls;
    std::vector<common::CallTM*> prestate_calls;
    std::vector<common::CallTM*> frame_loop_call_writelist;
    std::vector<common::CallTM*> range_loop_call_writelist;

    std::vector<std::string> cleaned_calls;
    std::vector<std::string> pre_state_calls;

    std::string header;

    void clean_loop_frame_calls(
        std::vector<common::CallTM*>* calls,
        std::vector<common::CallTM*>* clean_calls);

    void build_output_file_path();
    void build_output_file_path_range_trace();
    void extract_pre_frame_state_calls();

    static void print_args_info();
    void set_arg_defaults();
    void parse_args(
        int argc,
        char* argv[]);
    bool args_are_valid();

    std::string calc_trace_md5();
    void get_range_loop_header(std::string& out_string, const std::string& md5_string);
    void get_frame_loop_header(std::string& out_string, const std::string& md5_string);
};


#endif
