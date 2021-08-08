#include "fileOpt.h"
using namespace wuxin;
using namespace largeFile;

FileOperation::FileOperation(const string &fileName,const int openFlags):
fd_(-1),openFlags_(openFlags)
{
    fileName_ = strdup(fileName.c_str());//分配内存并且赋值
}


FileOperation::~FileOperation(){
    if(fd_ > 0){
        ::close(fd_);
    }
    if(fileName_){
        free(fileName_);
        fileName_ = nullptr;
    }
}

int FileOperation::openFile(){
    if(fd_ > 0){
        close(fd_);
        fd_ = -1;
    }

    fd_ = ::open(fileName_,openFlags_,OPEN_MODE);
    if(fd_ < 0){
        return -errno;
    }

    return fd_;
}

void FileOperation::closeFile(){
    if(fd_ < 0){
        return;
    }
    ::close(fd_);
    fd_ = -1;
}

int64_t FileOperation::getFileSize(){
    int fd = checkFile();
    if(fd < 0){
        return -1;
    }
    struct stat s;
    if(fstat(fd,&s) != 0){
        return -1;
    }
    return s.st_size;
}

int FileOperation::checkFile(){
    if(fd_ < 0){
        openFile();
    }
    return fd_;
}

bool FileOperation::ftruncateFile(const int64_t length){
    int fd = checkFile();
    if(fd < 0){
        return false;
    }
    return ftruncate(fd,length) == 0;
}

int FileOperation::seekFile(const int64_t offset){
    int fd = checkFile();
    if(fd < 0){
        return fd;
    }
    return lseek(fd,offset,SEEK_SET);//设置偏移量
}


int FileOperation::flushFile(){
    if(openFlags_ & O_SYNC){//如果是同步的则不需要flush
        return 0;
    }
    int fd = checkFile();
    if(fd < 0){
        return fd;
    }
    return fsync(fd);
}

int FileOperation::unlinkFile(){
    closeFile();
    return ::unlink(fileName_);
}


int FileOperation::writeFile(const char *buf ,const int32_t nbytes){
      int32_t left = nbytes;
      int32_t writeLen = 0;
      const char* p = buf;
      int i = 0;
      while (left > 0)
      {
        ++i;
        if (i >= MAX_DISK_TIMES)
        {
          break;
        }

        if (checkFile() < 0)
          return -errno;
        if ((writeLen = ::write(fd_, p, left)) < 0)
        {
          writeLen = -errno;
          if (EINTR == -writeLen || EAGAIN == -writeLen)//系统繁忙
          {
            continue;
          }
          if (EBADF == -writeLen)
          {
            fd_ = -1;
            continue;
          }
          else
          {
            return writeLen;
          }
        }
        else if (0 == writeLen)
        {
          break;
        }
        left -= writeLen;
        p += writeLen;
      }

      if (left != 0)
      {
        return EXIT_DISK_OPER_INCOMPLETE;
      }
      return TFS_SUCCESS;
}

int FileOperation::pwriteFile(const char *buf,const int32_t nbytes,const int64_t offset){
      int32_t left = nbytes;
      int64_t writeOffset = offset;
      int32_t writeLen = 0;
      const char* p = buf;
      int i = 0;
      while (left > 0)
      {
        ++i;
        if (i >= MAX_DISK_TIMES)
        {
          break;
        }

        if (checkFile() < 0)
          return -errno;
        if ((writeLen = ::pwrite64(fd_, p, left, writeOffset)) < 0)
        {
          writeLen = -errno;
          if (EINTR == -writeLen || EAGAIN == -writeLen)
          {
            continue;
          }
          if (EBADF == -writeLen)
          {
            fd_ = -1;
            continue;
          }
          else
          {
            return writeLen;
          }
        }
        else if (0 == writeLen)
        {
          break;
        }

        left -= writeLen;
        p += writeLen;
        writeOffset += writeLen;
      }

      if (left != 0)
      {
        return EXIT_DISK_OPER_INCOMPLETE;
      }
      return TFS_SUCCESS;
}

int FileOperation::preadFile(char *buf,const int32_t nbytes,const int64_t offset){
      int32_t left = nbytes;
      int64_t readOffset = offset;
      int32_t readLen = 0;
      const char* p = buf;
      int i = 0;
      while (left > 0)
      {
        ++i;
        if (i >= MAX_DISK_TIMES)
        {
          break;
        }
        if (checkFile() < 0)
          return -errno;
        if ((readLen = ::pread64(fd_, (char *)p, left, readOffset)) < 0)
        {
          readLen = -errno;
          if (EINTR == -readLen || EAGAIN == -readLen)
          {
            continue; 
          }
          else if (EBADF == -readLen)
          {
            fd_ = -1;
            continue;
          }
          else
          {
            return readLen;
          }
        }
        else if (0 == readLen)
        {
          break;
        }

        left -= readLen;
        p += readLen;
        readOffset += readLen;
      }

      if (left != 0)
      {
        return EXIT_DISK_OPER_INCOMPLETE;//读完数据但是没达到指定要读的数据
      }
     return TFS_SUCCESS;
}

