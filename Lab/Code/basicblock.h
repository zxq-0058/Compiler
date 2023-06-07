#ifndef _BASE_BASICBLOCK_H_
#define _BASE_BASICBLOCK_H_
#include "bitset.h"
#include "intercode.h"
#include "list.h"

/// @brief 基本块
typedef struct BasicBlock_* BasicBlock;
typedef struct FlowGraph_* FlowGraph;
typedef struct Instruction_* Instruction;
typedef struct Value_* Value;
typedef struct FlowNode_* FlowNode;
typedef struct BackEdge_* BackEdge;
typedef struct Loop_* Loop;
typedef struct Lp_Var_* Lp_Var;

// used for constant propagation
struct Value_ {
    enum { UNDEF, NAC, CONST } kind;
    long long int value;
};

/// @brief 中间代码
struct Instruction_ {
    struct list_head node;
    InterCode code;
};

struct FlowNode_ {
    struct list_head node;
    BasicBlock block;
};

struct BasicBlock_ {
    struct list_head interCodes;    // 基本块的中间代码(双向链表头节点)
    InterCodes start;               // 基本块的起始节点
    InterCodes end;                 // 基本块的结束节点 (不包含: NULL或者下一个基本块的起始节点)
    BasicBlock next;                // 下一个基本块
    BasicBlock prev;                // 上一个基本块
    struct list_head predecessors;  // 前驱基本块(双向链表)
    struct list_head successors;    // 后继基本块(双向链表)
    Bitset dominators;              // 支配集合(bitset) (支配该基本块的所有基本块)
    Bitset subordinates;            // 支配集合(bitset) (该基本块支配的所有基本块)
    int bid;                        // 基本块的编号

    // 常量传播相关
    Value* in;   // 入口处的值 (一个map: 变量->值)
    Value* out;  // 出口处的值 (一个map: 变量->值)

    // 支配树相关
    Bitset in_dom;   // 入口处的支配集合
    Bitset out_dom;  // 出口处的支配集合
};

/// @brief  反向边
struct BackEdge_ {
    struct list_head node;
    BasicBlock from;
    BasicBlock to;
};

/// @brief 循环
struct Loop_ {
    struct list_head node;
    BasicBlock header;  // 循环头
    Bitset body;        // 循环体
    Bitset exits;       // 循环的出口
};

// 流图
struct FlowGraph_ {
    struct list_head _graph;  // 考虑到可能有多个函数，所以用链表存储
    BasicBlock entry;         // 入口基本块
    BasicBlock exit;          // 出口基本块
    int blockCount;           // 基本块的数量
#define MAX_BLOCKS_NR 128
    BasicBlock blocks[MAX_BLOCKS_NR];  // 基本块数组(方便将id映射到基本块)
    bool is_deleted[MAX_BLOCKS_NR];    // 标记基本块是否被删除
    int dfn[MAX_BLOCKS_NR];            // 基本块的深度优先编号

    struct list_head back_edges;  // 反向边(双向链表)
    struct list_head loops;       // 循环(考虑到一个流图可能有多个循环，因此使用双向链表)
};

/// 循环内部的变量信息
struct Lp_Var_ {
    bool is_const;      // 是否为常量
    Instruction instr;  // 对应的中间代码
    Bitset def;         // 定义点
    Bitset use;         // 使用点
};

void del_all_intercodes(BasicBlock block);
void append_intercode(BasicBlock block, InterCode code);
/// @brief 划分基本块
void split_into_basicblocks(InterCodes interCodes, BasicBlock* basicBlocks, int* blockCount);
/// @brief 打印基本块(调试)
void print_basicblocks(BasicBlock basicBlocks, int blockCount);

/// @brief 给定基本块链表，创建流图
void create_flowgraphs(BasicBlock basicBlocks);

/// @brief 全局常量传播
void global_const_propagation();

/// @brief 计算支配集合
void compute_dominators();

/// @brief dfn排序
void compute_dfn();

/// @brief 计算回边
void compute_backedges();

/// @brief 计算循环
void compute_loops();

/// @brief 循环优化
void loop_const_optimize();

/// @brief 导出流图
void flw_graphs_export_code(FILE* fp);
#endif