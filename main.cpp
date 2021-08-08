#include "fileOpt.h"
#include "mmapFileOpt.h"
#include "mmapFile.h"
#include "indexHanle.h"
#include "glob.h"
using namespace wuxin;
using namespace largeFile;
using namespace std;

static const mode_t OPEN_MODE = 0644;
const static largeFile::mmapOption mmapopt_ = {1024 *10000,4096,4096};//4k为一页
const static uint32_t mainBlockSize = 1024*1024*64;//k mb 64mb 一个块64
const static uint32_t bucketSize =  1000;//桶大小

int openFile(string fileName,int openFlags){
    int fd = ::open(fileName.c_str(),openFlags,OPEN_MODE);
    if(fd < 0){
        return -errno; //返回负数 目的是为了< 0
    }
    return fd;
}

int main(int argc, char **argv){
   cout<<"input blockId:";
   int blockId;
   cin >> blockId;
   int fileId;
   cin >> fileId;
   int ret = TFS_SUCCESS;
   assert(blockId > 0);
   string blockPath;

   stringstream tss;
   tss<<"."<<MAIN_BLOCK_DIR<<blockId;
   tss>>blockPath;
   largeFile::IndexHandle *indexMainHandle = new largeFile::IndexHandle(".",blockId);
   ret = indexMainHandle->load(blockId,bucketSize,mmapopt_);//加载测试
   MetaInfo meta;
   ret = indexMainHandle->readMeteInfo(fileId,meta);
   largeFile::FileOperation *mmapMainBlock = new largeFile::FileOperation(blockPath,O_RDWR);
   char *buf = new char[meta.getSize()+1];
   ret = mmapMainBlock->preadFile(buf,meta.getSize(),meta.getOffset());
   buf[meta.getSize()] = '\0';
   printf("%s\n",buf);
//    largeFile::FileOperation *mmapMainBlock = new largeFile::FileOperation(blockPath,O_CREAT | O_LARGEFILE |O_RDWR);
   
//    ret = mmapMainBlock->ftruncateFile(mainBlockSize);
//    if(!ret){
//        printf("ftruncateFile error.:%s\n",strerror(errno));
//        delete mmapMainBlock;
//        indexMainHandle->remove(blockId);
//    }

//     char buf[4096];
//     memset(buf,'2',sizeof(buf));
    
//     uint32_t fileNo  = indexMainHandle->blockInfo()->seqNo_;//获取编号
//     int32_t dataOffset = indexMainHandle->getDataOffset();//得到偏移量
//     mmapMainBlock->pwriteFile(buf,sizeof(buf),dataOffset);//根据偏移量写到内存
//     /*设置metainfo信息*/
//     largeFile::MetaInfo metainfo;
//     metainfo.setFileId(fileNo);
//     metainfo.setOffset(dataOffset);
//     metainfo.setSize(sizeof(buf));

//     ret = indexMainHandle->writeMeteInfo(metainfo.getKey(),metainfo);
//     if(ret == TFS_SUCCESS){
//         indexMainHandle->commitBlockDataOffset(sizeof(buf)); //更新索引头部信息
//         indexMainHandle->updataBlockInfo(INSERT,sizeof(buf));//更新块信息
//         ret = indexMainHandle->flush();//刷新到磁盘
//         if(ret != TFS_SUCCESS){
//             fprintf(stderr,"indexMainHandle->flush error, %s.\n",strerror(errno));
//         }
//     }else{
//         fprintf(stderr,"indexMainHandle->writeMeteInfo error, %s.\n",strerror(errno));
//     }
//     mmapMainBlock->closeFile();
    delete indexMainHandle;
    delete mmapMainBlock;
    delete buf;
    /*delete mmapMainBlock;*/
    return 0;
}

 /*const char *filename = "test.txt";
    largeFile::MmapFileOperation *mmo = new largeFile::MmapFileOperation(filename);
    int fd = mmo->openFile();
    int ret = mmo->mmapFile(mmapopt_);
    if(ret == TFS_ERROR){
        cout<<"errno"<<endl;
        mmo->closeFile();
        exit(-1);
    }
    char buf[129];
    buf[128] = '\0';
    memset(buf,'8',128);
    printf("write:%s\n",buf);
    ret = mmo->pwriteFile(buf,128,4096);
    if(ret > 0){
        cout<<"pwriteFile"<<endl;
        printf("write:%s\n",buf);
    }
    memset(buf,0,128);
    ret = mmo->preadFile(buf,128,4096);
    if(ret > 0){
        cout<<"preadFile"<<endl;
        printf("read:%s\n",buf);
    }
    
    mmo->flushFile();
    mmo->munmapFile();
    
    /*const char * filename = "optTest.txt";
    largeFile::fileOperation *test = new largeFile::fileOperation(filename,O_RDWR | O_CREAT | O_LARGEFILE);
    int fd = test->openFile();
    char buf[65];
    memset(buf,'6',64);
    int ret = test->pwriteFile(buf,64,1024);
    if(ret < 0){
        fprintf(stderr,"ret failed. %s.\n",strerror(errno));
    }

    ret = test->preadFile(buf,64,1024);
    memset(buf,'9',64);
    test->writeFile(buf,64);
    buf[65] = '\0';
    printf("%s",buf);
    test->closeFile();*/
    /*const char*filename = "./test.txt";
    int fd = openFile(filename,O_RDWR | O_CREAT | O_LARGEFILE);//可读写 没有就创建 大文件
    if(fd < 0){
        fprintf(stderr,"openFile failed.filename:%s.%s.\n",filename,strerror(-errno));
        return -1;
    }

    largeFile::mmapFile *mapFile = new largeFile::mmapFile(mmapopt_,fd);
    bool flag = mapFile->mmapRun(true);
    if(flag){
       mapFile->mmapRst();
       memset(mapFile->getData(),'8',mapFile->getSize());
       mapFile->syncFile();
       printf("%p . %d.", mapFile->getData(),mapFile->getSize());
       mapFile->mmapStop();
    }else{
        printf("main mmap failed, %s.\n",strerror(errno));
    }
    ::close(fd);*/