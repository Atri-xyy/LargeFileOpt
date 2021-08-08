#include "indexHanle.h"

using namespace wuxin;
using namespace largeFile;


/*
(IndexHanle)
--------------
blockinfo                                       }     
int32_t bucketSize_;   //hash桶大小
int32_t dataFileOffset;//未使用数据起始的偏移         IndexHeader
int32_t indexFileSize; //索引文件的当前偏移(大小)
int32_t freeHeadOffset;//可重复链表的节点头部     }
meteinfo
--------------

*/
IndexHandle::IndexHandle(const string&basePath,uint32_t mainBlockId){
    stringstream ss;
    ss << basePath<<INDEX_DIR<<mainBlockId;

    string indexPath;
    ss >> indexPath;

    mmapFileopt_ = new MmapFileOperation(indexPath,O_CREAT | O_LARGEFILE | O_RDWR);
    isLoad_ = false;
}


IndexHandle::~IndexHandle(){
    if(mmapFileopt_){
        delete mmapFileopt_;
        mmapFileopt_ = nullptr;
    }
}

 int IndexHandle::create(const uint32_t logicBlockId,const int32_t bucketSize,const mmapOption mapOpt){
     int ret;
     printf(
          "index create block: %u index. bucket size: %d, max mmap size: %d, first mmap size: %d, per mmap size: %d\n",
          logicBlockId, bucketSize, mapOpt.max_mmap_size_, mapOpt.first_mmap_size_,
          mapOpt.per_mmap_size_);
        if(isLoad_){
            return INDEX_ALREADY_LOADED_ERROR;
        }
        int64_t fileSize = mmapFileopt_->getFileSize();
        if(fileSize < 0){
            return TFS_ERROR;
        }else if(fileSize == 0){ //空文件
            IndexHeader iheader;
            iheader.blockInfo_.blockId_ = logicBlockId;
            iheader.blockInfo_.seqNo_ = 1;//从1开始
            iheader.bucketSize_ =bucketSize;
            iheader.indexFileSize = sizeof(IndexHeader) + bucketSize * sizeof(int32_t);//一个桶就占四个字节

            char *initData = new char[iheader.indexFileSize];
            memcpy(initData,&iheader,sizeof(IndexHeader));
            memset(initData + sizeof(IndexHeader), 0, iheader.indexFileSize - sizeof(IndexHeader));//当前偏移 + 一个sizeof(header) 的位置 = 当前metaInfo的位置

            ret = mmapFileopt_->pwriteFile(initData,iheader.indexFileSize,0);//先写到文件里面去 因为是空文件 所以是0偏移
            delete []initData;
            initData = nullptr;
            if(ret != TFS_SUCCESS){
               return ret;
            }
            ret = mmapFileopt_->flushFile();//立即刷新到磁盘
        }else {//文件已经存在
            return META_ALREADY_ERROR;
        }
        mmapFileopt_->mmapFile(mapOpt);
        isLoad_ = true;
        if(debug){
            printf("IndexHandle::create succeed.logicBlockId:%d.\n",logicBlockId);
        }
        return TFS_SUCCESS;
 }

 int IndexHandle::load(const uint32_t logicBlockId,const int32_t BucketSize,const mmapOption mapOpt){
        int ret;
        if(isLoad_){
            return INDEX_ALREADY_LOADED_ERROR;
        }
        int64_t fileSize = mmapFileopt_->getFileSize();
        if(fileSize <= 0){ // == 0 为空 调用load 应该是不为空的
            return INDEX_FILE_SIZE_ERROR;
        }

        mmapOption tmapOpt = mapOpt;//临时定义 保存下来

        if(fileSize > tmapOpt.first_mmap_size_ && fileSize < tmapOpt.max_mmap_size_){//大于第一次映射小于最大映射
            tmapOpt.first_mmap_size_ = fileSize;
        }

        ret = mmapFileopt_->mmapFile(tmapOpt);//映射到内存
        if(ret != TFS_SUCCESS){
            return ret;
        }

        if(0 == bucketSize() || 0 == blockInfo()->blockId_){
            return INDEX_FILE_SIZE_ERROR;
        }

        int32_t indexFileSize = sizeof(IndexHeader) + bucketSize() * sizeof(int32_t);
        if(fileSize < indexFileSize){
         fprintf(stderr, "Index error. blockid: %u, bucket size: %d, file size: %ld, index file size: %ld\n",
         blockInfo()->blockId_, bucketSize(), fileSize, indexFileSize);
            return INDEX_FILE_SIZE_ERROR;
        }

        if(logicBlockId != blockInfo()->blockId_){
            fprintf(stderr, "block id error. blockid: %u, index blockid: %u\n", logicBlockId, blockInfo()->blockId_);
            return BLOCKID_ERROR;
        }

        if(bucketSize() != BucketSize){
            return BUCKET_SIZE_ERROR;
        }

        return TFS_SUCCESS;

 }

 int IndexHandle::remove(const uint32_t logicBlockId){
     if(isLoad_){
         if(logicBlockId != blockInfo()->blockId_){
             fprintf(stderr,"IndexHandle::remove error.%s\n",strerror(errno));
             return BLOCKID_ERROR;
         }
     }
     mmapFileopt_->munmapFile();//解除映射
     mmapFileopt_->unlinkFile();//删除文件
     return TFS_SUCCESS;
 }

 int IndexHandle::flush(){
     int ret = mmapFileopt_->flushFile() ;
     if(ret!= TFS_SUCCESS){
         fprintf(stderr,"IndexHandle::flush error.%s\n",strerror(errno));
     }
     return ret;
 }



 int IndexHandle::hashFind(uint64_t key,int32_t &curOffset,int32_t &preOffset){
     //确定桶位置
     int32_t slot = static_cast<uint32_t> (key) % bucketSize();//就是获取哈希key
      preOffset = 0;
      MetaInfo metaInfo;
      int ret = TFS_SUCCESS;
      //
      for (int32_t pos = bucketSlot()[slot]; pos != 0;)
      {
        ret = mmapFileopt_->preadFile(reinterpret_cast<char*> (&metaInfo), sizeof(metaInfo), pos);//读到meteinfo 然后比较
        if (TFS_SUCCESS != ret)
          return ret;

        if (hashCompare(key, metaInfo.getKey()))
        {
          curOffset = pos;
          return TFS_SUCCESS;
        }
        preOffset = pos;//前一个节点 = 当前位置
        pos = metaInfo.getNextMetaOffset(); //当前位置 = 下一个节点位置
      }

      //如果没找到那就是没找到
      return META_INFO_NOT_FOUND;
 }


 int IndexHandle::writeMeteInfo(const uint64_t key,MetaInfo &metaInfo){
     int32_t curOffset = 0,preOffset = 0;
     int ret = hashFind(key,curOffset,preOffset);
     if(ret == TFS_SUCCESS){//key 存在 不能存在重复的key 
        return ret;
     }else if(ret != META_INFO_NOT_FOUND){ //其它错误
        return ret;
     }
      //未找到 插入
      int32_t slot =  key % bucketSize();
      return hashInsert(slot, preOffset, metaInfo);
 }

  int IndexHandle::readMeteInfo(const uint64_t key,MetaInfo &metaInfo){
     int32_t curOffset = 0,preOffset = 0;
     int ret = hashFind(key,curOffset,preOffset);
     if(ret == TFS_SUCCESS){//存在
        ret = mmapFileopt_->preadFile(reinterpret_cast<char*>(&metaInfo),sizeof(MetaInfo),curOffset);
        if(ret != TFS_SUCCESS){
            fprintf(stderr,"IndexHandle::readMeteInfo error.%s\n",strerror(errno));
        }
     }
     //不存在就直接返回 
     return ret;
 }


 int IndexHandle::hashInsert(uint64_t key,int32_t &preOffset,MetaInfo &metaInfo){
    int32_t slot = key % bucketSize();
    int ret = TFS_SUCCESS;
    MetaInfo tmeta;
    int32_t curOffset = 0;
    if(indexHeader()->freeHeadOffset != 0){ // 可重用偏移不为0表示有重用节点
        ret = mmapFileopt_->preadFile(reinterpret_cast<char*>(&tmeta),sizeof(MetaInfo),indexHeader()->freeHeadOffset);//因为要获取下一个节点 所以要读出来
        if(TFS_SUCCESS != ret){
            return ret;
        }
        curOffset = indexHeader()->freeHeadOffset;//当前偏移 = 可重用偏移(空余节点)
        indexHeader()->freeHeadOffset = tmeta.getNextMetaOffset();//可重用偏移 = 下一个节点偏移 如果没那就是0

    }else{//没有可重用节点
        curOffset = indexHeader()->indexFileSize;//当前可插入桶的首节点
        indexHeader()->indexFileSize += sizeof(MetaInfo);

    }
      /*插入到哈希表中*/
     metaInfo.setNextMetaOffset(0);//下一个节点的偏移量为0
      ret = mmapFileopt_->pwriteFile(reinterpret_cast<const char*> (&metaInfo), sizeof(MetaInfo), curOffset);//把调用的metaInfo先写进去
      if(TFS_SUCCESS != ret){
        return ret;
      }
     /*判断是否存在上一个节点*/
     if(preOffset != 0){
         /*先读出来*/
         ret = mmapFileopt_->preadFile(reinterpret_cast<char *>(&tmeta),sizeof(MetaInfo),preOffset);
         if(ret != TFS_SUCCESS){
             indexHeader()->indexFileSize -= sizeof(MetaInfo);//回滚
             return ret;
         }
         tmeta.setNextMetaOffset(curOffset);//前一个节点的下一个节点设置成当前偏移量  就是类似pre->next = p;
         /*写回去*/
         ret = mmapFileopt_->pwriteFile(reinterpret_cast<char *>(&tmeta),sizeof(MetaInfo),preOffset);
         if(ret != TFS_SUCCESS){
             indexHeader()->indexFileSize -= sizeof(MetaInfo);//回滚
             return ret;
         }
     }else{ //不存在前一个节点 则将偏移量设置成当前位置
         bucketSlot()[slot] = curOffset;
     }
     return TFS_SUCCESS;

 }


 int IndexHandle::updataBlockInfo(const OptType type,const int32_t modSize){//每次读写操作都要更新blockInfo信息
    if(blockInfo()->blockId_ == 0){
           return BLOCKID_ERROR;
        }
    if(type == INSERT){
        ++blockInfo()->verSion_;
        ++blockInfo()->fileCount_;
        ++blockInfo()->seqNo_;
        blockInfo()->size_ += modSize;
    }else if(type == DELETE){
        ++blockInfo()->delFileCount_;
        blockInfo()->delSize_ += modSize;
    }
    if(deBug)printf("update block info. blockid: %u, version: %u, file count: %u, size: %u, del file count: %u, del size: %u, seq no: %u, oper type: %d\n",
        blockInfo()->blockId_, blockInfo()->verSion_, blockInfo()->fileCount_, blockInfo()->size_,
       blockInfo()->delFileCount_, blockInfo()->delSize_, blockInfo()->seqNo_, type);
    return TFS_SUCCESS;
}



int IndexHandle::delMetaInfo(const uint64_t key){
    /*类似于链表 p->next = p->next->next
        于是p->next 就从链表中排除 不过不是直接删除内存和文件
        设置记录 实现可重用 头插法重新加入链表中
    */
    int32_t curOffset = 0, preOffset = 0;
    int32_t slot = key % bucketSize();//获取hashKey
    int ret = hashFind(key,curOffset,preOffset);
    if(ret != TFS_SUCCESS){
        return ret;
    }
    MetaInfo metaInfo;
    /*获取当前key的metainfo*/
    ret = mmapFileopt_->preadFile(reinterpret_cast<char*> (&metaInfo),sizeof(MetaInfo),curOffset);
    if(ret != TFS_SUCCESS){
        return ret;
    }
    /*获取当前节点下一个节点的位置*/
    int32_t tpos = metaInfo.getNextMetaOffset();
    if(preOffset == 0){//前面没有节点 表示为首节点
        bucketSlot()[slot] = tpos;//首节点直接为下一个节点的位置
    }else{//前面有节点
        MetaInfo preMetaInfo;
        ret = mmapFileopt_->preadFile(reinterpret_cast<char*> (&preMetaInfo),sizeof(MetaInfo),preOffset);//先读到preMetaInfo
        if(ret != TFS_SUCCESS){
        return ret;
        }
        preMetaInfo.setNextMetaOffset(tpos);//将前一个节点的下一个节点设置为当前节点的下个节点 p->next = p->next->next
        ret = mmapFileopt_->pwriteFile(reinterpret_cast<const char*> (&preMetaInfo), sizeof(MetaInfo), preOffset);//然后写回去
        if (TFS_SUCCESS != ret){
          return ret;
        }
    }
    /*将删除的节点实现可重用*/
    metaInfo.setNextMetaOffset(indexHeader()->freeHeadOffset);//下一个节点设置为可重用节点的偏移量
    ret = mmapFileopt_->pwriteFile(reinterpret_cast<const char*> (&metaInfo), sizeof(MetaInfo), curOffset);//写回去
    indexHeader()->freeHeadOffset = curOffset;//curOffset就是当前被删掉的节点
    return TFS_SUCCESS;
}