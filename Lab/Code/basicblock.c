// 划分基本块
#include "basicblock.h"

extern void print_intercode(InterCode code, FILE* fp);

static void inline appendBasicBlock(BasicBlock* head, BasicBlock* tail, int* blockCount, BasicBlock block) {
    if (tail == NULL || block == NULL) {
        Panic("tail is NULL");
    } else if (*tail == NULL) {
        *tail = block;
        *head = block;
        (*blockCount)++;
        return;
    }
    (*tail)->next = block;
    block->prev = *tail;
    *tail = block;
    (*blockCount)++;
}

/// @brief 判断一个中间代码是否是基本块的起始节点
static inline int is_start_intercode(InterCodes interCodes) {
    Panic_on(interCodes == NULL, "interCodes is NULL");
    return interCodes->code->kind == IR_LABEL || interCodes->code->kind == IR_FUNCTION;
}

/// @brief 判断一个中间代码是否是基本块的结束节点
static inline int is_end_intercode(InterCodes interCodes) {
    if (interCodes == NULL) {
        return 1;
    }
    return interCodes->code->kind == IR_GOTO || interCodes->code->kind == IR_RETURN ||
           interCodes->code->kind == IR_IFGOTO;
}

void split_into_basicblocks(InterCodes interCodes, BasicBlock* basicBlocks, int* blockCount) {
    BasicBlock blocks = NULL;
    BasicBlock head = NULL;
    InterCodes current = interCodes;
    InterCodes prev = NULL;
    int count = 0;
    while (current != NULL) {
        if (is_start_intercode(current) || is_end_intercode(prev)) {
            // 如果当前节点是基本块的起始节点或者上一个节点是基本块的结束节点
            // Debug("找到一个基本块 [ %d ] 的起点：\n", count);
            // print_intercode(current->code, stdout);
            BasicBlock block = (BasicBlock)malloc(sizeof(struct BasicBlock_));
            block->start = current;
            appendBasicBlock(&head, &blocks, &count, block);
        }
        prev = current;
        current = current->next;
    }

    *basicBlocks = head;
    *blockCount = count;
    // 枚举所有的基本块，对bb->end进行赋值
    count = 0;
    BasicBlock block = head;
    while (block != NULL) {
        if (block->next) {
            block->end = block->next->start;
        } else {
            block->end = NULL;
        }
        block = block->next;
        count++;
    }

    Debug("共找到%d个基本块\n", count);
}

void print_basicblocks(BasicBlock basicBlocks, int blockCount) {
#ifdef DEBUG_ON
    BasicBlock block = basicBlocks;
    int i = 0;
    while (block != NULL) {
        Debug("Basic Block %d:\n", i);
        InterCodes current = block->start;
        while ((block->next && current != block->next->start) || (block->next == NULL && current != NULL)) {
            print_intercode(current->code, stdout);
            current = current->next;
        }
        // print_intercode(current->code, stdout);
        Debug("\n");
        block = block->next;
        i++;
    }
#endif
}
