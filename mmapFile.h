#ifndef _LARGERFILE_MmapFile_H_
#define _LARGERFILE_MmapFile_H_
#include "glob.h"

using namespace std;
namespace wuxin{
    namespace largeFile{
        
        
        class MmapFile
        {
        public:
            MmapFile();
            ~MmapFile();
            explicit MmapFile(const int fd);//显式调用
            MmapFile(const mmapOption&mmapopt,const int fd);
            
           
            bool syncFile();//同步文件 因为物理内存与磁盘数据不同 需要同步
            
            bool mmapRun(bool write = false);//是否开始映射到内存
            bool mmapStop();//解除映射
            bool mmapRst();//重新映射
            
            void*getData()const;//取出数据
            int32_t getSize()const;//获取大小
        private:
            bool mmapResize(const int32_t size);//扩容 调整大小
        
        private:
            int fd_;
            int32_t size_;
            void*data_;
            mmapOption mmapopt_;
        };
     
    }
}





#endif