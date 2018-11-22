//
// Emergency trace version downgrader
//
// Makes trace files look like they have been created with a very old tracer by optimizing their sigbooks.
//
// To compile:
// gcc -o update_dictionary patrace/src/tool/update_dictionary.cpp patrace/src/common/out_file.cpp -Wall -g -O3 -I thirdparty/snappy -std=c++11 builds/patrace/x11_x64/debug/snappy/libsnappy_bundled.a -lstdc++ -I patrace/src
//

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <list>
#include <vector>
#include <map>
#include <stdbool.h>

#include <snappy.h>

#include "common/out_file.hpp"

#include "common/api_info_auto.cpp"

#define SNAPPY_CHUNK_SIZE (1*1024*1024)

/// Set this to true to write out an .ra file of the data. This can be used to verify this code by comparing to the .ra file
/// output of eg trace_to_txt -- it should be bit perfect identical.
#define WRITE_RA_FILE false

namespace os
{
	void abort() { ::abort(); }
	void log(const char *format, ...) { va_list ap; va_start(ap, format); fflush(stdout); vfprintf(stderr, format, ap); va_end(ap); }
};

void myread(void *data, size_t size, FILE *fp, const char *where)
{
	if (fread(data, size, 1, fp) != 1)
	{
		printf("%s: %s\n", where, strerror(ferror(fp)));
		exit(1);
	}
}

void mywrite(const void *data, size_t size, FILE *fp)
{
	if (WRITE_RA_FILE && fwrite(data, size, 1, fp) != 1)
	{
		printf("Failed to write: %s\n", strerror(ferror(fp)));
		exit(1);
	}
}

// no idea why we're doing conversion stuff only here but ignore it everywhere else
bool read_compressed_length(unsigned *length, FILE *in)
{
	unsigned char buf[4];
	if (fread(buf, sizeof(buf), 1, in) != 1)
	{
		if (feof(in))
		{
			return false; // done
		}
		printf("Error: %s\n", strerror(ferror(in)));
		exit(1);
	}
	*length  =  (size_t)buf[0];
	*length |= ((size_t)buf[1] <<  8);
	*length |= ((size_t)buf[2] << 16);
	*length |= ((size_t)buf[3] << 24);
	return true;
}

int main(int argc, char **argv)
{
	if (argc != 3)
	{
		printf("Usage: %s <in-file> <out-file>\n", argv[0]);
		exit(1);
	}
	FILE *in = fopen(argv[1], "rb");
	FILE *ra = nullptr;

	if (WRITE_RA_FILE)
	{
		ra = fopen("tmp.ra", "wb");
	}

	// Read header
	uint32_t headerToNext = 0;
	uint32_t magicNo = 0;
	uint32_t version = 0;
	uint32_t jsonLength = 0;
	uint64_t jsonFileBegin = 0;
	uint64_t jsonFileEnd = 0;
	if (fread(&headerToNext, sizeof(headerToNext), 1, in) != 1
	    || fread(&magicNo, sizeof(magicNo), 1, in) != 1
	    || fread(&version, sizeof(version), 1, in) != 1)
	{
		printf("Error: %s\n", strerror(ferror(in)));
		exit(1);
	}
	else if (magicNo != 0x20122012)
	{
		printf("Error: Not a valid trace file\n");
		exit(1);
	}
	else if (version < common::HEADER_VERSION_4)
	{
		printf("Error: Trace file too bad - must be at least version 4\n");
		exit(1);
	}
	myread(&jsonLength, sizeof(jsonLength), in, "jsonlength");
	myread(&jsonFileBegin, sizeof(jsonFileBegin), in, "jsonfilebegin");
	myread(&jsonFileEnd, sizeof(jsonFileEnd), in, "jsonfileend");
	std::vector<char> jsondata(jsonLength);
	myread(jsondata.data(), jsonLength, in, "reading JSON");
	fseek(in, jsonFileEnd, SEEK_SET);

	mywrite(&headerToNext, sizeof(headerToNext), ra);
	mywrite(&magicNo, sizeof(magicNo), ra);
	mywrite(&version, sizeof(version), ra);
	mywrite(&jsonLength, sizeof(jsonLength), ra);
	mywrite(&jsonFileBegin, sizeof(jsonFileBegin), ra);
	mywrite(&jsonFileEnd, sizeof(jsonFileEnd), ra);
	mywrite(jsondata.data(), jsonLength, ra);
	if (WRITE_RA_FILE)
	{
		fseek(ra, jsonFileEnd, SEEK_SET);
	}

	// Find size of uncompressed data buffer -- parsing everything twice, which is wildly inefficient but don't care
	uint32_t compressed_length = 0;
	size_t size = 0;
	size_t start_pos = ftell(in);
	size_t uncompressed_length = 0;
	std::vector<char> buffer_compressed;
	for (;;)
	{
		// Read chunk length
		if (!read_compressed_length(&compressed_length, in))
		{
			break;
		}
		buffer_compressed.resize(compressed_length);
		myread(buffer_compressed.data(), compressed_length, in, "reading chunk pass 1");
		if (snappy::GetUncompressedLength(buffer_compressed.data(), buffer_compressed.size(), &size) == false)
		{
			printf("Error checking chunk size (pass 1)\n");
		}
		//printf("Chunk size %lu kb\n", (unsigned long)size / 1024);
		uncompressed_length += size;
	}
	fseek(in, start_pos, SEEK_SET);

	// Read rest of file into memory
	std::vector<char> big_buffer;
	big_buffer.resize(uncompressed_length);
	size_t big_counter = 0;
	for (;;)
	{
		// Read chunk length
		if (!read_compressed_length(&compressed_length, in))
		{
			break;
		}
		buffer_compressed.resize(compressed_length);
		myread(buffer_compressed.data(), compressed_length, in, "reading chunk pass 2");
		if (snappy::GetUncompressedLength(buffer_compressed.data(), buffer_compressed.size(), &size) == false)
		{
			printf("Error checking chunk size (pass 2)\n");
			exit(1);
		}
		if (snappy::RawUncompress(buffer_compressed.data(), buffer_compressed.size(), &big_buffer.data()[big_counter]) == false)
		{
			printf("Error decompressing chunk (pass 2)\n");
			exit(1);
		}
		mywrite(&big_buffer.data()[big_counter], size, ra);
		big_counter += size;
	}
	if (ra)
	{
		fclose(ra);
	}

	// Read sigbook
	uint32_t mMaxSigId = 0;
	uint32_t sigbookToNext = 0;
	std::map<unsigned, std::string> stored_sigbook;
	std::map<int, int> map_old_to_new; // maps file Id to optimized set of functions
	std::map<int, int> map_new_to_old; // maps optimized Id to file Id
	std::map<int, int> map_old_to_internal; // maps file Id to code Ids, which may be different
	char *src = big_buffer.data();
	src = common::ReadFixed(src, sigbookToNext); // should be zero -- looks unimplemented
	src = common::ReadFixed(src, mMaxSigId);
	for (unsigned id = 1; id <= mMaxSigId; ++id)
	{
		uint32_t notused = 0;
		src = common::ReadFixed(src, notused); // should be identical to id, but never checked or used, alas
		char *str = nullptr;
		src = common::ReadString(src, str);
		if (str)
		{
			stored_sigbook[id] = std::string(str);
		}
	}

	// Create file Id to code Id mapping (should usually be identity)
	for (const auto pair : stored_sigbook)
	{
		for (unsigned i = 0; i <= common::ApiInfo::MaxSigId; i++)
		{
			if (common::ApiInfo::IdToNameArr[i] && pair.second == common::ApiInfo::IdToNameArr[i])
			{
				map_old_to_internal[pair.first] = i;
				break;
			}
		}
	}

	// Parse file contents, figure out what's actually used
	char *calls_start = src;
	while ((size_t)(src - big_buffer.data()) < uncompressed_length)
	{
		common::BCall *call = (common::BCall*)src;
		assert(call->funcId > 0);
		assert(call->funcId <= mMaxSigId);
		unsigned internalId = map_old_to_internal[call->funcId];
		assert(internalId > 0);
		// now comes the fun part... for some reason we decided to make the design of our file format a lot more complicated to save 4 bytes each call...
		uint32_t call_size = common::ApiInfo::IdToLenArr[internalId];
		if (call_size == 0) // vlen type
		{
			common::BCall_vlen *vcall = (common::BCall_vlen*)src;
			call_size = vcall->toNext;
			//printf("function vlen %s : size=%lu - %u\n", common::ApiInfo::IdToNameArr[internalId], (unsigned long)call_size, (unsigned)sizeof(common::BCall_vlen));
		}
		else // fixed size
		{
			//printf("function fixed %s : size=%lu - %u\n", common::ApiInfo::IdToNameArr[internalId], (unsigned long)call_size, (unsigned)sizeof(common::BCall));
		}
		src += call_size;
		if (map_old_to_new.count(call->funcId) == 0) // we haven't seen this call before
		{
			int newid = map_old_to_new.size() + 4; // we just skip the first 4 for some unknown reason
			map_old_to_new[call->funcId] = newid;
			map_new_to_old[newid] = call->funcId;
		}
	}

	// Now finally - write everything out again!
	common::OutFile out;
	out.Open(argv[2], false);
	out.WriteHeader(jsondata.data(), jsonLength, true);

	std::vector<char> scratch(70*1024*1024); // why, it works for the tracer!!
	// Write new sigbook
	char *dest = scratch.data();
	uint32_t* toNext = (uint32_t*)dest;
	dest = common::WriteFixed<uint32_t>(dest, 0); // leave open slot
	dest = common::WriteFixed<uint32_t>(dest, (uint32_t)map_old_to_new.size() + 4 - 1); // index of last, not size
	// write some noise in the beginning... 3 reserved entries (for unknown reasons)
	dest = common::WriteFixed<uint32_t>(dest, 1);
	dest = common::WriteString(dest, "");
	dest = common::WriteFixed<uint32_t>(dest, 2);
	dest = common::WriteString(dest, "");
	dest = common::WriteFixed<uint32_t>(dest, 3);
	dest = common::WriteString(dest, "");
	for (uint32_t id = 4; id < map_old_to_new.size() + 4; id++)
	{
		dest = common::WriteFixed<uint32_t>(dest, id);
		dest = common::WriteString(dest, stored_sigbook[map_new_to_old[id]].c_str());
	}
	*toNext = dest - scratch.data(); // set size of block
	out.Write(scratch.data(), dest - scratch.data());
	// Write new calls
	src = calls_start; // iterate over this memory, again
	while ((size_t)(src - big_buffer.data()) < uncompressed_length)
	{
		dest = scratch.data();
		common::BCall *call = (common::BCall*)src;
		unsigned internalId = map_old_to_internal[call->funcId];
		unsigned newId = map_old_to_new[call->funcId];
		uint32_t call_size = common::ApiInfo::IdToLenArr[internalId];
		if (call_size == 0) // vlen type
		{
			common::BCall_vlen *vcall = (common::BCall_vlen*)src;
			call_size = vcall->toNext;
			common::BCall_vlen *destcall = (common::BCall_vlen*)dest;
			*destcall = *vcall;
			destcall->funcId = newId;
			dest += sizeof(*destcall);
			memcpy(dest, src + sizeof(*destcall), call_size - sizeof(*destcall)); // copy parameters - hopefully not bigger than 70mb!!
		}
		else // fixed size
		{
			common::BCall *destcall = (common::BCall*)dest;
			*destcall = *call;
			destcall->funcId = newId;
			dest += sizeof(*destcall);
			memcpy(dest, src + sizeof(*destcall), call_size - sizeof(*destcall)); // copy parameters - hopefully not bigger than 70mb!!
		}
		src += call_size;
		out.Write(scratch.data(), call_size);
	}
	out.Close();

	return 0;
}
