#include "collector_utility.hpp"

#include <dirent.h>
#include <string.h>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <climits>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <assert.h>


void splitString(const char* s, char delimiter, std::vector<std::string>& tokens)
{
    std::stringstream f(s);
    std::string str;
    while (std::getline(f, str, delimiter))
    {
        tokens.push_back(str);
    }
}


static std::vector<std::string> splitString(const char* s, char delimiter)
{
    std::vector<std::string> ret;
    std::stringstream f(s);
    std::string str;
    while (std::getline(f, str, delimiter))
    {
        ret.push_back(str);
    }
    return ret;
}

// returns empty string if not found
std::string getMidgardInstrOutputPath()
{
    const char* env_output_dir = getenv("INSTR_OUTPUT_DIR");
    std::string cinstrOutputDir;
    if (env_output_dir)
    {
        cinstrOutputDir = std::string(env_output_dir);
        DBG_LOG("cinstr plugin output path is '%s'\n", cinstrOutputDir.c_str());
        return cinstrOutputDir;
    }

    std::string instrConfigFilePath = getMidgardInstrConfigPath();

    if (instrConfigFilePath.size() > 0)
    {
        std::ifstream infile(instrConfigFilePath.c_str());
        std::string line;
        while (std::getline(infile, line))
        {
            if (line.size() > 5 && line.compare(0, 9, "outdir = ") == 0)
            {
                cinstrOutputDir = line.substr(9);
                DBG_LOG("cinstr plugin output path is '%s'\n", cinstrOutputDir.c_str());
                return cinstrOutputDir;
            }
        }
        DBG_LOG("Couldn't find the cinstr plugin output path in '%s'.\n", instrConfigFilePath.c_str());
    }
    else
    {
        DBG_LOG("Couldn't find the path to instrumentation config.\n");
    }

    return "";
}

std::string getMidgardInstrConfigPath()
{
    // Just pick the first file we find that has .instr_config in it.
    // NOTE: default.instr_config won't work because the cinstr plugin
    // only supports one process at a time, and the gateway client
    // also uses the DDK when figuring out the supported texture
    // formats.
    DIR *dir;
    if ((dir = opendir (".")) != NULL)
    {
        struct dirent *ent;
        while ((ent = readdir (dir)) != NULL)
        {
            if (strstr(ent->d_name, ".instr_config") != NULL &&
                    ent->d_type == DT_REG)
            {
                return std::string(ent->d_name);
            }
        }
        closedir(dir);
    }

    return "";
}

// Returns true if found, false if not
static bool searchMemoryAreaForKeyword(const char* data, size_t dataLen, const std::string& keyword)
{
    // Implementation of the Horspool algorithm to search for "keyword" inside data.
    // Based on http://www-igm.univ-mlv.fr/~lecroq/string/node18.html

    const unsigned char* x = (unsigned char*) keyword.c_str();
    const int m = keyword.size();

    const unsigned char* y = (unsigned char*) data;
    const int n = dataLen;

    // Generate bad-character shift table
    int ASIZE = UCHAR_MAX+1; // Alphabet size
    int bmBc[ASIZE];

    for (int i = 0; i < ASIZE; ++i)
        bmBc[i] = m;
    for (int i = 0; i < m - 1; ++i)
        bmBc[x[i]] = m - i - 1;

    // Do the search
    int j = 0;
    while (j <= n - m)
    {
        unsigned char c = y[j + m - 1];

        if (x[m - 1] == c && memcmp(x, y + j, m - 1) == 0)
            return true; // Found

        j += bmBc[c];
    }

    // Not found
    return false;
}

static bool fileHasKeyword(const std::string& filePath, const std::string& keyword)
{
    // Below, after checking the file exists, we mmap it and use a string-search
    // algorithm to search through it, looking for "keyword".

    struct stat sbuf;
    if (stat(filePath.c_str(), &sbuf) == -1)
    {
        DBG_LOG("Couldn't stat file %s\n", filePath.c_str());
        return false;
    }


    int fd = open(filePath.c_str(), O_RDONLY);
    if (fd == -1)
    {
        DBG_LOG("Couldn't open file %s\n", filePath.c_str());
        return false;
    }

    void* data = mmap((caddr_t) 0, sbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED)
    {
        DBG_LOG("Couldn't mmap file %s\n", filePath.c_str());
        close(fd);
        return false;
    }

    bool foundInFile = searchMemoryAreaForKeyword((const char*) data, sbuf.st_size, keyword);

    if (munmap(data, sbuf.st_size) == -1)
    {
        DBG_LOG("Error un-mmapping the file %s\n", filePath.c_str());
    }

    close(fd);

    return foundInFile;
}

static bool checkDDK(const std::string& filePath, const std::string& keyword)
{
    DBG_LOG("Checking whether %s supports instrumentation key \"%s\". This may take a while...\n", filePath.c_str(), keyword.c_str());
    if (fileHasKeyword(filePath, keyword))
    {
        DBG_LOG("Found instrumentation support.\n");
        return true;
    }
    else
    {
        DBG_LOG("No instr support compiled in Mali driver.\n");
        return false;
    }
}

bool DDKHasInstrCompiledIn(const std::string &keyword)
{
    /* Check if the DDK was compiled with instrumentation enabled.
     * Using dlopen and dlsym won't help because the symbols aren't dynamic.
     */

    std::vector<std::string> possible_paths;

#if defined(ANDROID)
    const std::string soName = "/libGLES_mali.so";

    possible_paths.push_back("/system/vendor/lib/egl" + soName);
    possible_paths.push_back("/system/lib/egl" + soName);
#elif defined(__linux__)
    const char* ld_library_path = getenv("LD_LIBRARY_PATH");

    if (ld_library_path)
    {
        possible_paths = splitString(ld_library_path, ':');
    }

    const std::string soName = "/libmali.so";
    possible_paths.push_back("/lib" + soName);
    possible_paths.push_back("/usr/lib" + soName);

#else
    // other systems - no checking implemented
    DBG_LOG("Not Android nor Linux, so not checking for instrumentation support. (Keyword: %s.)\n", keyword.c_str());
    return false;
#endif

    for (std::vector<std::string>::const_iterator it = possible_paths.begin();
            it != possible_paths.end();
            ++it)
    {
        std::string path = *it + soName;

        struct stat statbuf;
        if (stat(path.c_str(), &statbuf) == 0)
        {
            // Check first match
            return checkDDK(path, keyword);
        }
    }

    DBG_LOG("Couldn't locate %s - assuming instr support NOT compiled in.\n", soName.c_str());
    return false;
}


#ifdef __ANDROID__
int32_t _stoi(const std::string& str)
{
    int32_t value;
    std::stringstream sstr(str);
    if(!(sstr >> value)) {
        DBG_LOG("Failed to convert string: %s to a integer.\n", str.c_str());
        assert(false);
    }

    return value;
}


uint64_t _stol(const std::string& str)
{
    uint64_t value;
    std::stringstream sstr(str);
    if(!(sstr >> value)) {
        DBG_LOG("Failed to convert string: %s to a long long integer.\n", str.c_str());
        assert(false);
    }

    return value;
}

#endif
