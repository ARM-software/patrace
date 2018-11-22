#include <common/in_file_ra.hpp>
#include <libgen.h> // basename()

namespace common {

static bool StrEndWith(const char* str, const char* substr) {
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

bool InFileRA::Open(const char* name, bool readHeaderAndExit, const std::string& target)
{
    mFileName = name;
    if (readHeaderAndExit)
    {
        return InFileBase::Open();
    }

    // generate RA (random access) file if needed
    if (!StrEndWith(name, "ra"))
    {
        if (!CreateRAFile(name, target))
            return false;
    }

    if (!InFileBase::Open())
        return false;

    // when we only wanted to use -info to see header contents, no playback
    if ( readHeaderAndExit ) {
        return true;
    }

    // read signature book
    ReadSigBook();

    return true;
}

void InFileRA::Close()
{
    InFileBase::Close();
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

void streamCopyBytes(std::istream& in, std::size_t count, std::ostream& out)
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

    mStream.open(mFileName.c_str(), std::fstream::binary | std::fstream::out | std::fstream::trunc);

    // header part
    {
        common::BHeader bHeader;
        inStream.read((char*)&bHeader, sizeof(bHeader));
        if (bHeader.magicNo != 0x20122012) {
            DBG_LOG("Warning: %s seems to be an invalid trace file!\n", name);
            bHeader.version = 0; // only for supporting legency trace files
        }

        inStream.seekg(0, std::ios_base::beg);
        if (bHeader.version == 0) {
            DBG_LOG("Warning: Treat it as patrace version 1 by default!\n");
            common::BHeaderV1   bHeaderV1;
            mStream.write((char*)&bHeaderV1, sizeof(bHeaderV1));
        }
        else if (bHeader.version == HEADER_VERSION_1) {
            common::BHeaderV1   bHeaderV1;
            inStream.read((char*)&bHeaderV1, sizeof(bHeaderV1));
            mStream.write((char*)&bHeaderV1, sizeof(bHeaderV1));
        }
        else if (bHeader.version == HEADER_VERSION_2) {
            common::BHeaderV2   bHeaderV2;
            inStream.read((char*)&bHeaderV2, sizeof(bHeaderV2));
            mStream.write((char*)&bHeaderV2, sizeof(bHeaderV2));
        }
        else if (bHeader.version >= HEADER_VERSION_3 || bHeader.version <= HEADER_VERSION_4) {
            common::BHeaderV3   bHeaderV3;
            inStream.read((char*)&bHeaderV3, sizeof(bHeaderV3));
            mStream.write((char*)&bHeaderV3, sizeof(bHeaderV3));

            inStream.seekg( bHeaderV3.jsonFileBegin, std::ios_base::beg );
            long long headerSize = bHeaderV3.jsonFileEnd - bHeaderV3.jsonFileBegin;
            streamCopyBytes(inStream, headerSize, mStream);

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
        ::snappy::GetUncompressedLength(compressedCache, (size_t)compressedLength,
            &uncompressedLength);
        if (unCompressedCacheLen < uncompressedLength)
        {
            delete [] unCompressedCache;
            unCompressedCacheLen = uncompressedLength;
            unCompressedCache = new char [unCompressedCacheLen];
        }
        ::snappy::RawUncompress(compressedCache, compressedLength,
            unCompressedCache);

        mStream.write(unCompressedCache, uncompressedLength);
    }

    delete [] compressedCache;
    delete [] unCompressedCache;

    mStream.close();
    return true;
}

void InFileRA::ReadSigBook() {
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

    if (mExIdToName) delete [] mExIdToName;
    mExIdToName = new std::string[mMaxSigId + 1];
    mExIdToName[0] = "";
    for (unsigned short id = 1; id <= mMaxSigId; ++id)
    {
        unsigned int id_notused;
        src = ReadFixed<unsigned int>(src, id_notused);
        char *str;
        src = ReadString(src, str);
        mExIdToName[id] = str ? str : "";
        // DBG_LOG("    %d => %s\n", id, mExIdToName[id].c_str());
    }

    if (mExIdToLen) delete [] mExIdToLen;
    mExIdToLen = new int[mMaxSigId + 1];
    if (mExIdToFunc) delete [] mExIdToFunc;
    mExIdToFunc = new void*[mMaxSigId + 1];

    mExIdToLen[0] = 0;
    mExIdToFunc[0] = 0;
    for (unsigned short id = 1; id <= mMaxSigId; ++id)
    {
        const char* name = mExIdToName[id].c_str();
        mExIdToLen[id] = gApiInfo.NameToLen(name);
        mExIdToFunc[id] = gApiInfo.NameToFptr(name);
    }

}

}
