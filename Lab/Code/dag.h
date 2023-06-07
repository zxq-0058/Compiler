#ifndef _DAG_H_
#define _DAG_H_

#include "basicblock.h"
#include "intercode.h"

typedef struct DAGNode_* DAGNode;
typedef struct BasicBlockDAG_* BasicBlockDAG;

// DAG节点
enum {
    NODE_RESERVE,  // 该节点存储原来的代码（保持不变，方便导出）
    NODE_LAEF,
    NODE_NORMAL_ASS,
    NODE_GETADDR_ASS,
    NODE_GETVAL_ASS,
    NODE_SETVAL_ASS,
    NODE_ADD,
    NODE_SUB,
    NODE_MUL,
    NODE_DIV
};

struct DAGNode_ {
    Operand op;               // 节点代表的操作数
    int id;                   // 节点的编号
    int node_kind;            // 节点代表的运算符，0表示为叶子节点
    int parent_num;           // 父节点的数量（不确定）
    DAGNode parent[32];       // 父节点数组
    int star_parent_num;      // star 父节点数量
    DAGNode star_parent[32];  // t -> *t or v -> *v 一类特殊的父节点
    int child_num;            // 子节点数量: 1 || 2
    DAGNode children[2];      // 孩子节点数组
    InterCode code;           // 该节点对应的中间代码(只是为了方便导出代码)
    int is_deleted;           // 节点是否被删除
};

struct BasicBlockDAG_ {
    int root_count;  // 根节点数量
    DAGNode* roots;  // 根节点数组
};

void common_exp_optimize(BasicBlock block);

#endif