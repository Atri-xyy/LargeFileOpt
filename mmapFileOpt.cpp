#include "mmapFileOpt.h"
using namespace wuxin;
using namespace largeFile;
int MmapFileOperation::mmapFile(const mmapOption& mmap_option){
     if (mmap_option.max_mmap_size_ < mmap_option.first_mmap_size_)
      {
        return TFS_ERROR;
      }

      if (mmap_option.max_mmap_size_ == 0)
      {
        return TFS_ERROR;
      }

      int fd = checkFile();
      if(fd < 0){
           if(deBug)fprintf(stderr,"MmapFileOperation::checkFile failed.\n");
           return TFS_ERROR;
      }

      if(!isMmaped_){
          if(mmapFile_){
              delete mmapFile_;
          }
          mmapFile_ = new MmapFile(mmap_option, fd);
          isMmaped_ = mmapFile_->mmapRun(true);//设置映射
      }
      if(isMmaped_){
          return TFS_SUCCESS;
      }
    return TFS_ERROR;
}


int MmapFileOperation::munmapFile(){
    if(isMmaped_ && mmapFile_){
        delete mmapFile_;
        isMmaped_ = false;
    }
    return TFS_SUCCESS;
}

void* MmapFileOperation::getMapData()const{
    if(isMmaped_){
        return mmapFile_->getData();
    }
    return nullptr;
}

int MmapFileOperation::preadFile(char* buf, const int32_t size, const int64_t offset){
    if(isMmaped_ && (offset + size) >= mmapFile_->getSize()){//如果偏移量加size >= 映射位置 扩容
        mmapFile_->mmapRst();
    }

    if(isMmaped_ && (offset + size) < mmapFile_->getSize()){
        memcpy(buf, (char *) mmapFile_->getData() + offset, size);//内存对拷
        return TFS_SUCCESS;
    }
    return FileOperation::preadFile(buf, size, offset);//如果没有进行映射 则调用父类从磁盘读
}



int MmapFileOperation::pwriteFile(const char* buf, const int32_t size, const int64_t offset){
    if(isMmaped_ && (offset + size) >= mmapFile_->getSize()){
        mmapFile_->mmapRst();
    }

    if(isMmaped_ && (offset + size) < mmapFile_->getSize()){
        memcpy((char *) mmapFile_->getData() + offset, buf,size);//buf写到磁盘
        cout<<"MmapFileOperation::pwriteFile"<<endl;
        return TFS_SUCCESS;
    }
    return FileOperation::pwriteFile(buf, size, offset);
}

int MmapFileOperation::flushFile(){
     if (isMmaped_)
      {
        if (mmapFile_->syncFile())
        {
          return TFS_SUCCESS;
        }
        else
        {
          return TFS_ERROR;
        }
      }
      return FileOperation::flushFile();
}