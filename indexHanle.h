#ifndef _INDEXHANLE_H
#define _INDEXHANLE_H
#include "glob.h"
#include "mmapFile.h"
#include "mmapFileOpt.h"
static int debug = 1;
namespace wuxin
{
    namespace largeFile{
        struct IndexHeader{
            public:
                IndexHeader(){
                    memset(this, 0, sizeof(IndexHeader));
                }

                BlockInfo blockInfo_; //块信息

                int32_t bucketSize_;   //hash桶大小
                int32_t dataFileOffset;//未使用数据起始的偏移
                int32_t indexFileSize; //索引文件的当前偏移(大小)
                int32_t freeHeadOffset;//可重复链表的节点头部
        };

        class IndexHandle{
            public:
                IndexHandle(const string &basePath,const uint32_t mainBlockId);
                ~IndexHandle();


                int create(const uint32_t logicBlockId,const int32_t bucketSize,const mmapOption mapOpt);
                int load(const uint32_t logicBlockId,const int32_t bucketSize,const mmapOption mapOpt);
                int remove(const uint32_t logicBlockId);
                int flush();

                IndexHeader *indexHeader(){
                    return  reinterpret_cast<IndexHeader*>(mmapFileopt_->getMapData());//void* 转换为IndexHeader 映射后这样得到地址
                }

                BlockInfo *blockInfo(){
                    return reinterpret_cast<BlockInfo*>(mmapFileopt_->getMapData());
                }

                int32_t bucketSize()const {
                    return reinterpret_cast<IndexHeader*>(mmapFileopt_->getMapData())->bucketSize_;
                }

                int32_t getDataOffset()const{//未使用的偏移
                    return reinterpret_cast<IndexHeader*>(mmapFileopt_->getMapData())->dataFileOffset;
                }

                int32_t *bucketSlot(){
                    return reinterpret_cast<int32_t*>(reinterpret_cast<char*>(mmapFileopt_->getMapData()) + sizeof(IndexHeader));//返回一个桶的位置 metainfo
                }

                int hashFind(uint64_t key,int32_t &offset,int32_t &preOffset);
                int hashInsert(uint64_t key,int32_t &preOffset,MetaInfo &metaInfo);
                
                int writeMeteInfo(const uint64_t key,MetaInfo &metaInfo);//写metainfo到(内存)文件中
                int readMeteInfo(const uint64_t key,MetaInfo &metaInfo);//读
                int delMetaInfo(const uint64_t key);//删除

                void commitBlockDataOffset(const int32_t fileSize){
                    reinterpret_cast<IndexHeader*>(mmapFileopt_->getMapData())->dataFileOffset += fileSize;
                }

                int updataBlockInfo(const OptType type,const int32_t modSize);//修改块信息
            private:
                MmapFileOperation *mmapFileopt_;
                bool isLoad_;

                bool hashCompare(const uint32_t lKey,const uint32_t rKey){
                    return lKey == rKey;
                }
        };
    }
} // namespace wuxin

#endif