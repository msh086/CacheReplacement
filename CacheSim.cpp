//
// Created by find on 16-7-19.
// Cache architect
// memory address  format:
// |tag|组号 log2(组数)|组内块号log2(mapping_ways)|块内地址 log2(cache line)|
//
#include "CacheSim.h"
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cstdio>
#include <time.h>
#include <climits>
#include "map"
#include <fstream>
#include <vector>
#include <algorithm>

CacheSim::CacheSim() {}

/**@arg a_cache_size[] 多级cache的大小设置
 * @arg a_cache_line_size[] 多级cache的line size（block size）大小
 * @arg a_mapping_ways[] 组相连的链接方式*/

bool ShareMemoryPage::isFree() {
    return this->free;
}

_u32 CacheSim::pow_int(int base, int expontent) {
    _u32 sum = 1;
    for (int i = 0; i < expontent; i++) {
        sum *= base;
    }
    return sum;
}

_u64 CacheSim::pow_64(_u64 base, _u64 expontent) {
    _u64 sum = 1;
    for (int i = 0; i < expontent; i++) {
        sum *= base;
    }
    return sum;
}

void CacheSim::set_M(int m) {
    SRRIP_M = m;
    SRRIP_2_M_1 = pow_int(2, SRRIP_M) - 1;
    SRRIP_2_M_2 = pow_int(2, SRRIP_M) - 2;
}

void CacheSim::init(_u64 a_cache_size[3], _u64 a_cache_line_size[3], _u64 a_mapping_ways[3]) {
//如果输入配置不符合要求
    if (a_cache_line_size[0] < 0 || a_cache_line_size[1] < 0 || a_mapping_ways[0] < 1 || a_mapping_ways[1] < 1) {
        return;
    }
    cache_size[0] = a_cache_size[0];
    cache_size[1] = a_cache_size[1];

//    在这里屏蔽两级cache 仅仅作为一个cache与share mem
    cache_line_size[0] = a_cache_line_size[0];
    cache_line_size[1] = a_cache_line_size[1];



    // 总的line数 = cache总大小/ 每个line的大小（一般64byte，模拟的时候可配置）
    cache_line_num[0] = (_u64) a_cache_size[0] / a_cache_line_size[0];
    cache_line_num[1] = (_u64) a_cache_size[1] / a_cache_line_size[1];
    cache_line_shifts[0] = (_u64) log2(a_cache_line_size[0]);
    cache_line_shifts[1] = (_u64) log2(a_cache_line_size[1]);


    // 几路组相联
    cache_mapping_ways[0] = a_mapping_ways[0];
    cache_mapping_ways[1] = a_mapping_ways[1];
    // 总共有多少set
    cache_set_size[0] = cache_line_num[0] / cache_mapping_ways[0];
    cache_set_size[1] = cache_line_num[1] / cache_mapping_ways[1];
    // 其二进制占用位数，同其他shifts
    cache_set_shifts[0] = (_u64) log2(cache_set_size[0]);
    cache_set_shifts[1] = (_u64) log2(cache_set_size[1]);
    // 空闲块（line）
    cache_free_num[0] = cache_line_num[0];
    cache_free_num[1] = cache_line_num[1];

    cache_r_count = 0;
    cache_w_count = 0;
    cache_w_memory_count = 0;
    cache_r_memory_count = 0;
    cache_r_hit_count = 0;
    cache_w_hit_count = 0;
    cache_w_miss_count = 0;
    cache_r_miss_count = 0;
    SM_in = 0;
    // ticktock，主要用来在替换策略的时候提供比较的key，在命中或者miss的时候，相应line会更新自己的count为当时的tick_count;
    tick_count = 0;
//    cache_buf = (_u8 *) malloc(cache_size);
//    memset(cache_buf, 0, this->cache_size);
    // 为每一行分配空间
    for (int i = 0; i < 2; ++i) {
        caches[i] = (Cache_Line *) malloc(sizeof(Cache_Line) * cache_line_num[i]);
        memset(caches[i], 0, sizeof(Cache_Line) * cache_line_num[i]);
    }
    page_size = 12;
    num_of_share_memory_page = 512;
    free_SM_page = num_of_share_memory_page;
//    ShareMemory=(ShareMemoryPage*)malloc(sizeof(ShareMemoryPage)*num_of_share_memory_page);
//    memset(ShareMemory,0, sizeof(ShareMemoryPage)*num_of_share_memory_page);
    ShareMemory = new ShareMemoryPage[num_of_share_memory_page];
    SM_hit_count = 0;
    //测试时的默认配置
    swap_style[0] = CACHE_SWAP_SRRIP;
    swap_style[1] = CACHE_SWAP_LRU;
//    swap_style[1] = CACHE_SWAP_CAR;
    // 用于SRRIP算法

    PSEL = 0;
    cur_win_repalce_policy = CACHE_SWAP_SRRIP;
    lock_table = (_u64 *) malloc(sizeof(_u64) * 1024);

    write_through = 0;
    write_allocation = 0;
    /**延迟统计和设置*/
    overall_cycles = 0;
    mem_latency[0] = 100;
//    mem_latency[1] =



    re_init();
    srand((unsigned) time(NULL));
}

/**顶部的初始化放在最一开始，如果中途需要对tick_count进行清零和caches的清空，执行此。*/
void CacheSim::re_init() {
    tick_count = 0;
    target_out = 0;
    target_in = 0;
    target = 0x07264;
    memset(cache_hit_count, 0, sizeof(cache_hit_count));
    memset(cache_miss_count, 0, sizeof(cache_miss_count));
    cache_free_num[0] = cache_line_num[0];
    memset(caches[0], 0, sizeof(Cache_Line) * cache_line_num[0]);
    memset(caches[1], 0, sizeof(Cache_Line) * cache_line_num[1]);
//    memset(cache_buf, 0, this->cache_size);
}

CacheSim::~CacheSim() {
    free(caches[0]);
    free(caches[1]);
    free(ShareMemory);
    free(lock_table);
//    free(cache_buf);
}

int CacheSim::check_cache_hit(_u64 set_base, _u64 addr, int level) {
    /**循环查找当前set的所有way（line），通过tag匹配，查看当前地址是否在cache中*/
    _u64 i;
    for (i = 0; i < cache_mapping_ways[level]; ++i) {
        if ((caches[level][set_base + i].flag & CACHE_FLAG_VALID) &&
            (caches[level][set_base + i].tag == ((addr >> (cache_set_shifts[level] + cache_line_shifts[level]))))) {
            return set_base + i; //返回line在set内的偏移地址
        }
    }
    return -1;
}

/**获取当前set中可用的line，如果没有，就找到要被替换的块*/
int CacheSim::get_cache_free_line(_u64 set_base, int level) {
    _u64 i, min_count, j;
    int free_index;
    /**从当前cache set里找可用的空闲line，可用：脏数据，空闲数据
     * cache_free_num是统计的整个cache的可用块*/
    for (i = 0; i < cache_mapping_ways[level]; ++i) {
        if (!(caches[level][set_base + i].flag & CACHE_FLAG_VALID)) {
            if (cache_free_num[level] > 0)
                cache_free_num[level]--;
            return set_base + i;
        }
    }
    /**没有可用line，则执行替换算法
     * lock状态的块如何处理？？*/
    free_index = -1;
    switch (swap_style[level]) {
        case CACHE_SWAP_RAND:
            free_index = rand() % cache_mapping_ways[level];
            break;
        case CACHE_SWAP_LRU:
            min_count = ULONG_LONG_MAX;
            for (j = 0; j < cache_mapping_ways[level]; ++j) {
                if (caches[level][set_base + j].count < min_count &&
                    !(caches[level][set_base + j].flag & CACHE_FLAG_LOCK)) {
                    min_count = caches[level][set_base + j].count;
                    free_index = j;
                }
            }
            break;
        case CACHE_SWAP_SRRIP:
            while (free_index < 0) {
                for (_u64 k = 0; k < cache_mapping_ways[level]; ++k) {
                    if (caches[level][set_base + k].RRPV == SRRIP_2_M_1) {
                        free_index = k;
                        // break the for-loop
                        break;
                    }
                }
                // increment all RRPVs

                if (free_index < 0) {
                    // increment all RRPVs
                    for (_u64 k = 0; k < cache_mapping_ways[level]; ++k) {
                        caches[level][set_base + k].RRPV++;
                    }
                } else {
                    // break the while-loop
                    break;
                }
            }

//                for (_u64 k = 0; k < cache_mapping_ways[level]; ++k) {
//                    if (caches[level][set_base + k].RRPV == SRRIP_2_M_1) {
//                        free_index = k;
//                        // break the for-loop
//                        break;
//                    }
//                }
//                // increment all RRPVs
//
//                if (free_index < 0) {
//                    // increment all RRPVs
//                    for (_u64 k = 0; k < cache_mapping_ways[level]; ++k) {
//                        caches[level][set_base + k].RRPV++;
//                    }
//                }

            break;
    }
    //如果没有使用锁，那么这个if应该是不会进入的
    if (free_index < 0) {
        //如果全部被锁定了，应该会走到这里来。那么强制进行替换。强制替换的时候，需要setline?
        min_count = ULONG_LONG_MAX;
        for (j = 0; j < cache_mapping_ways[level]; ++j) {
            if (caches[level][set_base + j].count < min_count) {
                min_count = caches[level][set_base + j].count;
                free_index = j;
            }
        }
    }
    if (free_index >= 0) {
        free_index += set_base;
        //如果原有的cache line是脏数据，标记脏位
        if (caches[level][free_index].flag & CACHE_FLAG_DIRTY) {
            // TODO: 写回到L2 cache中。
            // TODO: 写延迟 ： mem， l2.
            caches[level][free_index].flag &= ~CACHE_FLAG_DIRTY;
            cache_w_memory_count++;
        }
    } else {
        printf("I should not show\n");
    }
    return free_index;
}


/**获取当前set中可用的line，如果没有，就找到要被替换的块*/
int CacheSim::get_cache_free_line_specific(_u64 set_base, int level, int a_swap_style) {
    _u64 i, min_count, j;
    int free_index;//要替换的
    /**从当前cache set里找可用的空闲line，可用：脏数据，空闲数据
     * cache_free_num是统计的整个cache的可用块*/
    for (i = 0; i < cache_mapping_ways[level]; ++i) {
        if (!(caches[level][set_base + i].flag & CACHE_FLAG_VALID)) {
            if (cache_free_num[level] > 0)
                cache_free_num[level]--;
            return set_base + i;
        }
    }
    /**没有可用line，则执行替换算法
     * lock状态的块如何处理？？*/
    free_index = -1;
    switch (a_swap_style) {
        case CACHE_SWAP_RAND:
            free_index = rand() % cache_mapping_ways[level];
            break;
        case CACHE_SWAP_LRU:
            min_count = ULONG_LONG_MAX;
            for (j = 0; j < cache_mapping_ways[level]; ++j) {
                if (caches[level][set_base + j].count < min_count &&
                    !(caches[level][set_base + j].flag & CACHE_FLAG_LOCK)) {
                    min_count = caches[level][set_base + j].count;
                    free_index = j;
                }
            }
            break;
        case CACHE_SWAP_SRRIP:
            while (free_index < 0) {
                for (_u64 k = 0; k < cache_mapping_ways[level]; ++k) {
                    if (caches[level][set_base + k].RRPV == SRRIP_2_M_1) {
                        free_index = k;
                        // break the for-loop
                        break;
                    }
                }
                // increment all RRPVs

                if (free_index < 0) {
                    // increment all RRPVs
                    for (_u64 k = 0; k < cache_mapping_ways[level]; ++k) {
                        caches[level][set_base + k].RRPV++;
                    }
                } else {
                    // break the while-loop
                    break;
                }
            }

//                for (_u64 k = 0; k < cache_mapping_ways[level]; ++k) {
//                    if (caches[level][set_base + k].RRPV == SRRIP_2_M_1) {
//                        free_index = k;
//                        // break the for-loop
//                        break;
//                    }
//                }
//                // increment all RRPVs
//
//                if (free_index < 0) {
//                    // increment all RRPVs
//                    for (_u64 k = 0; k < cache_mapping_ways[level]; ++k) {
//                        caches[level][set_base + k].RRPV++;
//                    }
//                }

            break;
    }
    //如果没有使用锁，那么这个if应该是不会进入的
    if (free_index < 0) {
        //如果全部被锁定了，应该会走到这里来。那么强制进行替换。强制替换的时候，需要setline?
        min_count = ULONG_LONG_MAX;
        for (j = 0; j < cache_mapping_ways[level]; ++j) {
            if (caches[level][set_base + j].count < min_count) {
                min_count = caches[level][set_base + j].count;
                free_index = j;
            }
        }
    }
    if (free_index >= 0) {
        free_index += set_base;
        //如果原有的cache line是脏数据，标记脏位
        if (caches[level][free_index].flag & CACHE_FLAG_DIRTY) {
            // TODO: 写回到L2 cache中。
            // TODO: 写延迟 ： mem， l2.
            caches[level][free_index].flag &= ~CACHE_FLAG_DIRTY;
            cache_w_memory_count++;
        }
    } else {
        printf("I should not show\n");
    }
    return free_index;
}

int CacheSim::check_sm_hit(_u64 addr, int level) {
    int i;
    for (i = 0; i < num_of_share_memory_page; ++i) {
        if (ShareMemory[i].tag == (addr >> page_size)) {
            ShareMemory[i].count = tick_count;
//            printf("Index %lld hit and addr is %lld\n",i,addr>>12);
            return 1;
        }
    }
    return 0;
}


int CacheSim::get_free_sm_page() {
    _u64 i, min_count, j;
    int free_index = -1;
    /**从当前SM set里找可用的空闲Page，可用：空闲数据
     * SM_free_num是统计的整个SM的可用Page*/
    if (free_SM_page > 0) {
        for (i = 0; i < num_of_share_memory_page; ++i) {
            if (ShareMemory[i].isFree()) {
                free_SM_page--;
                SM_in++;
                return i;
            }
        }
    }
    /**没有可用page，则执行替换算法
     **/
    // !!!BUG Fixed
    min_count = ULONG_LONG_MAX;
    for (j = 0; j < num_of_share_memory_page; ++j) {
        if (ShareMemory[j].count < min_count) {
            min_count = ShareMemory[j].count;
            free_index = j;
        }
    }
    if (free_index >= 0) {
        _u64 tag = ShareMemory[free_index].tag;
        if (tag == target) {
            target_out++;
        }
        SM_in++;
    }

    return free_index;
}


void CacheSim::set_sm_page(_u64 index, _u64 addr, int level) {
    ShareMemoryPage page = ShareMemory[index];
    // 这里每个line的buf和整个cache类的buf是重复的而且并没有填充内容。
//    line->buf = cache_buf + cache_line_size * index;
    // 更新这个line的tag位
    page.tag = addr >> page_size;
    if (page.tag == target) {
        target_in++;
    }
    page.count = tick_count;
    page.free = false;
    ShareMemory[index] = page;
}

/**返回这个set是否是sample set。*/
int CacheSim::get_set_flag(_u64 set_base) {
    // size >> 10 << 5 = size * 32 / 1024 ，参照论文中的sample比例
    int K = cache_set_size[1] >> 5;
    int log2K = (int) log2(K);
    int log2N = (int) log2(cache_set_size[1]);
    // 使用高位的几位，作为筛选.比如需要32 = 2^5个，则用最高的5位作为mask
    _u64 mask = pow_64(2, (_u64) (log2N - log2K)) - 1;
    _u64 residual = set_base & mask;
    return residual;
}

double CacheSim::get_miss_rate(int level) {
    return 100.0 * cache_miss_count[level] / (cache_miss_count[level] + cache_hit_count[level]);
}

double CacheSim::get_hit_rate(int level) {
    return 100.0 * cache_hit_count[level] / (cache_miss_count[level] + cache_hit_count[level]);
}

/**将数据写入cache line，只有在miss的时候才会执行*/
void CacheSim::set_cache_line(_u64 index, _u64 addr, int level) {
    Cache_Line *line = caches[level] + index;
    // 这里每个line的buf和整个cache类的buf是重复的而且并没有填充内容。
//    line->buf = cache_buf + cache_line_size * index;
    // 更新这个line的tag位
    line->tag = addr >> (cache_set_shifts[level] + cache_line_shifts[level]);
    line->flag = (_u8) ~CACHE_FLAG_MASK;
    line->flag |= CACHE_FLAG_VALID;
    line->count = tick_count;
}

/**不需要分level*/
void CacheSim::do_cache_op(_u64 addr, char oper_style) {
    _u64 set_l1, set_l2, set_base_l1, set_base_l2;
    long long hit_index_l1, hit_index_l2, free_index_l1, free_index_l2;
    tick_count++;
    if (oper_style == OPERATION_READ) cache_r_count++;
    if (oper_style == OPERATION_WRITE) cache_w_count++;
    set_l2 = (addr >> cache_line_shifts[1]) % cache_set_size[1]; //line所属的set号 0-8191
    set_base_l2 = set_l2 * cache_mapping_ways[1]; //cache内set的偏移地址 0 8 16 ...
    hit_index_l2 = check_cache_hit(set_base_l2, addr, 1);//set内line的偏移地址 0-7
    set_l1 = (addr >> cache_line_shifts[0]) % cache_set_size[0];
    set_base_l1 = set_l1 * cache_mapping_ways[0];
    hit_index_l1 = check_cache_hit(set_base_l1, addr, 0);
    int set_flag = get_set_flag(set_l2); //返回当前set是否为sample set，没太看懂这个方法
    int temp_swap_style = swap_style[1];
    if (swap_style[1] == CACHE_SWAP_DRRIP) {
        /**是否是sample set*/
        switch (set_flag) {
            case 0:
                temp_swap_style = CACHE_SWAP_BRRIP;
                break;
            case 1:
                temp_swap_style = CACHE_SWAP_SRRIP;
                break;
            default:
                if (PSEL > 0) {
                    cur_win_repalce_policy = CACHE_SWAP_BRRIP;
                } else {
                    cur_win_repalce_policy = CACHE_SWAP_SRRIP;
                }
                temp_swap_style = cur_win_repalce_policy;
        }
    }



    //是否写操作
    if (oper_style == OPERATION_WRITE) {
        if (hit_index_l2 >= 0) {
            cache_w_hit_count++;
            cache_hit_count[1]++;
            caches[1][hit_index_l2].count = tick_count;
            if (write_through) {
                cache_w_memory_count++;
            } else {
                caches[1][hit_index_l2].flag |= CACHE_FLAG_DIRTY;
            }
            switch (temp_swap_style) {
                case CACHE_SWAP_BRRIP:
                case CACHE_SWAP_SRRIP:
                    caches[1][hit_index_l2].RRPV = 0;
                    break;
                case CACHE_SWAP_SRRIP_FP:
                    if (caches[1][hit_index_l2].RRPV != 0) {
                        caches[1][hit_index_l2].RRPV -= 1;
                    }
                    break;
//                case CACHE_SWAP_DRRIP:
//                    break;
            }
        } else {
//            cache_r_miss_count++;
            cache_w_miss_count++;
            cache_miss_count[1]++;
            if (write_allocation) {
                free_index_l2 = get_cache_free_line_specific(set_base_l2, 1, temp_swap_style);
                set_cache_line((_u64) free_index_l2, addr, 1);
                cache_r_memory_count++;
                // 这里可能不对。
                if (write_through) {
                    cache_w_memory_count++;
                } else {
                    // 只是置脏位，不用立刻写回去，这样如果下次是write hit，就可以减少一次memory写操作
                    caches[1][free_index_l2].flag |= CACHE_FLAG_DIRTY;
                }
                switch (temp_swap_style) {
                    case CACHE_SWAP_SRRIP_FP:
                    case CACHE_SWAP_SRRIP:
                        caches[1][free_index_l2].RRPV = SRRIP_2_M_2;
                        break;
                    case CACHE_SWAP_BRRIP:
                        caches[1][free_index_l2].RRPV = rand() / RAND_MAX > EPSILON ? SRRIP_2_M_1 : SRRIP_2_M_2;
                        break;
                }
                // 如果是动态策略，则还需要更新psel
                if (swap_style[1] == CACHE_SWAP_DRRIP) {
                    if (set_flag == 1) {
                        PSEL++;
                    } else if (set_flag == 0) {
                        PSEL--;
                    }
                }
            } else {
                cache_w_memory_count++;
            }
        }
    } else {
//        cache命中则直接返回
        if (hit_index_l2 >= 0) {
            cache_r_hit_count++;
            cache_hit_count[1]++;
            caches[1][hit_index_l2].count = tick_count;
            switch (temp_swap_style) {
                case CACHE_SWAP_BRRIP:
                case CACHE_SWAP_SRRIP:
                    caches[1][hit_index_l2].RRPV = 0;
                    break;
                case CACHE_SWAP_SRRIP_FP:
                    if (caches[1][hit_index_l2].RRPV != 0) {
                        caches[1][hit_index_l2].RRPV -= 1;
                    }
                    break;
            }
        } else {
            cache_r_miss_count++;
            cache_r_memory_count++;
            cache_miss_count[1]++;
            free_index_l2 = get_cache_free_line_specific(set_base_l2, 1, temp_swap_style);
            set_cache_line((_u64) free_index_l2, addr, 1);
            switch (temp_swap_style) {
                case CACHE_SWAP_SRRIP_FP:
                case CACHE_SWAP_SRRIP:
                    caches[1][free_index_l2].RRPV = SRRIP_2_M_2;
                    break;
                case CACHE_SWAP_BRRIP:
                    caches[1][free_index_l2].RRPV = rand() / RAND_MAX > EPSILON ? SRRIP_2_M_1 : SRRIP_2_M_2;
                    break;
            }
            // 如果是动态策略，则还需要更新psel
            if (swap_style[1] == CACHE_SWAP_DRRIP) {
                if (set_flag == 1) {
                    PSEL++;
                } else if (set_flag == 0) {
                    PSEL--;
                }
            }
        }
    }
}

bool comp_by_value(std::pair<_u64, _u64> &a, std::pair<_u64, _u64> &b) {
    return a.second > b.second;
}


/*
 * 加载配置取样有关的参数
 * input_sample_num: 每页取样的cacheline数
 * page_unit：每页分成几分
 * input_sample_time：取样间隔时间
 * */
//void CacheSim::load_sample_config(int sample_num, int page_unit, int sample_time)
//{
//    this->sample_num = sample_num * page_unit;
//    this->max_page = new _u64[this->sample_num];
//    this->page_unit = page_unit;
//    this->sample_time = sample_time*1e6;
//
//    int tmp = this->page_unit;
//    if(tmp > 1) {
//        while(tmp!=1) {
//            bit_num++;
//            tmp /= 2;
//        }
//    }
//    printf("bitnum: %d\n", bit_num);
//}
//
//
//
//void CacheSim::sample(_u64 addr, double op_time) {
//    envoke_cnt++;
////    if(envoke_cnt>453641&&envoke_cnt<912512)
////    {
////        block_set.insert(block_set.end(), addr);
////    }
//
//
////    _u64 sample_time = 3e6;
//    _u64 Page_number = addr >> (12-bit_num);
////    printf("***Page_number:%llu***\n", Page_number);
//    //开始时间
//    if (start_time == 0) {
//        start_time = op_time;
//    }
//
//    //开始取样
//    if (op_time - start_time < sample_time) {
//        //在取样周期内
//        sample_map[Page_number]++;
//    } else {
//        sample_cnt++;
////        printf("envoke_cnt:%d\n", envoke_cnt);
////        printf("=====================\n");
//        std::vector<std::pair<_u64, _u64>> result(sample_map.begin(), sample_map.end());
//        std::sort(result.begin(), result.end(), comp_by_value);
//        int tmp = 0;
//        std::vector<std::pair<_u64, _u64>>::iterator it;
//        //找到取样时间内最多的n页
//        for (it = result.begin(); it != result.end() && tmp < this->sample_num; it++) {
//            this->max_page[tmp++] = it->first;
//        }
//
//        //sample_map的长度即为当前周期内page的数量
////        printf("sample_cnt:%d\n", sample_cnt);
////        if(sample_cnt == 1)
////        {
////            std::map<_u64, _u64>::iterator iter;
////            iter = sample_map.begin();
////            while(iter!=sample_map.end())
////            {
//////                printf("sample_page: %llu\n", iter->first);
////                iter++;
////            }
////        }
//        sample_map.clear();
//        start_time = op_time;
//    }
//}
/*********************************************************/


/**
 *
 * */
//void CacheSim::directHit(_u64 addr,char oper_style){
//
//
//
//    long long hit_index_l2 = check_cache_hit()
//    if (oper_style == OPERATION_WRITE) {
//        cache_w_hit_count++;
//        cache_hit_count[1]++;
//        caches[1][hit_index_l2].count = tick_count;
//        if (write_through) {
//            cache_w_memory_count++;
//        } else {
//            caches[1][hit_index_l2].flag |= CACHE_FLAG_DIRTY;
//        }
//        switch (temp_swap_style) {
//            case CACHE_SWAP_BRRIP:
//            case CACHE_SWAP_SRRIP:
//                caches[1][hit_index_l2].RRPV = 0;
//            break;
//                case CACHE_SWAP_SRRIP_FP:
//                    if (caches[1][hit_index_l2].RRPV != 0) {
//                        caches[1][hit_index_l2].RRPV -= 1;
//                    }
//                    break;
////                case CACHE_SWAP_DRRIP:
////                    break;
//            }
//    } else { //read
//            cache_r_hit_count++;
//            cache_hit_count[1]++;
//            caches[1][hit_index_l2].count = tick_count;
//            switch (temp_swap_style) {
//                case CACHE_SWAP_BRRIP:
//                case CACHE_SWAP_SRRIP:
//                    caches[1][hit_index_l2].RRPV = 0;
//                    break;
//                case CACHE_SWAP_SRRIP_FP:
//                    if (caches[1][hit_index_l2].RRPV != 0) {
//                        caches[1][hit_index_l2].RRPV -= 1;
//                    }
//                    break;
//            }
//    }
//}



/**
 * 查找filter中被替换的page
 * 返回：victim page的index；没有这样的页则返回-1
 * */
//int CacheSim::find_free_page(){
//
//    find_cnt++;
//
//    //从p_pointer处开始遍历filter
//    Page p;
//
//    for (int j = ptr_index; j < filter.size(); j++){
//        p = filter[j];
//        if(p.frequency > hot_page_thresh_hold && p.rotation_cnt < expiration) //p是hot page，保留它
//            continue;
//        else {  //p为victim page，将其替换
//            return j;
//        }
//    }
//
//    for (int k = 0; k < ptr_index; k++) {
//        p = filter[k];
//        if (p.frequency > hot_page_thresh_hold && p.rotation_cnt < expiration) //p是hot page，保留它
//            continue;
//        else {//p为victim page，将其替换
//            return k;
//        }
//    }
//
////    printf("direct miss");
//    return -1;
//}
//
///*
// * 根据CLOCK-DWF算法的思想来预取page
// * filter中的每个1/16 page要维持两个参数：page_frequency；rotation_count
// * filter size设为1000*16
// * */
//void CacheSim::CLOCK_DWF(_u64 addr, char style){
//    Page p;
//    p.page_num = addr >> 9; //考虑1/16页
//
//    //判断当前页是否在filter中
//    std::vector<Page>::iterator iter;
//    iter = std::find(filter.begin(), filter.end(), p);
//    bool inFilter;
//    int index; //如果该页在filter中，其下标
//    int directMiss = 0;
//
//    if(iter != filter.end()) {
//        inFilter = true;
//        index = std::distance(filter.begin(), iter);
//    }
//    else
//        inFilter = false;
//
//    /*当前访问的页不在filter中*/
//    if(!inFilter) {
//        /*filter没有满，则存放该页*/
//        if(filter.size() < filter_size) {
//
////            printf("filter没满：do cache op\n"); //运行1000次
//
//            p.frequency = 1;
//            p.rotation_cnt = 0;
//            filter.push_back(p);
//
//            do_cache_op(addr, style);
//
////            page_pointer = &filter[0];
//            ptr_index = 0;//下一次迭代开始的位置
//            index = filter.size()-1;
//        }
//        /*filter满了，寻找被替换的页*/
//        else {
//            int victim_idx = find_free_page(); //victim page的index
//
//
//            if(victim_idx != -1) { //找到了，替换
//
////                printf("victim page: %d\n", victim_idx);
//
//                p.frequency = 1;
//                p.rotation_cnt = 0;
//                filter.insert(filter.begin()+victim_idx, p);
//
//                //删除victim page
//                filter.erase(filter.begin()+victim_idx+1);
//
//                do_cache_op(addr, style);
//
////                page_pointer = &filter[victim_idx+1];
//                index = victim_idx;
//                ptr_index = index+1;
//            }
//            else { //没有找到victim page，该页不放入filter，直接miss
//                /*直接miss之前要先判断当前cache line是否已经在cache中*/
//                long long set_l2, set_base_l2, hit_index_l2;
//                set_l2 = (addr >> cache_line_shifts[1]) % cache_set_size[1];
//                set_base_l2 = set_l2 * cache_mapping_ways[1];
//                hit_index_l2 = check_cache_hit(set_base_l2, addr, 1);
//                int temp_swap_style = swap_style[1];
//
//                if (hit_index_l2 >= 0){
//                    /*判断读写*/
//                    if (style == OPERATION_READ){
//                        cache_r_count++;
//                        cache_r_hit_count++;
//                        cache_hit_count[1]++;
//                        caches[1][hit_index_l2].count = tick_count;
//                        switch (temp_swap_style) {
//                            case CACHE_SWAP_BRRIP:
//                            case CACHE_SWAP_SRRIP:
//                                caches[1][hit_index_l2].RRPV = 0;
//                                break;
//                            case CACHE_SWAP_SRRIP_FP:
//                                if (caches[1][hit_index_l2].RRPV != 0) {
//                                    caches[1][hit_index_l2].RRPV -= 1;
//                                }
//                                break;
//                        }
//                    }
//                    else{
//                        cache_w_count++;
//                        cache_w_hit_count++;
//                        cache_hit_count[1]++;
//                        caches[1][hit_index_l2].count = tick_count;
//                        if (write_through) {
//                            cache_w_memory_count++;
//                        } else {
//                            caches[1][hit_index_l2].flag |= CACHE_FLAG_DIRTY;
//                        }
//                        switch (temp_swap_style) {
//                            case CACHE_SWAP_BRRIP:
//                            case CACHE_SWAP_SRRIP:
//                                caches[1][hit_index_l2].RRPV = 0;
//                                break;
//                            case CACHE_SWAP_SRRIP_FP:
//                                if (caches[1][hit_index_l2].RRPV != 0) {
//                                    caches[1][hit_index_l2].RRPV -= 1;
//                                }
//                                break;
////                case CACHE_SWAP_DRRIP:
////                    break;
//                        }
//                    }
//                }
//                else{ /*cache里也没有，直接miss*/
//                    //                printf("direct miss\n");
//                    miss_cnt++;
//                    directMiss = 1;
//
//                    if(style == OPERATION_WRITE)
//                        cache_w_miss_count++;
//                    else
//                        cache_r_miss_count++;
//                    cache_miss_count[1]++;
//                }
//                ptr_index = 0;//?没有找到则下一次查找从filter头开始
//            }
//        }
//    }
//    /*当前访问的页在filter中，此时页的下标是index*/
//    else {
//
////        printf("页在filter中：do cache op\n");
//        do_cache_op(addr, style);//应该是直接命中的
//        filter[index].frequency++;
//        filter[index].rotation_cnt = 0;
//        //直接命中
//
//
////        *page_pointer = filter[index+1];
//        ptr_index = index+1;
//    }
//
//    //对于所有未涉及的页，rotation_cnt++
//    for(int i=0; i<filter.size(); i++) {
//        if(directMiss == 1)
//            filter[i].rotation_cnt++;
//        else
//            if(i!=index)
//                filter[i].rotation_cnt++;
//
//    }
//
////    for(int i=0; i<filter.size(); i++) {
////        printf("rotation:%llu\n", filter[i].rotation_cnt);
////
////    }
//}

/**
 * CAR-directMiss(): 没有页能从filter中替换出去，检查page是否在cache中
 *      若在，则直接做cache命中的操作；否则直接cache miss
 * */
void CacheSim::directMiss(_u64 addr, char style){

//    printf("directMiss():directMiss\n");//可以进到这里来

    /*直接miss之前要先判断当前cache line是否已经在cache中*/
    long long set_l2, set_base_l2, hit_index_l2;
    set_l2 = (addr >> cache_line_shifts[1]) % cache_set_size[1];
    set_base_l2 = set_l2 * cache_mapping_ways[1];
    hit_index_l2 = check_cache_hit(set_base_l2, addr, 1);
//    int temp_swap_style = swap_style[1];

    if (hit_index_l2 >= 0){
        /*判断读写*/
        if (style == OPERATION_READ){
            cache_r_count++;
            cache_r_hit_count++;
            cache_hit_count[1]++;
            caches[1][hit_index_l2].count = tick_count;
        }
        else{
            cache_w_count++;
            cache_w_hit_count++;
            cache_hit_count[1]++;
            caches[1][hit_index_l2].count = tick_count;
            if (write_through) {
                cache_w_memory_count++;
            } else {
                caches[1][hit_index_l2].flag |= CACHE_FLAG_DIRTY;
            }

        }
    }
    else{ /*cache里也没有，直接miss*/

        if(style == OPERATION_WRITE)
            cache_w_miss_count++;
        else
            cache_r_miss_count++;
        cache_miss_count[1]++;
    }
}



int CacheSim::replace(){

//    printf("replace\n");

    int isfound = 0;
    //T1
    if(T1.QueueLength()>=std::max(1, p_t1)) { /*|T1|>=max(1,p_t1)*/
        for(int i=0; i<T1.QueueLength(); i++) {
            if(T1.m_pQueue[T1.m_iHead].reference_bit==0) {
                isfound = 1;
                /*删除T1头的页，将它放在B1的MRU处*/
                B1.push_back(*T1.DeQueue());
                return isfound;
            }
            else{
                T1.m_pQueue[T1.m_iHead].reference_bit = 0;
                /*将当前页放到T2尾*/
                T2.EnQueue(*T1.DeQueue());
            }
        }
    }
    else{ //T2

//        printf("replace():遍历T2\n");

        for(int j = 0; j < T2.QueueLength(); j++){
            if(T2.m_pQueue[T1.m_iHead].reference_bit==0){
                isfound=1;
                /*删除T2头页，将它放在B2的MRU处*/
                B2.push_back(*T2.DeQueue());
                enqueue_cnt++;

                return isfound;
            }
            else{
                T2.m_pQueue[T1.m_iHead].reference_bit = 0;
                /*将当前页放在T2尾*/
                T2.EnQueue(*T2.DeQueue());
            }
        }
    }

    return isfound;
}

/**
 * CAR
 * */
 void CacheSim::CAR(_u64 addr, char style){
     Page p = Page();
     p.page_num = addr >> 9;

     int index_in_T1 = T1.Find(p), index_in_T2 = T2.Find(p);
     auto iter_B1 = std::find(B1.begin(), B1.end(), p);
     auto iter_B2 = std::find(B2.begin(), B2.end(), p);


     if(index_in_T1!= -1 || index_in_T2 != -1) { /**filter hit*/

//         printf();

//        p.reference_bit = 1; //问题：不能将filter中对用的page的reference bit置成1

        if(index_in_T1!= -1){ //page在T1中
            T1.m_pQueue[index_in_T1].reference_bit = 1;
        }
        else{ //page在T2中
            T2.m_pQueue[index_in_T2].reference_bit = 1;
        }

        do_cache_op(addr, style);
     }
     else{ /**filter miss*/
         if((T1.QueueLength()+T2.QueueLength())==filter_size) { /**filter满了*/

             int isFound = replace();

             if(isFound==1){ //找到了victim page，剔除操作在replace()已经操作完毕

                 do_cache_op(addr, style);

                 /*对B1 B2进行操作*/
                 if((iter_B1==B1.end()&& iter_B2==B2.end())&&(T1.QueueLength()+B1.size()==filter_size)) /*p不在B1 B2中，|T1|+|B1|=filter_size*/
                 {
                     /*删除B1中的LRU page*/
//                     printf("入 B1的长度为: %d\n", B1.get_size());
                     B1.pop_front();
//                     printf("出 B1的长度为: %d\n", B1.get_size());

                 }
                 else if((T1.QueueLength()+B1.size()+T2.QueueLength()+B2.size()==(2*filter_size))&&(iter_B1==B1.end() && iter_B2==B2.end())) /*|T1|+|B1|+|T2|+|B2|=2*filter_size，p不在B1 B2中*/
                 {
                     /*删除B2中的LRU page*/
                     B2.pop_front();
                     dequeue_cnt++;
//                     printf("B2的长度：%d\n", B2.get_size());
                 }

             }
             else { //没有可以替换的page
                 directMiss(addr, style);
                 return;
             }
         }
         else { /*filter没有满*/

             do_cache_op(addr, style);
         }


         /**把p放进filter中*/
         if(iter_B1==B1.end() && iter_B2==B2.end()) /*p不在B1 B2中*/
         {
             /*将p插到T1尾，reference_bit=0*/
             p.reference_bit = 0;
             T1.EnQueue(p);
         }
         else if(iter_B1!= B1.end()) /*p在B1中*/
         {
             /*adapt*/
             p_t1 = std::min(p_t1+(std::max(1*1.0, 1.0*B2.size()/B1.size())), filter_size*1.0);
             /*p从B1中删除，移到T2尾，reference_bit=0*/
             iter_B1->reference_bit = 0;
             Page temp1 = *iter_B1;
             B1.erase(iter_B1);
             T2.EnQueue(temp1);
         }
         else /*p在B2中*/
         {
             /*adapt*/
             p_t1 = std::max(p_t1-std::max(1*1.0,1.0*B1.size()/B2.size()),0*1.0);
             /*p从B2中删除，移到T2尾，reference_bit=0*/
             iter_B2->reference_bit = 0;
             Page temp2 = *iter_B2;
             delete_cnt++;
             T2.EnQueue(temp2);

//             p.reference_bit = 0;/*错误的做法*/
//             B2.delete_node(B2.frond, p);
//             T2.EnQueue(p);
         }
    }
//     printf("enqueue_cnt:%d\n", enqueue_cnt);
//     printf("dequeue_cnt:%d\n", dequeue_cnt);
//     printf("delete_cnt:%d\n", delete_cnt);
 }



/**从文件读取trace，在我最后的修改目标里，为了适配项目，这里需要改掉*/
void CacheSim::load_trace(const char *filename) {
    char buf[128];
    // 添加自己的input路径
    FILE *fin;
    // 记录的是trace中指令的读写，由于cache机制，和真正的读写次数当然不一样。。主要是如果设置的写回法，则写会等在cache中，直到被替换。
    _u64 rcount = 0, wcount = 0;
    fin = fopen(filename, "r");

    if (!fin) {
        printf("load_trace %s failed\n", filename);
        return;
    }
    int get_miss_rate_interval = 10000;
    std::ofstream fmiss_rate("fmiss_rate");
    _u64 i = 0;
    while (fgets(buf, sizeof(buf), fin)) {
        char tmp_style[5];
        char style;
        // 原代码中用的指针，感觉完全没必要，而且后面他的强制类型转换实际运行有问题。addr本身就是一个数值，32位unsigned int。
        _u64 addr = 0;
        int datalen = 0;
        int burst = 0;
        int mid = 0;
        float delay = 0;
        float ATIME = 0;
        int ch = 0;
        int qos = 0;

        sscanf(buf, "%s %x %d %d %x %f %f %d %d", tmp_style, &addr, &datalen, &burst, &mid, &delay, &ATIME, &ch, &qos);
        if (strcmp(tmp_style, "nr") == 0 || strcmp(tmp_style, "wr") == 0) {
            style = 'l';
        } else if (strcmp(tmp_style, "nw") == 0 || strcmp(tmp_style, "naw") == 0) {
            style = 's';
        } else {
            printf("%s", tmp_style);
            return;
        }

//        do_cache_op(addr, style);

        /*sample*/
//        sample(addr, ATIME);

//        //判断(不在前1500页时，还要考虑命不命中)
//        int cnt;
//        int is_find = 0;
//        for (cnt = 0; cnt < this->sample_num; cnt++) {
//            if (this->max_page[cnt] == addr >> (12-bit_num))
//            {
//                is_find = 1;
//                break;
//            }
//        }
//
//        if (is_find == 1) {
//            do_cache_op(addr, style);
//        } else {
//            if(style == OPERATION_WRITE)
//                cache_w_miss_count++;
//            else
//                cache_r_miss_count++;
//            cache_miss_count[1]++;
//        }

        /*CLOCK-filter + do cache op*/
//        CLOCK_DWF(addr, style);

        /*CAR-filter*/
        line_cnt++;
//        printf("line_cnt:%llu\n", line_cnt);
        CAR(addr, style);

//        printf("load_trace():出CAR\n");


        switch (style) {
            case 'l' :
                rcount++;
                break;
            case 's' :
                wcount++;
                break;
            case 'k' :
                break;
            case 'u' :
                break;
        }
        if (i % 10000000 == 0) {
            printf("insts index:%lld\n", i);
        }
        i++;
        if (get_miss_rate_interval > 0) { get_miss_rate_interval--; }
        else {
            get_miss_rate_interval = 10000;
            fmiss_rate << get_hit_rate(1) << "\t";
        }
    }
    fmiss_rate.close();
    // 文件中的指令统计
    printf("all r/w/sum: %lld %lld %lld \nread rate: %f%%\twrite rate: %f%%\n",
           rcount, wcount, tick_count,
           100.0 * rcount / tick_count,
           100.0 * wcount / tick_count
    );
    // miss率
//    printf("L1 miss/hit: %lld/%lld\t hit/miss rate: %f%%/%f%%\n",
//           cache_miss_count[0], cache_hit_count[0],
//           100.0 * cache_hit_count[0] / (cache_hit_count[0] + cache_miss_count[0]),
//           100.0 * cache_miss_count[0] / (cache_miss_count[0] + cache_hit_count[0]));
    printf("L2 miss/hit SM hit: %lld/%lld/%lld\t hit/miss rate: %f%%/%f%% ShareMemory hit %f%%\n",
           cache_miss_count[1], cache_hit_count[1], SM_hit_count,
           100.0 * cache_hit_count[1] / (cache_hit_count[1] + cache_miss_count[1] + SM_hit_count),
           100.0 * cache_miss_count[1] / (cache_miss_count[1] + cache_hit_count[1] + SM_hit_count),
           100.0 * SM_hit_count / (cache_miss_count[1] + cache_hit_count[1] + SM_hit_count));
    printf("read hit rate:%f%%\twrite hit rate:%f%%\n",
           100.0 * cache_r_hit_count / (cache_r_hit_count + cache_r_miss_count),
           100.0 * cache_w_hit_count / (cache_w_hit_count + cache_w_miss_count));
    printf("read hit count\t%lld\tmiss count\t%lld\nwrite hit count\t%lld\tmiss count\t%lld\n", cache_r_hit_count,
           cache_r_miss_count, cache_w_hit_count, cache_w_miss_count);
    printf("SM_in is %lld\t and cache in is %lld\n", SM_in, cache_miss_count[1]);
    printf("Write through:\t%d\twrite allocation:\t%d\n", write_through, write_allocation);
//    printf("%lld 被调出了%lld次 调入了%lld次\n", target, target_out, target_in);
    char a_swap_style[100];
    switch (swap_style[1]) {
        case CACHE_SWAP_LRU:
            strcpy(a_swap_style, "LRU");
            break;
        case CACHE_SWAP_DRRIP:
            strcpy(a_swap_style, "DRRIP");
            break;
        case CACHE_SWAP_SRRIP:
            strcpy(a_swap_style, "SRRIP");
            break;
        case CACHE_SWAP_BRRIP:
            strcpy(a_swap_style, "BRRIP");
            break;
        case CACHE_SWAP_SRRIP_FP:
            strcpy(a_swap_style, "SRRIP_FP");
            break;
    }
    printf("\n=======Cache Policy=======\n%s", a_swap_style);
    if (swap_style[1] == CACHE_SWAP_SRRIP) {
        printf("\t%d", SRRIP_M);
    }
    printf("\n=======Bandwidth=======\nMemory --> Cache:\t%.4fGB\nCache --> Memory:\t%.4fMB\ncache sum:\t%.4fGB\nsum:\t%.4fGB\n",
           cache_r_memory_count * cache_line_size[1] * 1.0 / 1024 / 1024 / 1024,
           cache_w_memory_count * cache_line_size[1] * 1.0 / 1024 / 1024,
           (cache_r_memory_count * cache_line_size[1] * 1.0 / 1024 / 1024 / 1024)+cache_w_memory_count * cache_line_size[1] * 1.0 / 1024 / 1024/ 1024, //cache sum：经过cache操作的流量总和
           (cache_miss_count[1]+ cache_hit_count[1]-tick_count) *cache_line_size[1] *1.0/1024/1024/1024 //未经cache的那部分流量
           +((cache_r_memory_count * cache_line_size[1] * 1.0 / 1024 / 1024 / 1024)+cache_w_memory_count * cache_line_size[1] * 1.0 / 1024 / 1024/ 1024)); //sum：包括了直接miss掉的那部分流量
//    printf("find count:%d\n", find_cnt);
//    printf("direct miss count:%d\n", miss_cnt);

    // 读写通信
//    printf("read : %d Bytes \t %dKB\n write : %d Bytes\t %dKB \n",
//           cache_r_count * cache_line_size,
//           (cache_r_count * cache_line_size) >> 10,
//           cache_w_count * cache_line_size,
//           (cache_w_count * cache_line_size) >> 10);
    fclose(fin);
//    FILE *fout;
//    fout = fopen("/home/embedded/文档/华为/运行结果6.29/取样后的片段/1m_sample1000_第二次取样.txt", "a");
//    for (int ii=0; ii < block_set.size(); ii++){
////        printf("%llu\n", block_set[ii]);
//        fprintf(fout, "%llu\n", block_set[ii]);
//    }
//    fclose(fout);

//    fprintf(fout,"hit rate: %f%%\nsum:\t%.4fGB\n",100.0 * cache_hit_count[1] / (cache_hit_count[1] + cache_miss_count[1] + SM_hit_count),
//            (cache_miss_count[1]+ cache_hit_count[1]-tick_count) *cache_line_size[1] *1.0/1024/1024/1024
//            +((cache_r_memory_count * cache_line_size[1] * 1.0 / 1024 / 1024 / 1024)+cache_w_memory_count * cache_line_size[1] * 1.0 / 1024 / 1024/ 1024));//写入到文件末尾

}


int CacheSim::lock_cache_line(_u64 line_index, int level) {
    caches[level][line_index].flag |= CACHE_FLAG_LOCK;
    return 0;
}

int CacheSim::unlock_cache_line(_u64 line_index, int level) {
    caches[level][line_index].flag &= ~CACHE_FLAG_LOCK;
    return 0;
}

