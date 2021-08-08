#ifndef _GLOB_H_
#define _GLOB_H_
#include <unistd.h>
#include <stdlib.h>
#include <iostream>
#include <fcntl.h>
#include <string>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sstream>
#include <cassert>
static int deBug = 1;
using namespace std;
namespace wuxin{
    namespace largeFile{
        
        const int32_t TFS_SUCCESS = 1;
        const int32_t TFS_ERROR = -1;

        const int32_t EXIT_DISK_OPER_INCOMPLETE = -9527; //读写少于指定数目
        const int32_t META_ALREADY_ERROR = -9525;//meta已经存在

        const int32_t INDEX_ALREADY_LOADED_ERROR = -9526;//索引已被加载
        const int32_t INDEX_FILE_SIZE_ERROR = -9524;//索引文件有问题

        const int32_t BLOCKID_ERROR = -9523;//块id出错
        const int32_t BUCKET_SIZE_ERROR = -9522;//桶size出错

        const int32_t META_INFO_NOT_FOUND = -9521;//未找到mateinfo
    

        static const string MAIN_BLOCK_DIR = "/mainblock/";//块储存目录
        static const string INDEX_DIR = "/index/"; //索引信息储存目录


        enum OptType{
            INSERT = 1,
            DELETE
        };


        struct mmapOption
        {
            int32_t max_mmap_size_; //最大映射
            int32_t first_mmap_size_;//第一次映射
            int32_t per_mmap_size_;//追加映射
        };

        struct BlockInfo //块信息
        {
            public:
                BlockInfo()
                    {
                        memset(this, 0, sizeof(BlockInfo));
                    }

                uint32_t blockId_;
                int32_t verSion_;
                int32_t fileCount_;
                int32_t size_;
                int32_t delFileCount_;
                int32_t delSize_;
                uint32_t seqNo_;
            private:
                
                inline bool operator==(const BlockInfo& rhs) const
                {
                    return blockId_ == rhs.blockId_ && verSion_ == rhs.verSion_ && fileCount_ == rhs.fileCount_ && size_
                    == rhs.size_ && delFileCount_ == rhs.delFileCount_ && delSize_ == rhs.delSize_ && seqNo_ == rhs.seqNo_;
                }
        };

        struct MetaInfo //索引信息
        {
            public:
                MetaInfo(){
                    init();
                }

                MetaInfo(const uint64_t fileId,const int32_t inOffset,const int32_t fileSize,const int32_t nextMetaOffset){
                    fileId_ = fileId;
                    location_.inneOffset_ = inOffset;
                    location_.size_ = fileSize;
                    nextMetaOffset_ = nextMetaOffset;
                }

                MetaInfo(const MetaInfo &metaInfo){//拷贝构造
                    memcpy(this,&metaInfo,sizeof(MetaInfo));//直接拷贝内存
                }

                
                uint64_t getKey()const{
                    return fileId_;
                }

                void setKey(const uint64_t key){
                    fileId_ = key;
                }

                void setFileId(const uint64_t fileId){
                    fileId_ = fileId;
                }

                void setOffset(const int32_t Offset){
                    location_.inneOffset_ = Offset;
                }

                int32_t getOffset(){
                    return location_.inneOffset_;
                }

                int32_t getSize(){
                    return location_.size_;
                }

                void setSize(const int32_t fileSize){
                    location_.size_ = fileSize;
                }

                int32_t getNextMetaOffset()const {
                    return nextMetaOffset_;
                }

                void setNextMetaOffset(const int32_t nextMetaOffset){
                    nextMetaOffset_  = nextMetaOffset;
                }

                MetaInfo&operator=(const MetaInfo &metaInfo){
                    if(&metaInfo == this){
                        return *this;
                    }
                    fileId_ = metaInfo.fileId_;
                    location_.inneOffset_ = metaInfo.location_.inneOffset_;
                    location_.size_ = metaInfo.location_.size_;
                    nextMetaOffset_ = metaInfo.nextMetaOffset_;
                }

                bool operator == (const MetaInfo &rhs)const{
                    return fileId_ == rhs.fileId_ &&  location_.size_ == rhs.location_.size_ 
                    && location_.inneOffset_ == rhs.location_.inneOffset_ && nextMetaOffset_ ==
                    rhs.nextMetaOffset_;
                }

                /*
                    clone();
                */
            private:
                void init()
                {
                    fileId_ = 0;
                    location_.inneOffset_ = 0;
                    location_.size_ = 0;
                    nextMetaOffset_ = 0;
                }
                uint64_t fileId_;//文件编号
                struct{
                    int32_t inneOffset_;
                    int32_t size_;
                }location_;
                int32_t nextMetaOffset_;//下一个索引块位置
        };
    }
}


#endif