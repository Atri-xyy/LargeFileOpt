#ifndef _MMAPFILEOPT_H_
#define _MMAPFILEOPT_H_
#include "glob.h"
#include "mmapFile.h"
#include "fileOpt.h"

namespace wuxin {
    namespace largeFile{
        class MmapFileOperation:public FileOperation
        {
            public:
                MmapFileOperation(const string&filename,const int OPEN_FLAGS = O_RDWR | O_CREAT | O_LARGEFILE):
                FileOperation(filename,OPEN_FLAGS),mmapFile_(nullptr),isMmaped_(false)
                {

                }
                
                ~MmapFileOperation(){
                    if(mmapFile_){
                        delete mmapFile_;
                        mmapFile_ = nullptr;
                    }
                }

                int preadFile(char* buf, const int32_t size, const int64_t offset);
                int pwriteFile(const char* buf, const int32_t size, const int64_t offset);
                
                int munmapFile();
                int mmapFile(const mmapOption& mmap_option);

                void* getMapData() const;
                int flushFile();
                
            private:
                MmapFileOperation();
                MmapFile * mmapFile_;
                bool isMmaped_;

        };
    }
}

#endif