#ifndef _DAG_H_
#define _DAG_H_

#include "intercode.h"

typedef struct DAGNode_* DAGNode;
typedef struct BasicBlockDAG_* BasicBlockDAG;

struct DAGNode_ {
    Operand op;      // 节点代表的操作数
    int op_idx;      // 因为可能会对同一个操作数进行多次赋值，所以需要记录操作数的下标
    int op_kind;     // 节点代表的运算符，0表示为叶子节点
    DAGNode parent;  // 父节点
    int child_num;   // 子节点数量: 1 || 2
    DAGNode children[2];  // 孩子节点数组
    int visited;          // 节点是否被访问过
};

struct BasicBlockDAG_ {
    int root_count;  // 根节点数量
    DAGNode* roots;  // 根节点数组
};

#endif