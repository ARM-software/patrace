#include <GLES2/gl2.h>

#include "memory_test.hpp"
#include "common/memory.hpp"
#include "md5/md5.h"

using namespace common;

MemoryTest::MemoryTest()
{
}

void MemoryTest::setUp()
{
}

void MemoryTest::tearDown()
{
}

void MemoryTest::testMD5()
{
    const unsigned char orig_data[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    };
    const unsigned char ele_data[] = {
        0x02, 0x03, 0x06, 0x07, 0x0A, 0x0B,
    };

    MD5Digest digest1, digest2;
    md5_state_t mdContext;
    md5_init(&mdContext);
    md5_append(&mdContext, ele_data, 6);
    md5_finish(&mdContext, digest1);

    md5_init(&mdContext);
    md5_append(&mdContext, orig_data+2, 2);
    md5_append(&mdContext, orig_data+6, 2);
    md5_append(&mdContext, orig_data+10, 2);
    md5_finish(&mdContext, digest2);

    // should get the same result with a whole memory
    // and with a strided memory
    CPPUNIT_ASSERT(memcmp(digest1, digest2, 16) == 0);

    ClientSideBufferObject mb(ele_data, sizeof(ele_data));
    CPPUNIT_ASSERT(mb.base_address == ele_data);
    CPPUNIT_ASSERT(mb.size == sizeof(ele_data));
    MD5Digest digest = mb.md5_digest();
    CPPUNIT_ASSERT(memcmp(digest, digest2, 16) == 0);
}

void MemoryTest::testMemoryBase()
{
    const unsigned char orig_data[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    };

    // The new-created object has a zero size and NULL address
    ClientSideBufferObject mb;
    CPPUNIT_ASSERT(mb.base_address == NULL);
    CPPUNIT_ASSERT(mb.size == 0);
    CPPUNIT_ASSERT(mb.md5_digest() == MD5Digest());

    // Initialized with address and length, without copy
    mb = ClientSideBufferObject(PTR_MOVE(orig_data, 0x04), 0x10, false);
    CPPUNIT_ASSERT(mb.base_address == PTR_MOVE(orig_data, 0x04));
    CPPUNIT_ASSERT(mb.size == 0x10);

    // overlap
    CPPUNIT_ASSERT(mb.overlap(PTR_MOVE(orig_data, 0x14), 4) == false);
    CPPUNIT_ASSERT(mb.overlap(PTR_MOVE(orig_data, 0x00), 4) == false);
    CPPUNIT_ASSERT(mb.overlap(PTR_MOVE(orig_data, 0x01), 4) == true);

    // Subset of the later region
    mb = ClientSideBufferObject(PTR_MOVE(orig_data, 0x04), 0x10, false);
    CPPUNIT_ASSERT(mb.extend(PTR_MOVE(orig_data, 0x01), 0x20) == PTR_MOVE(orig_data, 0x01));
    CPPUNIT_ASSERT(mb.base_address == PTR_MOVE(orig_data, 0x01));
    CPPUNIT_ASSERT(mb.size == 0x20);

    // Overlap with the later region
    mb = ClientSideBufferObject(PTR_MOVE(orig_data, 0x04), 0x10, false);
    CPPUNIT_ASSERT(mb.extend(PTR_MOVE(orig_data, 0x10), 0x10) == PTR_MOVE(orig_data, 0x04));
    CPPUNIT_ASSERT(mb.base_address == PTR_MOVE(orig_data, 0x04));
    CPPUNIT_ASSERT(mb.size == 0x1C);

    const unsigned char BUFFER1[8] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
    const unsigned char BUFFER2[8] = {0x00, 0x01, 0x03, 0x02, 0x04, 0x05, 0x06, 0x07};
    const unsigned char BUFFER3[2] = {0x03, 0x02};
    // Initialized with address and length, with copy
    mb = ClientSideBufferObject(BUFFER1, 8, true);
    CPPUNIT_ASSERT(mb.base_address != BUFFER1);
    CPPUNIT_ASSERT(mb.size == 8);
    CPPUNIT_ASSERT(memcmp(mb.base_address, BUFFER1, 8) == 0);
    CPPUNIT_ASSERT(mb.md5_digest() == MD5Digest(BUFFER1, 8));

    // Set sub-data
    mb.set_subdata(BUFFER3, 2, 2);
    CPPUNIT_ASSERT(mb.size == 8);
    CPPUNIT_ASSERT(memcmp(mb.base_address, BUFFER2, 8) == 0);
    CPPUNIT_ASSERT(mb.md5_digest() == MD5Digest(BUFFER2, 8));
}

void MemoryTest::testDataInitialization()
{
    // Test the initialization
    VertexAttributeMemoryMerger mbc;
    CPPUNIT_ASSERT(mbc.memory_range_count() == 0);
    CPPUNIT_ASSERT(mbc.attribute_count() == 0);

    // position attribute (3 float) and uv attribute (2 int)
    // float float float int int
    // float float float int int
    // float float float int int
    unsigned char BUFFER[60];
    const size_t stride = sizeof(float) * 3 + sizeof(int) * 2;
    mbc.add_attribute(0, PTR_MOVE(BUFFER, sizeof(float) * 3), (sizeof(int) * 6 + sizeof(float) * 6), 2, GL_INT, false, stride); // uv
    mbc.add_attribute(1, BUFFER, (sizeof(int) * 4 + sizeof(float) * 9), 3, GL_FLOAT, true, stride); // position

    CPPUNIT_ASSERT(mbc.memory_range_count() == 1);
    const ClientSideBufferObject *mb0 = mbc.memory_range(0);
    CPPUNIT_ASSERT(mb0->base_address == BUFFER);
    CPPUNIT_ASSERT(mb0->size == (sizeof(float) * 9 + sizeof(int) * 6));

    CPPUNIT_ASSERT(mbc.attribute_count() == 2);
    const VertexAttributeMemoryMerger::AttributeInfo *ai0 = mbc.attribute(0);
    CPPUNIT_ASSERT(ai0->index == 0);
    CPPUNIT_ASSERT(ai0->base_address == BUFFER);
    CPPUNIT_ASSERT(ai0->offset == sizeof(float) * 3);
    CPPUNIT_ASSERT(ai0->size == 2);
    CPPUNIT_ASSERT(ai0->type == GL_INT);
    CPPUNIT_ASSERT(ai0->normalized == false);
    CPPUNIT_ASSERT(ai0->stride == stride);
    const VertexAttributeMemoryMerger::AttributeInfo *ai1 = mbc.attribute(1);
    CPPUNIT_ASSERT(ai1->index == 1);
    CPPUNIT_ASSERT(ai1->base_address == BUFFER);
    CPPUNIT_ASSERT(ai1->offset == 0);
    CPPUNIT_ASSERT(ai1->size == 3);
    CPPUNIT_ASSERT(ai1->type == GL_FLOAT);
    CPPUNIT_ASSERT(ai1->normalized == true);
    CPPUNIT_ASSERT(ai1->stride == stride);
}

void MemoryTest::testClientSideBufferObjectSet()
{
    // test with a const object, only const methods should be called
    const ClientSideBufferObjectSet cmbs;
    ClientSideBufferObjectName name = 0;

    // a new set should have nothing inside
    CPPUNIT_ASSERT(cmbs.total_size(0) == 0);
    CPPUNIT_ASSERT(cmbs.total_size(1) == 0);
    CPPUNIT_ASSERT(cmbs.get_object(0, 0) == NULL);
    CPPUNIT_ASSERT(cmbs.find(0, ClientSideBufferObject(), name) == false);
    CPPUNIT_ASSERT(cmbs.translate_address(0, 0, 0x10) == NULL);

    ClientSideBufferObjectSet mbs;

    // The first available name should be zero
    ClientSideBufferObjectName bufferName = mbs.create_object(0);
    CPPUNIT_ASSERT(bufferName == 0);
    // The new-created object has a zero size and NULL address
    const ClientSideBufferObject * obj = mbs.get_object(0, 0);
    CPPUNIT_ASSERT(obj->base_address == NULL);
    CPPUNIT_ASSERT(obj->size == 0);
    bufferName = mbs.create_object(0);
    CPPUNIT_ASSERT(bufferName == 1);

    // fetch with invalid name
    obj = mbs.get_object(0, 3);
    CPPUNIT_ASSERT(obj == NULL);

    // seperated namespace for different threads
    bufferName = mbs.create_object(1);
    CPPUNIT_ASSERT(bufferName == 0);

    // reuse deleted name
    mbs.delete_object(0, 0);
    bufferName = mbs.create_object(0);
    CPPUNIT_ASSERT(bufferName == 0);

    // try to find a non-existing object
    name = 99;
    const char buffer0[8] = {0, 0, 1, 1, 2, 2, 3, 3};
    CPPUNIT_ASSERT(mbs.find(0, ClientSideBufferObject(buffer0, 8), name) == false);
    CPPUNIT_ASSERT(name == 99);

    // reserve the space for an object
    mbs.object_data(0, 0, 8, NULL, true);
    CPPUNIT_ASSERT(mbs.find(0, ClientSideBufferObject(buffer0, 8), name) == false);
    CPPUNIT_ASSERT(name == 99);

    // set data for the created object
    mbs.object_subdata(0, 0, 0, 8, buffer0);
    CPPUNIT_ASSERT(mbs.find(0, ClientSideBufferObject(buffer0, 8), name) == true);
    CPPUNIT_ASSERT(name == 0);

    // check the total size
    CPPUNIT_ASSERT(mbs.total_size(0) == 8);
    CPPUNIT_ASSERT(mbs.total_size(1) == 0);

    // try to find a deleted object
    mbs.delete_object(0, 0);
    CPPUNIT_ASSERT(mbs.find(0, ClientSideBufferObject(buffer0, 8), name) == false);
    CPPUNIT_ASSERT(name == 0);

    // check the total size
    CPPUNIT_ASSERT(mbs.total_size(0) == 0);
    CPPUNIT_ASSERT(mbs.total_size(1) == 0);

    unsigned char BUFFER0[16] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0xC1,
    0x00, 0x00, 0x80, 0xBF, 0xFF, 0xFF, 0xFF, 0x08};
    unsigned char BUFFER1[16] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0xC1,
    0x00, 0x00, 0x80, 0xBF, 0xFF, 0xFF, 0xFF, 0x0D};

    // Modify the content of a memory range between two find query, should not use the same object name
    CPPUNIT_ASSERT(mbs.find(0, ClientSideBufferObject(BUFFER0, 16), name) == false);
    name = mbs.create_object(0);
    CPPUNIT_ASSERT(name == 0);
    mbs.object_data(0, 0, 16, BUFFER0, false);

    memcpy(BUFFER0, BUFFER1, 16);
    CPPUNIT_ASSERT(mbs.find(0, ClientSideBufferObject(BUFFER0, 16), name) == false);
}
