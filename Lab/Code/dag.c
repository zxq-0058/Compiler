#include "dag.h"

BasicBlockDAG buildDAG(InterCodes start, InterCodes end) {
    // 初始化DAG
    BasicBlockDAG dag = (BasicBlockDAG)malloc(sizeof(struct BasicBlockDAG_));
    for (InterCodes code = start; code != end; code = code->next) {
        int kind = code->code->kind;
        
    }
}