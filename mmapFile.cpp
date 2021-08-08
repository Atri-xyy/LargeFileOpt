#include "mmapFile.h"

using namespace wuxin;
using namespace largeFile;

MmapFile::MmapFile():
size_(0),fd_(-1),data_(nullptr)
{

}

MmapFile::MmapFile(const int fd):
size_(0),fd_(fd),data_(nullptr) 
{

}

MmapFile::MmapFile(const mmapOption&mmapopt,const int fd):
size_(0),fd_(fd),data_(nullptr)
{
    mmapopt_.max_mmap_size_ = mmapopt.max_mmap_size_;
    mmapopt_.first_mmap_size_ = mmapopt.first_mmap_size_;
    mmapopt_.per_mmap_size_ = mmapopt.per_mmap_size_;
}

MmapFile::~MmapFile(){
    if(data_){
        if(deBug)printf("mmap file delete. fd: %d. size: %d. data: %p.\n",fd_,size_,data_);
        msync(data_,size_,MS_SYNC);//同步映射
        munmap(data_,size_);//解除映射
        size_ = 0;
        data_ = nullptr;
        mmapopt_.first_mmap_size_ = 0;
        mmapopt_.max_mmap_size_ = 0;
        mmapopt_.per_mmap_size_ = 0;
    }
    
}

bool MmapFile::syncFile(){
    if(data_ && size_ > 0){
        return msync(data_,size_,MS_ASYNC) == 0;
    }
    return true;//没有东西需要同步
}

bool MmapFile::mmapRun(bool write){
    int flags = PROT_READ;
    if(write){
        flags |= PROT_WRITE;
    }

    if(fd_ < 0){
        return false;
    }

    if(mmapopt_.max_mmap_size_ == 0){
        return false;
    }
    
    if(mmapopt_.max_mmap_size_ > size_){
        size_ = mmapopt_.first_mmap_size_;
    }else{
        size_ = mmapopt_.max_mmap_size_;
    }

    if(!mmapResize(size_)){
        fprintf(stderr,"mmapResize error. %s.\n",strerror(errno));
        return false;
    }

    data_ = mmap(0,size_,flags,MAP_SHARED,fd_,0);

    if(data_ == MAP_FAILED){//失败
        fprintf(stderr,"mmap failed. %s.",strerror(errno));
        size_ = 0;
        fd_ = -1;
        data_ = nullptr;
        return false;
    }

    if(deBug){
       printf("mmap succeed. fd: %d. size: %d. data: %p.\n",fd_,size_,data_);
    }
    return true;
}

void* MmapFile::getData()const{
    return data_;
}

int32_t MmapFile::getSize()const {
    return size_;
}

bool MmapFile::mmapStop(){
    if(munmap(data_,size_) == 0){
        return true;
    }
    return false;
}

bool MmapFile::mmapResize(const int32_t size){
    struct stat s;
    if(fstat(fd_,&s) < 0){
        fprintf(stderr,"fstat error. %s.\n",strerror(errno));
        return false;
    }

    if(s.st_size < size){
        if(ftruncate(fd_,size) < 0){
            fprintf(stderr,"ftruncate error. %s.\n",strerror(errno));
            return false;
        }
    }
    return true;
}

bool MmapFile::mmapRst(){
    if(fd_ < 0 || data_ == nullptr){
        fprintf(stderr,"mmapRst error. %s.\n",strerror(errno));
        return false;
    }

    if(size_ == mmapopt_.max_mmap_size_){
        fprintf(stderr,"mmapRst error.size:%d.  %s.\n",size_,strerror(errno));
        return false;
    }

    int32_t newSize_ = size_ + mmapopt_.per_mmap_size_;
    if(newSize_ > mmapopt_.max_mmap_size_){
        newSize_ = mmapopt_.max_mmap_size_;
    }

    if(!mmapResize(newSize_)){
        fprintf(stderr,"mmapRst.mmapResize error. %s.\n",strerror(errno));
        return false;
    }

    void* newData_ = mremap(data_,size_,newSize_,MREMAP_MAYMOVE);//可以找到合适的地址映射
    if(newData_ == MAP_FAILED){
       fprintf(stderr,"mremap error. %s.\n",strerror(errno)); 
    }


    if(deBug){
         printf("mmapRst succeed. fd: %d. size: %d. data: %p.\n",fd_,newSize_,newData_);
    }
    data_ = newData_;
    size_ = newSize_;
    
    return true;
}