#include "basicblock.h"
#include "intercode.h"

/// @brief 删除一条中间代码
void delete_intercodes(InterCodes codes) {
    Panic_on(codes == NULL, "codes is NULL");
    if (codes->prev) {
        codes->prev->next = codes->next;
    }
    if (codes->next) {
        codes->next->prev = codes->prev;
    }
}

// 区间 [start, end) 中等号右边是否出现了 op
// 不需要考虑左边的原因是，如果左边出现了，那么意味着变量的值被改变了，所以当前指令也可以被删除
int is_occur_in_left(InterCodes start, InterCode end, Operand op) {
    InterCodes current = start;
    while (current != end) {
        if (current->code->kind == IR_ASSIGN) {
            Operand left = current->code->u.assign.left;
            if (operand_equal(left, op)) {
                return 1;
            }
        } else if (isBinop(current->code->kind)) {
            Operand result = current->code->u.binop.result;
            if (operand_equal(result, op)) {
                return 1;
            }
        }
        current = current->next;
    }
    return 0;
}

/// 对于每一个出现在 = 左边的变量，如果在同一个基本块的 = 右边没有出现，那么就将这个所在的代码删除
void deadcode_elimination(BasicBlock bb) {
    InterCodes current = bb->start;
    while (current != bb->end) {
        if (current->code->kind == IR_ASSIGN) {
            Operand left = current->code->u.assign.left;
            if (left->kind == OP_TEMP && !is_occur_in_left(current->next, bb->end, left)) {
                // 如果在同一个基本块的当前指令后面 = 右边没有出现，那么就将这个所在的代码删除
                delete_intercodes(current);
            }
        } else if (isBinop(current->code->kind)) {
            Operand result = current->code->u.binop.result;
            if (result->kind == OP_TEMP && !is_occur_in_left(current->next, bb->end, result)) {
                // 如果在同一个基本块的当前指令后面 = 右边没有出现，那么就将这个所在的代码删除
                delete_intercodes(current);
            }
        }
        current = current->next;
    }
}