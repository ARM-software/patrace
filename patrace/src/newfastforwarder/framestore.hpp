#ifndef _FRAMESTORE_HPP_
#define _FRAMESTORE_HPP_

#include <map>
#include <vector>
#include <set>
#include <unordered_set>

class frameStoreState
{
public:
    frameStoreState();
    std::unordered_set<unsigned int> & queryTextureNoList(unsigned int textureIdx);
    std::unordered_set<unsigned int> & queryTextureClearNoList(unsigned int textureIdx);
    void insertTextureListNo(std::vector<unsigned int> &textureIdx, std::unordered_set<unsigned int> &callNoList);
    void clearTextureNoList(unsigned int index);
    void setEndDraw(bool index);
    bool readEndDraw();
    void setEndDrawClear(bool index);
    bool readEndDrawClear();
    void debug();
    void setEndReadFrameBuffer(bool index);
    bool readEndReadFrameBuffer();
    std::map<unsigned int, std::unordered_set<unsigned int>> textureNoList;//
    std::map<unsigned int, std::unordered_set<unsigned int>> textureClearNoList;//save glClear for every texture
    std::set<unsigned int> cube_map_list;
    bool endDraw;
    bool endDrawClear;
    bool endReadFramebuffer;
    float timeCount;
};

class preExecuteState
{
public:
    preExecuteState();
    std::unordered_set<unsigned int> & queryTextureNoList(unsigned int textureIdx);
    std::unordered_set<unsigned long long> & newQueryTextureNoList(unsigned int textureIdx);
    void insertTextureListNo(std::vector<unsigned int> &textureIdx, std::unordered_set<unsigned int> &callNoList);
    void clearTextureNoList(unsigned int index);
    void finalTextureNoListDraw();
    void newFinalTextureNoListDraw();
    void newInsertTextureListNo(std::vector<unsigned int> &textureIdx, std::unordered_set<unsigned long long> &callNoList);
    void newInsertOneCallNo(bool clear, unsigned int drawTextureIdx, unsigned int callNo);//0 means no texture
    void newInsertCallIntoList(bool clear, unsigned int drawTextureIdx);
    void test();
    void debug();
    std::map<unsigned int, std::unordered_set<unsigned int>> textureNoList;//0 means no texture
    std::set<unsigned int> finalTextureNoList;
    std::set<unsigned long long> newFinalTextureNoList;
    std::set<unsigned int>::iterator finalPotr;
    std::set<unsigned long long>::iterator newFinalPotr;
    std::set<unsigned int> cube_map_list;
    float timeCount;
    unsigned int loopTime;
    unsigned int drawCallNo;
    struct preExecutePointerState
    {
        unsigned int drawTextureIdx;
        unsigned int drawCallNo;
        unsigned int thisCallNo;
    };
    preExecutePointerState beginCallNo;
    preExecutePointerState endCallNo;
    bool endPreExecute;
    std::map<unsigned int, std::unordered_set<unsigned long long>> newTextureNoList;
    unsigned int beginDrawCallNo;
    unsigned int endDrawCallNo;
};
#endif// BEGIN
