#ifndef _OPTIMIZE_H_
#define _OPTIMIZE_H_

#include "list.h"

struct BlockList_ {
    struct BasicBlock_ *entry;  // 基本块链的入口
    struct BasicBlock_ *exit;   // 基本块链的出口
    int blockCount;             // 基本块链的基本块数量
    struct BlockList_ *next;    // 下一个基本块链
};


#endif