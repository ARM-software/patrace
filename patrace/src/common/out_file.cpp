#include <common/out_file.hpp>

#include <vector>
#include <common/os.hpp>
#include <common/api_info.hpp>
#include <common/pa_exception.h>

#include <snappy.h> // Compression

namespace common {

OutFile::OutFile()
 : mHeader()
 , mIsOpen(false)
 , mStream()
 , mCache(NULL)
 , mCacheLen(0)
 , mCacheP(NULL)
 , mCompressedCache(NULL)
 , mCompressedCacheLen(0)
 , mFileName()
{}

OutFile::OutFile(const char *name)
 : mHeader()
 , mIsOpen(false)
 , mStream()
 , mCache(NULL)
 , mCacheLen(0)
 , mCacheP(NULL)
 , mCompressedCache(NULL)
 , mCompressedCacheLen(0)
 , mFileName()
{
    Open(name);
}

OutFile::~OutFile()
{
    Close();
}

bool OutFile::Open(const char* name, bool writeSigBook, const std::vector<std::string> *sigbook)
{
    os::String autogenFileName;
    if (name == NULL) {
        autogenFileName = AutogenTraceFileName();
        name = autogenFileName;
    }

    mStream.close(); // Make sure it's closed (sets failbit if not open)
    mStream.clear(); // Clear state-bits

    std::ios_base::openmode fmode =
        std::fstream::binary | std::fstream::out | std::fstream::trunc;
    mStream.open(name, fmode);

    if (!mStream.is_open() || !mStream.good()) {
        DBG_LOG("Failed to open file %s\n", name);
        return false;
    } else {
        DBG_LOG("Successfully open file %s\n", name);
    }

    mFileName = name;
    mIsOpen = true;

    // It will be re-written before the file is closed.
    mStream.write((char*)&mHeader, sizeof(BHeaderV3) );
    mHeader.jsonFileBegin = mStream.tellp();
    // reserve 512k at beginning of file for json data
    std::vector<char> zerobuf(mHeader.jsonMaxLength);
    mStream.write(zerobuf.data(), zerobuf.size());
    long long jsonEnd = (long long)mStream.tellp();
    if (mHeader.jsonFileEnd == jsonEnd) {
        DBG_LOG("json file end calculated correctly, endoffs: %lld\n", jsonEnd );
    } else {
        DBG_LOG("json file end wrong\n");
        mHeader.jsonFileEnd = jsonEnd; // is this more robust than calculating it beforehand, assuming all bytes we have is header+jsonMaxLength?
    }

    CreateCache(SNAPPY_CHUNK_SIZE);
    if (writeSigBook)
    {
        if (sigbook)
            WriteSigBook(sigbook);
        else
            WriteSigBook(NULL);
    }

    return true;
}

void OutFile::Close()
{
    if (!mIsOpen)
        return;

    Flush();
    mStream.seekp(0, std::ios_base::beg);
    mStream.write((char*)&mHeader, sizeof(BHeaderV3));

    // Verify all writes went OK.
    if (!mStream.good())
    {
        throw PA_EXCEPTION("Detected bad stream while closing OutFile. Writes might have failed.");
    }

    mIsOpen = false;
    mStream.close();
    DBG_LOG("Close trace file %s\n", mFileName.c_str());

    delete [] mCache;
    mCache = NULL;
    mCacheLen = 0;
    mCacheP = NULL;
    delete [] mCompressedCache;
    mCompressedCache = NULL;
    mCompressedCacheLen = 0;
}

void OutFile::Flush()
{
    unsigned int len = UsedSize();
    if (len == 0)
        return;

    size_t compressedLen;
    ::snappy::RawCompress(mCache, len, mCompressedCache, &compressedLen);
    WriteCompressedLength((unsigned int)compressedLen);
    mStream.write(mCompressedCache, compressedLen);
    mStream.flush();
    mCacheP = mCache;
}

void OutFile::FlushHeader()
{
    std::ios::streampos curP = mStream.tellp();
    mStream.seekp(0, std::ios_base::beg);
    mStream.write((char*)&mHeader, sizeof(BHeaderV3));
    mStream.seekp(curP, std::ios_base::beg);
    mStream.flush();
}

void OutFile::WriteHeader(const char* buf, unsigned int len, bool verbose)
{
    if (!mIsOpen) {
        DBG_LOG("cant write header when file is closed!\n");
        return;
    }

    // write variable length header to beginning of file, then seek back to previous file put position
    Flush(); // flush last compressed part
    if ( len > mHeader.jsonMaxLength ) {
        DBG_LOG("Error: json file too long for header, %d > %d\n", len, mHeader.jsonMaxLength);
        os::abort();
    } else {
        std::ios::streampos oldP = mStream.tellp();
        mStream.seekp(mHeader.jsonFileBegin, std::ios_base::beg);
        mStream.write(buf,len);
        mHeader.jsonLength = len;
        if (verbose)
        {
            DBG_LOG("wrote json header, length=%d\n", mHeader.jsonLength);
        }
        mStream.seekp(oldP, std::ios_base::beg);

        FlushHeader();
    }
}

void OutFile::CreateCache(int len)
{
    if (len <= mCacheLen)
        return;

    delete [] mCache;
    delete [] mCompressedCache;

    mCacheLen = len;
    mCompressedCacheLen = snappy::MaxCompressedLength(mCacheLen);
    mCache = new char[mCacheLen];
    mCacheP = mCache;
    mCompressedCache = new char[mCompressedCacheLen];
}

void OutFile::WriteSigBook(const std::vector<std::string> *sigbook)
{
    char* buf = new char[1024*1024];
    char* dest = buf;

    unsigned int* toNext = (unsigned int*)dest;
    dest = WriteFixed<unsigned int>(dest, 0);           // leave a slot
    if (sigbook)
    {
        dest = WriteFixed<unsigned int>(dest, sigbook->size() - 1);   // cnt
        for (unsigned short id = 1; id <= sigbook->size() - 1; ++id)
        {
            dest = WriteFixed<unsigned int>(dest, id);
            dest = WriteString(dest, sigbook->at(id).c_str());
        }
    }
    else
    {
        dest = WriteFixed<unsigned int>(dest, ApiInfo::MaxSigId);   // cnt
        for (unsigned short id = 1; id <= ApiInfo::MaxSigId; ++id)
        {
            dest = WriteFixed<unsigned int>(dest, id);
            dest = WriteString(dest, ApiInfo::IdToNameArr[id]);
        }
    }

    *toNext = dest-buf;

    Write(buf, dest-buf);

    delete [] buf;
}

os::String OutFile::AutogenTraceFileName()
{
    os::String filename;

#ifdef ANDROID
    unsigned int sCounter = 0;
    os::String prefix = "/data/apitrace/";
    os::String process = os::getProcessName();
    prefix.join(process);

    while(true) {
        if (sCounter)
            filename = os::String::format("%s.%u.pat", prefix.str(), sCounter);
        else
            filename = os::String::format("%s.pat", prefix.str());

        FILE *file;
        file = fopen(filename, "rb");
        if (file == NULL)
            break;
        fclose(file);

        ++sCounter;
    }
#else
    filename = getenv("OUT_TRACE_FILE");
#endif

    return filename;
}

std::string OutFile::getFileName() const
{
    return mFileName;
}

}
