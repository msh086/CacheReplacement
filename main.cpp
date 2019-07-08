#include <iostream>
#include <cstring>
#include <stdlib.h>
#include "CacheSim.h"
#include "argparse.hpp"
#include  <fstream>
#include <vector>
#include<string>
//t添加的头文件《fstream》
using namespace std;
vector<string> split(const string& str, const string& delim) {
    vector<string> res;
    if("" == str) return res;
    //先将要切割的字符串从string类型转换为char*类型
    char * strs = new char[str.length() + 1] ; //不要忘了
    strcpy(strs, str.c_str());

    char * d = new char[delim.length() + 1];
    strcpy(d, delim.c_str());

    char *p = strtok(strs, d);
    while(p) {
        string s = p; //分割得到的字符串转换为string类型
        res.push_back(s); //存入结果数组
        p = strtok(NULL, d);
    }

    return res;
}

int main(const int argc, const char *argv[]) {
//    char test_case[100] = "";
//    // 如果没有输入文件，默认是gcc.trace
//    if(argc > 1){
//        strcat(test_case, argv[1]);
//    }else{
//        strcat(test_case, "gcc.trace");
//    }
////    int line_size[] = {8, 16, 32, 64, 128};
//    int line_size[] = {32};
//    int ways[] = {1, 2, 4, 8, 12, 16, 32};
////    int ways[] = {8};
////    int cache_size[] = {0x800,0x1000,0x2000,0x4000,0x8000,0x10000,0x20000,0x40000,0x80000};
//    int cache_size[] = {0x8000,0x10000,0x20000};
//    int i,j,m;`
//    CacheSim cache;
//    for (m = 0;m<sizeof(cache_size)/sizeof(int);m++){
//        for (i=0; i<sizeof(line_size)/sizeof(int); i++){
//            for (j=0; j<sizeof(ways)/sizeof(int); j++){
////                for (int k = CACHE_SWAP_FIFO; k < CACHE_SWAP_MAX; ++k) {
//                    printf("\ncache_size: %d Bytes\tline_size: %d\t mapping ways %d \t swap_style %d \n", cache_size[m],line_size[i], ways[j], k);
//                    cache.init(cache_size[m], line_size[i], ways[j]);
////                    cache.set_swap_style(k);
//                    cache.load_trace(test_case);
//                    delete cache;
////                }
//            }
//        }
//    }
    //以下为固定的测试

    //修改代码
    string line;
    fstream f("parameter.txt");
//    FILE *fwrite;
//    fwrite=fopen("result.txt", "w");
//    fclose(fwrite);//清空结果文件，如果没有新创建

    while (getline(f, line))
    {
        vector<string> inVector=split(line, " ");
        size_t num = inVector.size();
        char ** parseChar=new char*[num+1];
        for (int ii =0;ii<num;ii++){
            const char* a=inVector[ii].c_str();
            parseChar[ii]= const_cast<char*>(a);
        }
//        int indexnull=line.find(' ');
//        string title=line.substr(indexnull);
//        title=title+"\n";//要写入文件的输入参数

//        FILE *fout;
//        fout = fopen("result.txt", "a");
//        fputs(title.c_str(),fout);//写入到文件末尾
//        fclose(fout);
        //end
        ArgumentParser parser;
        // 输入trace的地址
        parser.addArgument("-i", "--input", 1, false);
        parser.addArgument("-p", "--page_unit", 1, false); //页的单位:每页被分成的份数
        parser.addArgument("-n", "--sample_num", 1, false ); //取样的页数: 1/page * [1000 1500 2000]
        parser.addArgument("-t", "--sample_time", 1, false); //取样间隔时间 ms
        parser.addArgument("--l1", 1, true);
        parser.addArgument("--l2", 1, true);
        parser.addArgument("--line_size", 1, true);
        parser.addArgument("--ways", 1, true);
        parser.parse(num, (const char**)parseChar);//修改
//        delete(parseChar);
        printf("*****%s\n*****", parser.retrieve<string>("input").c_str());


        //    _u64 line_size[] = {128};
        _u64 line_size[] = {64};

        _u64 ways[] = {8};
        //    _u64 ways[] = {1};
        //    _u64 cache_size[] = {0x100000, 0x200000, 0x300000, 0x400000};
        _u64 cache_size[] = {0x400000};
        int ms[] = {3};
        int i, j, m, k;
        CacheSim cache;

        /*加载取样有关的参数*/
        //    const char *s = parser.retrieve<string>("sample_time").c_str();
        //    _u64 sample_time;
        //    sample_time = strtoull(s, NULL, 10);
        //    printf("***sample_time:  ***%d\n", atoi(parser.retrieve<string>("sample_time").c_str()));

        cache.load_sample_config(atoi(parser.retrieve<string>("sample_num").c_str()), atoi(parser.retrieve<string>("page_unit").c_str()), atoi(parser.retrieve<string>("sample_time").c_str()));
        printf("sample_num:%d\n", atoi(parser.retrieve<string>("sample_num").c_str()));
        //    cache.load_sample_config(atoi(parser.retrieve<string>("sample_num").c_str()), atoi(parser.retrieve<string>("page_unit").c_str()));


        for (m = 0; m < sizeof(cache_size) / sizeof(_u64); m++) {
            for (i = 0; i < sizeof(line_size) / sizeof(_u64); i++) {
                for (j = 0; j < sizeof(ways) / sizeof(_u64); j++) {
                    for (k = 0; k < sizeof(ms) / sizeof(int); k++) {
                        //                for (int k = CACHE_SWAP_FIFO; k < CACHE_SWAP_MAX; ++k) {
                        printf("\n=================\ncache_size: %lld Bytes\tline_size: %lld\t mapping ways %lld \t \n",
                               cache_size[m],
                               line_size[i],
                               ways[j]);
                        _u64 temp_cache_size[2], temp_line_size[2], temp_ways[2];
                        temp_cache_size[0] = 0x1000;
                        temp_cache_size[1] = cache_size[m];
                        temp_line_size[0] = 128;
                        temp_line_size[1] = line_size[i];
                        temp_ways[0] = 8;
                        temp_ways[1] = ways[j];
                        cache.init(temp_cache_size, temp_line_size, temp_ways);
                        cache.set_M(ms[k]);
                        //                    cache.set_swap_style(k);

                        cache.load_trace(parser.retrieve<string>("input").c_str());

                        cache.re_init();
                        //                }
                    }
                }
            }
        }
    }
    f.close();

////    int line_size[] = {8, 16, 32, 64, 128};
//    _u64 line_size[] = {64,64 };
//    _u64 ways[] = {4,4};
//    _u64 cache_size[3] = {0x1000,0x400000};
//    CacheSim cache;
//    cache.init(cache_size, line_size,ways);
//    cache.load_trace(parser.retrieve<string>("input").c_str());
    return 0;
}