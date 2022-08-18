//
// Print the dictionary
//
// To compile:
// gcc -o print_dictionary patrace/src/tool/print_dictionary.cpp -Wall -g -O3 -I thirdparty/snappy -std=c++11 builds/patrace/x11_x64/debug/snappy/libsnappy_bundled.a -lstdc++ -I patrace/src
//

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <list>
#include <vector>
#include <map>
#include <stdbool.h>

#include <snappy.h>

#include "common/api_info_auto.cpp"

#define SNAPPY_CHUNK_SIZE (1*1024*1024)

namespace os
{
	void abort() { ::abort(); }
	void log(const char *format, ...) { va_list ap; va_start(ap, format); fflush(stdout); vfprintf(stderr, format, ap); va_end(ap); }
};

void myread(void *data, size_t size, FILE *fp, const char *where)
{
	size_t written = 0;
	size_t r;
	int err;
	do
	{
		r = fread((char*)data + written, 1, size, fp);
		size -= r;
		written += r;
		err = ferror(fp);
	} while (size > 0 && err != EWOULDBLOCK && err != EINTR);
	if (err)
	{
		printf("Failed to read %s: %s\n", where, strerror(err));
		abort();
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
	if (argc != 2)
	{
		printf("Usage: %s <in-file>\n", argv[0]);
		exit(1);
	}
	FILE *in = fopen(argv[1], "rb");

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

	printf("JSON length %d\n", (int)jsonLength);

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
			printf("Done reading first round\n");
			break;
		}
		buffer_compressed.resize(compressed_length);
		myread(buffer_compressed.data(), compressed_length, in, "reading chunk pass 1");
		if (snappy::GetUncompressedLength(buffer_compressed.data(), buffer_compressed.size(), &size) == false)
		{
			printf("Error checking chunk size (pass 1)\n");
			abort();
		}
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
			printf("Done reading second round\n");
			break;
		}
		buffer_compressed.resize(compressed_length);
		myread(buffer_compressed.data(), compressed_length, in, "reading chunk pass 2");
		if (snappy::GetUncompressedLength(buffer_compressed.data(), buffer_compressed.size(), &size) == false)
		{
			printf("Error checking chunk size (pass 2)\n");
			abort();
		}
		if (snappy::RawUncompress(buffer_compressed.data(), buffer_compressed.size(), &big_buffer.data()[big_counter]) == false)
		{
			printf("Error decompressing chunk (pass 2)\n");
			abort();
		}
		big_counter += size;
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
	std::map<int, int> counts; // old idx : count of usage
	for (const auto pair : stored_sigbook)
	{
		counts[pair.first] = 0;
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
	while ((size_t)(src - big_buffer.data()) < uncompressed_length)
	{
		common::BCall *call = (common::BCall*)src;
		assert(call->funcId > 0);
		assert(call->funcId <= mMaxSigId);
		unsigned internalId = map_old_to_internal[call->funcId];
		uint32_t call_size = 0;
		if (internalId == 0 && counts[call->funcId] == 0)
		{
			printf("Function %d -> %s not found in current version, we'll assume vlen type and continue! if it is not we'll crash soon after this...\n", call->funcId, stored_sigbook[call->funcId].c_str());
		}
		else
		{
			// now comes the fun part... for some reason we decided to make the design of our file format a lot more complicated and less future proof to save 4 bytes each call...
			call_size = common::ApiInfo::IdToLenArr[internalId];
		}
		if (call_size == 0) // vlen type
		{
			common::BCall_vlen *vcall = (common::BCall_vlen*)src;
			call_size = vcall->toNext;
		}
		src += call_size;
		counts[call->funcId]++;
		if (map_old_to_new.count(call->funcId) == 0) // we haven't seen this call before
		{
			int newid = map_old_to_new.size() + 4; // we just skip the first 4 for some unknown reason
			map_old_to_new[call->funcId] = newid;
			map_new_to_old[newid] = call->funcId;
		}
	}


	for (const auto pair : stored_sigbook)
	{
		const int newid = map_old_to_new.count(pair.first);
		printf("sigbook (%u -> %u) => (%s -> %s), used %d times\n", pair.first, map_old_to_internal[pair.first], pair.second.c_str(), newid > 0 ? common::ApiInfo::IdToNameArr[newid] : "-", counts[pair.first]);
	}

	return 0;
}
