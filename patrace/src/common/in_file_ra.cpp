#include <common/in_file_ra.hpp>
#include <libgen.h> // basename()
#include <sstream>

namespace common {

static bool StrEndWith(const char* str, const char* substr)
{
    int strl = strlen(str);
    int subl = strlen(substr);
    if (strl < subl)
        return false;

    for (int i = subl; i > 0; i--) {
        if (str[strl-i] != substr[subl-i])
            return false;
    }
    return true;
}

bool InFileRA::Open(const char *name, bool readHeaderAndExit)
{
    mFileName = name;

    // generate RA (random access) file if needed
    if (!StrEndWith(name, "ra"))
    {
        if (!CreateRAFile(name, mTarget))
            return false;
    }

    std::ios_base::openmode fmode = std::fstream::binary | std::fstream::in;
    mStream.open(mFileName, fmode);
    if (!mStream.is_open() || !mStream.good())
    {
        DBG_LOG("Failed to open file %s\n", mFileName.c_str());
        mStream.close();
        return false;
    }

    // Read Base Header that is common for all header versions
    common::BHeader bHeader;
    mStream.read((char*)&bHeader, sizeof(bHeader));
    if (bHeader.magicNo != 0x20122012)
    {
        DBG_LOG("Warning: %s seems to be an invalid trace file!\n", mFileName.c_str());
        return false;
    }

    mStream.seekg(0, std::ios_base::beg); // rewind to beginning of file
    mHeaderVer = static_cast<HeaderVersion>(bHeader.version);

    DBG_LOG("### .pat file format Version %d ###\n", bHeader.version - HEADER_VERSION_1 + 1);

    if (bHeader.version == HEADER_VERSION_1) {
        BHeaderV1 hdr;
        mStream.read((char*)&hdr, sizeof(hdr));
        mHeaderParseComplete = parseHeader(hdr, mJsonHeader);
    } else if (bHeader.version == HEADER_VERSION_2) {
        BHeaderV2 hdr;
        mStream.read((char*)&hdr, sizeof(hdr));
        mHeaderParseComplete = parseHeader(hdr, mJsonHeader);
    } else if (bHeader.version >= HEADER_VERSION_3 && bHeader.version <= HEADER_VERSION_4) {
        BHeaderV3 hdr;
        mStream.read((char*)&hdr, sizeof(hdr));
        mHeaderParseComplete = parseHeader(hdr, mJsonHeader);
        mStream.seekg(hdr.jsonFileEnd, std::ios_base::beg);
    } else {
        DBG_LOG("Unsupported file format version: %d\n", bHeader.version - HEADER_VERSION_1 + 1);
        return false;
    }

    if (!mHeaderParseComplete)
    {
        return false;
    }

    // when we only wanted to use -info to see header contents, no playback
    if (readHeaderAndExit)
    {
        return true;
    }
    mIsOpen = true;

    // read signature book
    ReadSigBook();

    return true;
}

unsigned int InFileRA::ReadCompressedLength(std::fstream& inStream)
{
    unsigned char buf[4];
    unsigned int length;
    inStream.read((char *)buf, sizeof(buf));
    if (inStream.fail()) {
        length = 0;
    } else {
        length  =  (size_t)buf[0];
        length |= ((size_t)buf[1] <<  8);
        length |= ((size_t)buf[2] << 16);
        length |= ((size_t)buf[3] << 24);
    }
    return length;
}

static void streamCopyBytes(std::istream& in, std::size_t count, std::ostream& out)
{
    // copy count bytes from in to out
    const std::size_t buffer_size = 4096;
    char buffer[buffer_size];
    while(count > buffer_size)
    {
        in.read(buffer, buffer_size);
        out.write(buffer, buffer_size);
        count -= buffer_size;
    }

    in.read(buffer, count);
    out.write(buffer, count);
}

bool InFileRA::CreateRAFile(const char* name, const std::string& target)
{
    std::ios_base::openmode readMode = std::fstream::binary | std::fstream::in;
    std::fstream inStream;
    std::fstream outStream;

    inStream.open(name, readMode);
    if (!inStream.is_open())
    {
        DBG_LOG("Failed to open file %s\n", name);
        return false;
    }

    if (target.empty())
    {
        // Extract filename from path (on copy because basename() might modify input)
        char* nameCopy = strdup(name);
        {
            // Construct RA filename. Will be created in current working dir.
            std::stringstream ss;
            ss << basename(nameCopy) << ".ra";
            mFileName = ss.str();
        }
        free(nameCopy); // basename() might use/modify input, so this needs to be kept alive until done

        DBG_LOG("Creating temporary random access file %s\n", mFileName.c_str());
    }
    else
    {
        mFileName = target;
    }

    outStream.open(mFileName.c_str(), std::fstream::binary | std::fstream::out | std::fstream::trunc);

    // header part
    {
        common::BHeader bHeader;
        inStream.read((char*)&bHeader, sizeof(bHeader));
        if (bHeader.magicNo != 0x20122012) {
            DBG_LOG("Warning: %s seems to be an invalid trace file!\n", name);
            bHeader.version = 0; // only for supporting legacy trace files
        }

        inStream.seekg(0, std::ios_base::beg);
        if (bHeader.version == 0) {
            DBG_LOG("Warning: Treat it as patrace version 1 by default!\n");
            common::BHeaderV1   bHeaderV1;
            outStream.write((char*)&bHeaderV1, sizeof(bHeaderV1));
        }
        else if (bHeader.version == HEADER_VERSION_1) {
            common::BHeaderV1   bHeaderV1;
            inStream.read((char*)&bHeaderV1, sizeof(bHeaderV1));
            outStream.write((char*)&bHeaderV1, sizeof(bHeaderV1));
        }
        else if (bHeader.version == HEADER_VERSION_2) {
            common::BHeaderV2   bHeaderV2;
            inStream.read((char*)&bHeaderV2, sizeof(bHeaderV2));
            outStream.write((char*)&bHeaderV2, sizeof(bHeaderV2));
        }
        else if (bHeader.version >= HEADER_VERSION_3 && bHeader.version <= HEADER_VERSION_4) {
            common::BHeaderV3   bHeaderV3;
            inStream.read((char*)&bHeaderV3, sizeof(bHeaderV3));
            outStream.write((char*)&bHeaderV3, sizeof(bHeaderV3));

            inStream.seekg( bHeaderV3.jsonFileBegin, std::ios_base::beg );
            long long headerSize = bHeaderV3.jsonFileEnd - bHeaderV3.jsonFileBegin;
            streamCopyBytes(inStream, headerSize, outStream);

            if ((long long)(inStream.tellg()) != bHeaderV3.jsonFileEnd) {
                DBG_LOG("inStream tellg != bHeaderV3.jsonFileEnd\n");
                os::abort();
            }
        }
        else {
            DBG_LOG("Not supported file version: %d\n", bHeader.version);
            Close();
            return false;
        }
    }


    // content part
    char*               compressedCache = NULL;
    unsigned int        compressedCacheLen = 0;
    char*               unCompressedCache = NULL;
    unsigned int        unCompressedCacheLen = 0;

    while ( !inStream.eof() )
    {
        unsigned int compressedLength = ReadCompressedLength(inStream);
        size_t uncompressedLength = 0;
        if (compressedLength)
        {
            if (compressedCacheLen < compressedLength)
            {
                delete [] compressedCache;
                compressedCacheLen = compressedLength;
                compressedCache = new char [compressedCacheLen];
            }
        }

        inStream.read(compressedCache, compressedLength);
        if (!snappy::GetUncompressedLength(compressedCache, (size_t)compressedLength, &uncompressedLength) && compressedLength > 0)
        {
            DBG_LOG("Failed to parse chunk of size %u - file corrupt - aborting!\n", compressedLength);
            os::abort();
        }
        if (unCompressedCacheLen < uncompressedLength)
        {
            delete [] unCompressedCache;
            unCompressedCacheLen = uncompressedLength;
            unCompressedCache = new char [unCompressedCacheLen];
        }
        if (!snappy::RawUncompress(compressedCache, compressedLength, unCompressedCache) && compressedLength > 0)
        {
            DBG_LOG("Failed to decompress chunk of size %u - file is corrupt - aborting!\n", compressedLength);
            os::abort();
        }

        outStream.write(unCompressedCache, uncompressedLength);
    }

    delete [] compressedCache;
    delete [] unCompressedCache;

    outStream.close();
    inStream.close();
    return true;
}

void InFileRA::ReadSigBook()
{
    unsigned int toNext;
    mStream.read((char*)&toNext, sizeof(toNext));
    this->ReadChunk(toNext - sizeof(toNext));

    char* src = mCache;
    src = ReadFixed(src, mMaxSigId);

    if (mMaxSigId > ApiInfo::MaxSigId) {
        DBG_LOG("mMaxSigId (%d) > ApiInfo::MaxSigId (%d)\n", mMaxSigId, ApiInfo::MaxSigId);

        os::abort();
    } else if (mMaxSigId == 0) {
        DBG_LOG("mMaxSigId = %d\n", mMaxSigId);
        os::abort();
    }

    mExIdToName.resize(mMaxSigId + 1);
    mExIdToName[0] = "";
    for (unsigned short id = 1; id <= mMaxSigId; ++id)
    {
        unsigned int id_notused;
        char *src_tmp = nullptr;
        src_tmp = ReadFixed<unsigned int>(src, id_notused);
        if(id == id_notused)
        {
            src = src_tmp;
            char *str;
            src = ReadString(src, str);
            mExIdToName[id] = str ? str : "";
        }
        else
        {
            mExIdToName[id] = "";
            continue;
        }
    }

    if (mExIdToLen) delete [] mExIdToLen;
    mExIdToLen = new int[mMaxSigId + 1];
    if (mExIdToFunc) delete [] mExIdToFunc;
    mExIdToFunc = new void*[mMaxSigId + 1];

    mExIdToLen[0] = 0;
    mExIdToFunc[0] = 0;
    for (unsigned short id = 1; id <= mMaxSigId; ++id)
    {
        const char* name = mExIdToName.at(id).c_str();
        mExIdToLen[id] = gApiInfo.NameToLen(name);
        mExIdToFunc[id] = gApiInfo.NameToFptr(name);
    }
}

void InFileRA::copySigBook(std::vector<std::string> &sigbook)
{
    sigbook.push_back("");
    for (unsigned short id = 1; id <= mMaxSigId; ++id)
    {
        sigbook.push_back(mExIdToName.at(id));
    }
}

}
