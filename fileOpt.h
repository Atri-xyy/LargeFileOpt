#ifndef _FileOperation_H_
#define _FileOperation_H_

#include "glob.h"
namespace wuxin{
    namespace largeFile{
        class FileOperation{
            public:
                FileOperation(const string &fileName,const int openFlags = O_RDWR | O_LARGEFILE);
                virtual ~FileOperation();
                
                int openFile();
                void closeFile();
                virtual int flushFile();//write到磁盘
                int unlinkFile();//删除文件

                int preadFile(char *buf,const int32_t nbytes,const int64_t offset);
                virtual int pwriteFile(const char *buf,const int32_t nbytes,const int64_t offset);//偏移写
                virtual int writeFile(const char *buf ,const int32_t nbytes);//当前位置写
                
                int64_t getFileSize();

                bool ftruncateFile(const int64_t length);
                int seekFile(const int64_t offset);

                int getFd()const {return fd_;}

            protected:
                int checkFile();

            protected:
                int fd_;
                int openFlags_;
                char *fileName_;
            protected:
                static const mode_t OPEN_MODE = 0644;//用户本身 同组的其它成员 其它组 
                static const int MAX_DISK_TIMES = 5;//最大磁盘读取次数
        };
           
    }
}



#endif