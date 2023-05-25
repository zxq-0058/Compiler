#ifndef _BASE_BASICBLOCK_H_
#define _BASE_BASICBLOCK_H_
#include "intercode.h"

/// @brief 基本块
typedef struct BasicBlock_* BasicBlock;

struct BasicBlock_ {
    InterCodes start;  // 基本块的起始节点
    InterCodes end;    // 基本块的结束节点 (不包含: NULL或者下一个基本块的起始节点)
    BasicBlock next;   // 下一个基本块
    BasicBlock prev;   // 上一个基本块
};

/// @brief 划分基本块
void split_into_basicblocks(InterCodes interCodes, BasicBlock* basicBlocks, int* blockCount);
/// @brief 打印基本块(调试)
void print_basicblocks(BasicBlock basicBlocks, int blockCount);

#endif