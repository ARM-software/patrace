#include <map>
#include <vector>
#include <algorithm>
#include <set>
#include <newfastforwarder/framestore.hpp>
#include <unordered_set>
#include "common/os_time.hpp"
using namespace std;

frameStoreState::frameStoreState()
 : endDraw(0)
 , endDrawClear(0)
 , endReadFramebuffer(0)
 , timeCount(0)
{}

preExecuteState::preExecuteState()
 : timeCount(0)
 , loopTime(0)
 , drawCallNo(0)
 , beginCallNo({0, 4294967294, 0})
 , endCallNo({0, 4294967294, 0})
 , endPreExecute(0)
 , beginDrawCallNo(0)
 , endDrawCallNo(0)
{}

const unsigned int CUBE_MAP_INDEX = 500000000;
const unsigned int FIRST_10_BIT = 1000000000;

unordered_set<unsigned int> & frameStoreState::queryTextureNoList(unsigned int textureIdx)
{
    if(textureIdx >= CUBE_MAP_INDEX)//500000000+ for cube_map
    {
        cube_map_list.insert(textureIdx % CUBE_MAP_INDEX);
    }

    map<unsigned int, unordered_set<unsigned int>>::iterator it = textureNoList.find(textureIdx);

    if(it == textureNoList.end())
    {
        unordered_set<unsigned int> newTextureNo;
        textureNoList.insert(pair<unsigned int, unordered_set<unsigned int>>(textureIdx, newTextureNo));
        map<unsigned int, unordered_set<unsigned int>>::iterator it2 = textureNoList.find(textureIdx);
        return (it2->second);
    }
    else
    {
        return (it->second);
    }
}


unordered_set<unsigned int> & frameStoreState::queryTextureClearNoList(unsigned int textureIdx)
{
    if(textureIdx >= CUBE_MAP_INDEX)//500000000+ for cube_map
    {
        cube_map_list.insert(textureIdx % CUBE_MAP_INDEX);
    }

    map<unsigned int, unordered_set<unsigned int>>::iterator it = textureClearNoList.find(textureIdx);
    if(it == textureClearNoList.end())
    {
        unordered_set<unsigned int> newTextureClearNo;
        textureClearNoList.insert(pair<unsigned int, unordered_set<unsigned int>>(textureIdx, newTextureClearNo));
        map<unsigned int, unordered_set<unsigned int>>::iterator it2 = textureClearNoList.find(textureIdx);
        return (it2->second);
    }
    else
    {
        return (it->second);
    }
}

void frameStoreState::insertTextureListNo(vector<unsigned int> &textureIdx, unordered_set<unsigned int> &callNoList)
{
    sort(textureIdx.begin(), textureIdx.end());
    textureIdx.erase(unique(textureIdx.begin(), textureIdx.end()), textureIdx.end());

    long long start_time = os::getTime();
    for(unsigned int i=0; i<textureIdx.size(); i++)
    {
        map<unsigned int, unordered_set<unsigned int>>::iterator it = textureNoList.find(textureIdx[i]);
        if(it != textureNoList.end())
        {
                callNoList.insert(it->second.begin(), it->second.end());
        }
            map<unsigned int, unordered_set<unsigned int>>::iterator it2 = textureClearNoList.find(textureIdx[i]);
            if(it2 != textureClearNoList.end())
            {
                callNoList.insert(it2->second.begin(), it2->second.end());
            }
            //  save for glFramebufferTexture2D GL_TEXTURE_CUBE_MAP
            set<unsigned int>::iterator it3 = cube_map_list.find(textureIdx[i]);
            if(it3 != cube_map_list.end())
        {
            for(unsigned int j=CUBE_MAP_INDEX; j<=CUBE_MAP_INDEX*5; j=j+CUBE_MAP_INDEX)
            {
                it = textureNoList.find(textureIdx[i]+j);
                if(it != textureNoList.end())
                {
                    callNoList.insert(it->second.begin(), it->second.end());
                }
                it2 = textureClearNoList.find(textureIdx[i]+j);
                if(it2 != textureClearNoList.end())
                {
                    callNoList.insert(it2->second.begin(), it2->second.end());
                }
            }//j cycle
        }//if cube map
        //  save for glFramebufferTexture2D GL_TEXTURE_CUBE_MAP
    }//for
    long long end_time = os::getTime();
    float duration = static_cast<float>(end_time - start_time) / os::timeFrequency;
    timeCount = timeCount +duration;
}

void frameStoreState::clearTextureNoList(unsigned int textureIdx)
{
    map<unsigned int, unordered_set<unsigned int>>::iterator it = textureNoList.find(textureIdx);
    if(it != textureNoList.end())
    {
        it->second.clear();
    }
    map<unsigned int, unordered_set<unsigned int>>::iterator it2 = textureClearNoList.find(textureIdx);
    if(it2 != textureClearNoList.end())
    {
        it2->second.clear();
    }
}

void frameStoreState::setEndDraw(bool index)
{
    endDraw = index;
}

void frameStoreState::setEndDrawClear(bool index)
{
    endDrawClear = index;
}

void frameStoreState::setEndReadFrameBuffer(bool index)
{
    endReadFramebuffer = index;
}

bool frameStoreState::readEndReadFrameBuffer()
{
    return endReadFramebuffer;
}


bool frameStoreState::readEndDraw()
{
    return endDraw;
}

bool frameStoreState::readEndDrawClear()
{
    return endDrawClear;
}

void frameStoreState::debug()
{
}

//preExecute

unordered_set<unsigned int> & preExecuteState::queryTextureNoList(unsigned int textureIdx)
{
    if(textureIdx >= CUBE_MAP_INDEX)
    {
        cube_map_list.insert(textureIdx % CUBE_MAP_INDEX);
    }
    map<unsigned int, unordered_set<unsigned int>>::iterator it = textureNoList.find(textureIdx);
    if(it == textureNoList.end())
    {
        unordered_set<unsigned int> newTextureNo;
        textureNoList.insert(pair<unsigned int, unordered_set<unsigned int>>(textureIdx, newTextureNo));
        map<unsigned int, unordered_set<unsigned int>>::iterator it2 = textureNoList.find(textureIdx);
        return (it2->second);
    }
    else
    {
        return (it->second);
    }
}

unordered_set<unsigned long long> & preExecuteState::newQueryTextureNoList(unsigned int textureIdx)
{
    if(textureIdx >= CUBE_MAP_INDEX)
    {
        cube_map_list.insert(textureIdx % CUBE_MAP_INDEX);
    }
    map<unsigned int, unordered_set<unsigned long long>>::iterator it = newTextureNoList.find(textureIdx);
    if(it == newTextureNoList.end())
    {
        unordered_set<unsigned long long> newTextureNo;
        newTextureNoList.insert(pair<unsigned int, unordered_set<unsigned long long>>(textureIdx, newTextureNo));
        map<unsigned int, unordered_set<unsigned long long>>::iterator it2 = newTextureNoList.find(textureIdx);
        return (it2->second);
    }
    else
    {
        return (it->second);
    }
}

void preExecuteState::debug()
{
    std::map<unsigned int, std::unordered_set<unsigned long long>>::iterator it = newTextureNoList.find(116);
    if(it != newTextureNoList.end())
    {
        printf("!!!!!!!!!!! %ld\n", (long)it->second.size());
    }
}

void preExecuteState::clearTextureNoList(unsigned int textureIdx)
{
    map<unsigned int, unordered_set<unsigned int>>::iterator it = textureNoList.find(textureIdx);
    if(it != textureNoList.end())
    {
        it->second.clear();
    }
}

void preExecuteState::insertTextureListNo(vector<unsigned int> &textureIdx, unordered_set<unsigned int> &callNoList)
{
    sort(textureIdx.begin(), textureIdx.end());
    textureIdx.erase(unique(textureIdx.begin(), textureIdx.end()), textureIdx.end());
    long long start_time = os::getTime();

    for(unsigned int i=0; i<textureIdx.size(); i++)
    {
        loopTime++;
        map<unsigned int, unordered_set<unsigned int>>::iterator it = textureNoList.find(textureIdx[i]);
        if(it != textureNoList.end())
        {
            callNoList.insert(it->second.begin(), it->second.end());
        }
        //  save for glFramebufferTexture2D GL_TEXTURE_CUBE_MAP
        set<unsigned int>::iterator it3 = cube_map_list.find(textureIdx[i]);
        if(it3!=cube_map_list.end())
        {
            for(unsigned int j=CUBE_MAP_INDEX; j<=CUBE_MAP_INDEX*5; j=j+CUBE_MAP_INDEX)
            {
                it = textureNoList.find(textureIdx[i]+j);
                if(it != textureNoList.end())
                {
                    unordered_set<unsigned int>::iterator potr = it->second.begin();
                    while(potr != it->second.end())
                    {
                        callNoList.insert(*potr);
                        potr++;
                    }
                }
            }//j cycle
        }//if cube map
        //  save for glFramebufferTexture2D GL_TEXTURE_CUBE_MAP
    }//for
    long long end_time = os::getTime();
    float duration = static_cast<float>(end_time - start_time) / os::timeFrequency;
    timeCount = timeCount +duration;
}

void preExecuteState::newInsertTextureListNo(vector<unsigned int> &textureIdx, unordered_set<unsigned long long> &callNoList)
{
    sort(textureIdx.begin(), textureIdx.end());
    textureIdx.erase(unique(textureIdx.begin(), textureIdx.end()), textureIdx.end());
    vector<unsigned int>::iterator dele = find(textureIdx.begin(), textureIdx.end(), 202);

    if(dele != textureIdx.end())
    {
        textureIdx.erase(dele);
    }

    for(unsigned int i=0; i<textureIdx.size(); i++)
    {
        map<unsigned int, unordered_set<unsigned long long>>::iterator it = newTextureNoList.find(textureIdx[i]);
        if(it != newTextureNoList.end())
        {
            callNoList.insert(it->second.begin(), it->second.end());
        }
        //  save for glFramebufferTexture2D GL_TEXTURE_CUBE_MAP
        set<unsigned int>::iterator it3 = cube_map_list.find(textureIdx[i]);
        if(it3 != cube_map_list.end())
        {
            for(unsigned int j=CUBE_MAP_INDEX; j<=CUBE_MAP_INDEX*5; j=j+CUBE_MAP_INDEX)
            {
                it = newTextureNoList.find(textureIdx[i]+j);
                if(it != newTextureNoList.end())
                {
                    unordered_set<unsigned long long>::iterator potr = it->second.begin();
                    while(potr != it->second.end())
                    {
                        callNoList.insert(*potr);
                        potr++;
                    }
                }
            }//j cycle
        }//if cube map
        //  save for glFramebufferTexture2D GL_TEXTURE_CUBE_MAP
    }//for
}

void preExecuteState::finalTextureNoListDraw()
{
    map<unsigned int, unordered_set<unsigned int>>::iterator it = textureNoList.find(0);
    finalTextureNoList.insert(it->second.begin(), it->second.end());
}

void preExecuteState::newFinalTextureNoListDraw()
{
    map<unsigned int, unordered_set<unsigned long long>>::iterator it = newTextureNoList.find(0);
    newFinalTextureNoList.insert(it->second.begin(), it->second.end());
    //for gfx 50
    map<unsigned int, unordered_set<unsigned long long>>::iterator it2 = newTextureNoList.find(202);
    if(it2!=newTextureNoList.end())
    {
        newFinalTextureNoList.insert(it2->second.begin(), it2->second.end());
    }
}

void preExecuteState::newInsertCallIntoList(bool clear, unsigned int drawTextureIdx)
{
    if(clear == false)
    {
        if(drawTextureIdx == beginCallNo.drawTextureIdx && drawCallNo == endCallNo.drawCallNo + 1)
        {
        }
        else
        {
            unsigned long long beginEnd = 0;//first 10 bit begin, last 9 bit end
            beginEnd = beginCallNo.thisCallNo;
            beginEnd = beginEnd * FIRST_10_BIT;//save in first 10 bit
            beginEnd = beginEnd + endCallNo.thisCallNo;
            map<unsigned int, unordered_set<unsigned long long>>::iterator it = newTextureNoList.find(beginCallNo.drawTextureIdx);
            if(it == newTextureNoList.end())
            {
                unordered_set<unsigned long long> newList;
                newList.insert(beginEnd);
                newTextureNoList.insert(pair<unsigned int, unordered_set<unsigned long long>>(beginCallNo.drawTextureIdx, newList));
            }
            else
            {
                it->second.insert(beginEnd);
            }
        }
    }//if clear == false
    else
    {
        if(drawTextureIdx == beginCallNo.drawTextureIdx)
        {
            map<unsigned int, unordered_set<unsigned long long>>::iterator it = newTextureNoList.find(drawTextureIdx);
            if(it != newTextureNoList.end())it->second.clear();
        }
        else
        {
            unsigned long long beginEnd = 0;//first 10 bit begin, last 9 bit end
            beginEnd = beginCallNo.thisCallNo;
            beginEnd = beginEnd * FIRST_10_BIT;//save in first 10 bit
            beginEnd = beginEnd + endCallNo.thisCallNo;
            map<unsigned int, unordered_set<unsigned long long>>::iterator it2 = newTextureNoList.find(beginCallNo.drawTextureIdx);
            if(it2 == newTextureNoList.end())
            {
                unordered_set<unsigned long long> newList;
                newList.insert(beginEnd);
                newTextureNoList.insert(pair<unsigned int, unordered_set<unsigned long long>>(beginCallNo.drawTextureIdx, newList));
            }
            else
            {
                it2->second.insert(beginEnd);
            }
            it2 = newTextureNoList.find(drawTextureIdx);
            if(it2 != newTextureNoList.end())
            {
                it2->second.clear();
            }
        }
    }
}

void preExecuteState::newInsertOneCallNo(bool clear, unsigned int drawTextureIdx, unsigned int callNo)
{
    if(clear == false)
    {
        if(drawTextureIdx == beginCallNo.drawTextureIdx && drawCallNo == endCallNo.drawCallNo + 1)
        {
            endCallNo.drawCallNo++;
            endCallNo.thisCallNo = callNo;
        }
        else
        {
            beginCallNo.thisCallNo = callNo;
            beginCallNo.drawTextureIdx = drawTextureIdx;
            beginCallNo.drawCallNo = drawCallNo;
            endCallNo = beginCallNo;
        }
    }//if clear == false
    else
    {
        if(drawTextureIdx == beginCallNo.drawTextureIdx)
        {
            map<unsigned int, unordered_set<unsigned long long>>::iterator it = newTextureNoList.find(drawTextureIdx);
            if(it != newTextureNoList.end())
            {
                it->second.clear();
            }
            beginCallNo.thisCallNo = callNo;
            beginCallNo.drawTextureIdx = drawTextureIdx;
            beginCallNo.drawCallNo = drawCallNo;
            endCallNo = beginCallNo;
        }
        else
        {
            map<unsigned int, unordered_set<unsigned long long>>::iterator it2 = newTextureNoList.find(drawTextureIdx);
            if(it2 != newTextureNoList.end())
            {
                it2->second.clear();
            }
            beginCallNo.thisCallNo = callNo;
            beginCallNo.drawTextureIdx = drawTextureIdx;
            beginCallNo.drawCallNo = drawCallNo;
            endCallNo = beginCallNo;
        }
    }
}

void preExecuteState::test()
{
    set<unsigned long long>::iterator it2 = newFinalTextureNoList.begin();
    /*    if(*it2 == 0 )it2++;
    while(it != finalTextureNoList.end())
    {
        if(*it>(*it2)%1000000000||*it<(*it2)/1000000000)
        {
            printf("error!!!!!!!!!\n");
        }
        printf("drawNo : %d    newMethod  %llu\n", *it, *it2);
        it++;
        if((*it)>(*it2)%1000000000)
        {
            it2++;
        }
    }*/

    while(it2 != newFinalTextureNoList.end())
    {
        printf("draw num  %lld\n", *it2);
        it2++;
    }
    printf("number : %ld    newNumber:  %ld\n", (long)finalTextureNoList.size(), (long)newFinalTextureNoList.size());
}
