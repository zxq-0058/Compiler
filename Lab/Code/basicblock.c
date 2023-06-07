// 划分基本块
#include "basicblock.h"

#define FLOW_G_MAX_VAR 512   // 最大的变量数量
#define FLOW_G_MAX_TEMP 512  // 最大的临时变量数量
#define FLOW_G_MAX_MAP ((FLOW_G_MAX_VAR + FLOW_G_MAX_TEMP) * 3)

struct list_head graphs;  // 流图链表

static int dec_var_addr = 0;  // 用于数组和结构体等的地址分配（DEC指令需要）

static inline Operand new_const_op(int value) {
    Operand op = (Operand)malloc(sizeof(struct Operand_));
    op->kind = OP_CONSTANT;
    op->u.value = value;
    return op;
}

/**
 * BasicBlock是整个实验的核心数据结构
 * （1）能够方便的添加和删除中间代码 list_append(code)和list_del(code)，code是Intercode类型
 * （2）能够方便的添加和删除前驱和后继基本块： add_predecessor(block)和add_successor(block)，所以前驱和后驱都得是链表
 * BasicBlock对外应该体现的接口：
 *
 * 需要思考一个问题：将原来的指令序列按照顺序分割为一个基本块的链表之后，我们如何建立流图？
 * （1）每一个FUNCTION对应一个entry block，每一个FUNCTION的最后一条指令对应一个exit block
 * （2）查看每一个基本块的最后一条指令
 *      （2.1）如果是GOTO 或者 IFGOTO，那么这个基本块的后继基本块就是这条指令的标号对应的基本块
 *      （2.2）如果是RETURN，那么这个基本块的后继基本块就是exit block（NULL）
 *       (2.3) 其它的一般情况，后继基本块就是下一个基本块
 *
 * 如何建立支配关系？
 * （1）如果没有前驱基本块，那么支配集合就是自己
 * （2）如果有前驱基本块，那么支配集合就是前驱基本块的支配集合的交集，然后加上自己
 * 算法貌似轻描淡写很简单，但是如何“巧妙”地实现交集，似乎是不太容易的。我使用的是一个C版本的Bitset，每一个位对应一个基本块（基本块的id对应于其在流图中的id）
 *
 * 如何建立定值关系？（到达定值分析）
 * 简单来说，我们需要实现Bitset in 以及 out，分别代表一个Block的定值集合，其中的位对应于一个变量的id
 * in[B] = ∪ out[P]，其中P是B的前驱基本块
 * out[B] = gen[B] ∪ (in[B] - kill[B])
 *
 * 在做循环代码外提的任何工作前，应该进行常量传播，同时进行死代码消除，如何进行常量传播？
 * 常量传播中一个重要的数据结构是 Operand -> ValueType的映射，其中ValueType是一个枚举类型，包括：
 * （1）CONSTANT：常量
 * （2）NAC : not a constant
 * （3）UNDEF：未定义
 *
 */

/// @brief 返回基本块的第一条中间代码
InterCode block_start_intercode(BasicBlock block) {
    Panic_on(block == NULL, "block is NULL");

    if (!list_empty(&block->interCodes)) {
        struct Instruction_* start_instruction = list_entry(block->interCodes.next, struct Instruction_, node);
        return start_instruction->code;
    }

    return NULL;
}

/// @brief 返回基本块的最后一条中间代码
InterCode block_end_intercode(BasicBlock block) {
    Panic_on(block == NULL, "block is NULL");

    if (!list_empty(&block->interCodes)) {
        struct Instruction_* end_instruction = list_entry(block->interCodes.prev, struct Instruction_, node);
        return end_instruction->code;
    }

    return NULL;
}

/// @brief 申请一个新的BLOCK
static inline BasicBlock alloc_block() {
    BasicBlock block = (BasicBlock)malloc(sizeof(struct BasicBlock_));
    memset(block, 0, sizeof(struct BasicBlock_));

    init_list_head(&block->interCodes);
    init_list_head(&block->predecessors);
    init_list_head(&block->successors);

    block->dominators = NULL;

    block->in = NULL;
    block->out = NULL;
    block->bid = -1;
    return block;
}

/// @brief 给一个基本块append一行中间代码
void append_intercode(BasicBlock block, InterCode code) {
    Panic_on(block == NULL || code == NULL, "block or code is NULL");

    Instruction new_instruction = (struct Instruction_*)malloc(sizeof(struct Instruction_));
    Panic_on(new_instruction == NULL, "new_instruction is NULL");
    // 初始化新指令
    init_list_head(&new_instruction->node);
    new_instruction->code = code;

    // 将新指令追加到基本块的 interCodes 链表末尾
    list_append(&new_instruction->node, &block->interCodes);
}

/// @brief 删除一个基本块中的所有中间代码(局部优化重构代码时需要)
void del_all_intercodes(BasicBlock block) {
    Panic_on(block == NULL, "block is NULL");

    init_list_head(&block->interCodes);
    // Instruction instruction = NULL;
    // Instruction next = NULL;

    // for (instruction = container_of(block->interCodes.next, struct Instruction_, node);
    //      &instruction->node != &block->interCodes; instruction = next) {
    //     next = container_of(instruction->node.next, struct Instruction_, node);
    //     del_instruction(block, instruction);
    // }
}

/// @brief 删除一个基本块中的一行中间代码
void del_instruction(BasicBlock block, Instruction instruction) {
    Panic_on(block == NULL, "block is NULL");
    Panic_on(instruction == NULL, "instruction is NULL");
    list_del(&instruction->node);
    free(instruction);
}

/// @brief 给一个基本块添加一个前驱基本块
void add_predecessor(BasicBlock block, BasicBlock predecessor) {
    Panic_on(block == NULL, "block is NULL");
    Panic_on(predecessor == NULL, "predecessor is NULL");
    FlowNode node = (FlowNode)malloc(sizeof(struct FlowNode_));
    node->block = predecessor;
    list_append(&node->node, &block->predecessors);
}

/// @brief 删除一个基本块的一个前驱基本块
void delete_predecessor(BasicBlock block, BasicBlock predecessor) {
    Panic_on(block == NULL, "block is NULL");
    Panic_on(predecessor == NULL, "predecessor is NULL");
    FlowNode node = NULL;
    for_each_in_list(node, struct FlowNode_, node, &block->predecessors) {
        if (node->block == predecessor) {
            list_del(&node->node);
            free(node);
            return;
        }
    }
    Panic("delete_predecessor");
}

/// @brief 给一个基本块添加一个后继基本块
void add_successor(BasicBlock block, BasicBlock successor) {
    Panic_on(block == NULL, "block is NULL");
    Panic_on(successor == NULL, "successor is NULL");
    FlowNode node = (FlowNode)malloc(sizeof(struct FlowNode_));
    node->block = successor;
    list_append(&node->node, &block->successors);
}

/// @brief 删除一个基本块的一个后继基本块
void delete_successor(BasicBlock block, BasicBlock successor) {
    Panic_on(block == NULL, "block is NULL");
    Panic_on(successor == NULL, "successor is NULL");
    FlowNode node = NULL;
    for_each_in_list(node, struct FlowNode_, node, &block->successors) {
        if (node->block == successor) {
            list_del(&node->node);
            free(node);
            return;
        }
    }
    Panic("delete_successor");
}

extern void print_intercode(InterCode code, FILE* fp);

/// @brief 将一个基本块追加到链表中
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

/// @brief 将中间代码分割为基本块
/// @param interCodes
/// @param basicBlocks
/// @param blockCount
void split_into_basicblocks(InterCodes interCodes, BasicBlock* basicBlocks, int* blockCount) {
    BasicBlock blocks = NULL;
    BasicBlock head = NULL;
    InterCodes current = interCodes, prev = NULL;
    // InterCodes prev = NULL;
    int count = 0;
    BasicBlock block = NULL;
    while (current != NULL) {
        if (is_start_intercode(current) || is_end_intercode(prev)) {
            block = alloc_block();
            appendBasicBlock(&head, &blocks, &count, block);
        }
        append_intercode(block, current->code);
        prev = current;
        current = current->next;
    }

    *basicBlocks = head;
    *blockCount = count;
    Debug("共找到%d个基本块\n", count);
}
void print_basicblocks(BasicBlock basicBlocks, int blockCount) {
#ifdef DEBUG_ON
    BasicBlock block = basicBlocks;
    int i = 0;
    while (block != NULL) {
        Debug("Basic Block %d:\n", i + 1);

        Instruction instruction = NULL;
        for_each_in_list(instruction, struct Instruction_, node, &block->interCodes) {
            print_intercode(instruction->code, stdout);
        }

        Debug("\n");
        block = block->next;
        i++;
    }
#endif
}

/// @brief 申请一个新的流图
static inline FlowGraph alloc_flowgraph() {
    // TODO: 申请一个新的流图(需要完善)
    FlowGraph graph = (FlowGraph)malloc(sizeof(struct FlowGraph_));
    memset(graph, 0, sizeof(struct FlowGraph_));

    // TODO: 可能需要修改
    graph->entry = NULL;
    graph->exit = alloc_block();
    add2graph(graph, graph->exit);

    init_list_head(&graph->back_edges);
    init_list_head(&graph->loops);
    return graph;
}

/// @brief 给定一个基本块链表，查找label对应的基本块
BasicBlock find_label(BasicBlock basicBlocks, Operand label) {
    BasicBlock block = basicBlocks;
    while (block != NULL) {
        InterCode start = block_start_intercode(block);
        if (start->kind == IR_LABEL && operand_equal(start->u.one.op, label)) {
            return block;
        }
        block = block->next;
    }
    Debug("label not found\n");
    return NULL;
}

/// @brief 给定一个流图，查找label对应的基本块
BasicBlock find_label_in_flwgraph(FlowGraph g, Operand label) {
    for (int i = 0; i < g->blockCount; i++) {
        BasicBlock block = g->blocks[i];
        InterCode start = block_start_intercode(block);
        if (start && start->kind == IR_LABEL && operand_equal(start->u.one.op, label)) {
            return block;
        }
    }
    Panic("find_label_in_flwgraph");
}

/// @brief 给定一个流图，删除一个基本块(死代码消除)
void delete_node_in_flwgraph(FlowGraph g, BasicBlock block) {
    for (int i = 0; i < g->blockCount; i++) {
        if (g->blocks[i] == block) {
            g->is_deleted[i] = true;
            return;
        }
    }
    Panic("delete_node_in_flwgraph");
}

/// @brief 给定一个基本块，返回它在流图中的id
int get_id_in_graph(FlowGraph graph, BasicBlock block) {
    int i = 0;
    for (; i < graph->blockCount; i++) {
        if (graph->blocks[i] == block) {
            return i;
        }
    }
    return -1;
}

/// @brief 将一个基本块添加到流图的数组中
void add2graph(FlowGraph graph, BasicBlock block) {
    if (graph->blockCount >= MAX_BLOCKS_NR) {
        Panic("too many blocks");
    }
    block->bid = graph->blockCount;
    graph->blocks[graph->blockCount++] = block;
}

/// @brief 将一个基本块插入到流图的数组中,index为插入的位置，将[index, blockCount)的元素向后移动一位
void insert2graph(FlowGraph graph, BasicBlock block, int index) {
    if (graph->blockCount >= MAX_BLOCKS_NR) {
        Panic("too many blocks");
    }
    for (int i = graph->blockCount; i > index; i--) {
        graph->blocks[i] = graph->blocks[i - 1];
        graph->blocks[i]->bid = i;
    }
    graph->blocks[index] = block;
    block->bid = index;
    graph->blockCount++;
}

/// @brief 打印流图，同时将流图输出到文件中（方便可视化）
void print_flow_graph(FlowGraph graph, char* filename) {
    FILE* fp = fopen(filename, "w");
    Panic_on(fp == NULL, "fopen");
    BasicBlock block = NULL;
    for (int i = 0; i < graph->blockCount; i++) {
        // 打印predecessors
        block = graph->blocks[i];
        fprintf(fp, "Basic Block %d:\n", i);
        fprintf(fp, "successors: ");
        FlowNode successor = NULL;
        for_each_in_list(successor, struct FlowNode_, node, &block->successors) {
            fprintf(fp, "%d ", successor->block->bid);
        }
        fprintf(fp, "\n");
    }
}

/// @brief 给定基本块链表，创建流图
void create_flowgraphs(BasicBlock basicBlocks) {
    init_list_head(&graphs);
    BasicBlock block = basicBlocks;
    Panic_ON(block_start_intercode(block)->kind != IR_FUNCTION,
             "supposed to be FUNCTION");  //  第一个基本块的第一条指令必须为FUNCTION
    FlowGraph graph = NULL;
    while (block != NULL) {
        if (block_start_intercode(block)->kind == IR_FUNCTION) {
            graph = alloc_flowgraph();
            list_append(&graph->_graph, &graphs);
            graph->entry = block;
        }
        if (get_id_in_graph(graph, block) == -1) add2graph(graph, block);
        InterCode end = block_end_intercode(block);
        if (end->kind == IR_GOTO) {
            // 如果是GOTO，那么这个基本块的后继基本块就是这条指令的标号对应的基本块
            BasicBlock successor = find_label(basicBlocks, end->u.one.op);
            add_successor(block, successor);
            add_predecessor(successor, block);
        } else if (end->kind == IR_IFGOTO) {
            // 如果是IFGOTO，那么这个基本块的后继基本块就是这条指令的标号对应的基本块 + 下一个基本块
            BasicBlock successor = find_label(basicBlocks, end->u.ifgoto.z);
            add_successor(block, successor);
            add_predecessor(successor, block);
            if (block->next) {
                add_successor(block, block->next);
                add_predecessor(block->next, block);
            }
        } else if (end->kind == IR_RETURN) {
            // 如果是RETURN，那么这个基本块的后继基本块就是exit block（NULL）
            add_successor(block, graph->exit);
            add_predecessor(graph->exit, block);
        } else {
            // 其它的一般情况，后继基本块就是下一个基本块
            if (block->next) {
                add_successor(block, block->next);
                add_predecessor(block->next, block);
            }
        }
        block = block->next;
    }
    print_flow_graph(graph, "flow_graph.txt");
}

/// ======================= 支配关系相关 ====================///

/// @brief 计算基本块的支配集合（单个流图）
/// 流图中支配集合的计算
void compute_dominators_(FlowGraph graph) {
    // Initialization (dominator[B] = all)
    for (int i = 0; i < graph->blockCount; i++) {
        BasicBlock block = graph->blocks[i];
        if (block == graph->entry) continue;  // entry block的支配集合为自己
        block->dominators = create_bitset(graph->blockCount);
        block->subordinates = create_bitset(graph->blockCount);
        setall_bit(block->dominators);
    }

    // Boundary condition(Out[Entry] = {Entry})
    graph->entry->dominators = create_bitset(graph->blockCount);
    graph->entry->subordinates = create_bitset(graph->blockCount);
    set_bit(graph->entry->dominators, graph->entry->bid);
    set_bit(graph->entry->subordinates, graph->entry->bid);

#ifdef DEBUG_ON
    Debug("Before compute_dominators\n");
    for (int i = 0; i < graph->blockCount; i++) {
        BasicBlock block = graph->blocks[i];
        Debug("block %d: ", block->bid);
        print_bitset(block->dominators);
        print_bitset(block->subordinates);
    }
#endif

    bool changed = true;
    while (changed) {
        changed = false;
        for (int i = 0; i < graph->blockCount; i++) {
            BasicBlock block = graph->blocks[i];
            if (block == graph->entry) continue;
            Bitset new_dom = create_bitset(graph->blockCount);
            setall_bit(new_dom);
            FlowNode predecessor = NULL;
            for_each_in_list(predecessor, struct FlowNode_, node, &block->predecessors) {
                and_bitsets(new_dom, predecessor->block->dominators, new_dom);
            }
            set_bit(new_dom, block->bid);
            if (!bitset_equal(new_dom, block->dominators)) {
                changed = true;
                destory_bitset(block->dominators);
                block->dominators = new_dom;
            } else {
                destory_bitset(new_dom);
            }
        }
    }

    for (int i = 0; i < graph->blockCount; i++) {
        BasicBlock block = graph->blocks[i];
        for (int j = 0; j < graph->blockCount; j++) {
            BasicBlock otherBlock = graph->blocks[j];
            if (is_set(block->dominators, otherBlock->bid)) {  // 表示Other支配Block
                // 将block加入到otherBlock的subordinates中
                set_bit(otherBlock->subordinates, block->bid);
            }
        }
    }

#ifdef DEBUG_ON
    Debug("compute_dominators done\n");
    for (int i = 0; i < graph->blockCount; i++) {
        BasicBlock block = graph->blocks[i];
        Debug("block %d: ", block->bid);
        print_bitset(block->dominators);
        print_bitset(block->subordinates);
    }
#endif
}

/// @brief 计算基本块的支配集合(多个流图)
void compute_dominators() {
    FlowGraph graph = NULL;
    for_each_in_list(graph, struct FlowGraph_, _graph, &graphs) { compute_dominators_(graph); }
}
///======================== 深度优先遍历相关 ====================///

/// @brief 深度优先遍历 copy from 龙书 P 422
static void search_dfn(BasicBlock block, int* dfn, int* visited, int* count) {
    Panic_on(block == NULL, "block is NULL");
    if (visited[block->bid]) return;
    visited[block->bid] = 1;

    FlowNode successor = NULL;
    for_each_in_list(successor, struct FlowNode_, node, &block->successors) {
        search_dfn(successor->block, dfn, visited, count);
    }

    Panic_on(*count < 0, "count < 0");
    dfn[block->bid] = *count;
    *count = *count - 1;
}

/// @brief 深度优先遍历 （单个流图）
void compute_dfn_(FlowGraph graph) {
    int count = graph->blockCount - 1;
    BasicBlock block = graph->entry;
    int visited[MAX_BLOCKS_NR] = {0};
    search_dfn(block, &graph->dfn, visited, &count);

#ifdef DEBUG_ON
    Debug("compute_dfn done\n");
    for (int i = 0; i < graph->blockCount; i++) {
        Debug("block %d: dfn = %d\n", i, graph->dfn[i]);
    }
#endif
}

/// @brief 计算基本块的深度优先编号(多个流图)
void compute_dfn() {
    FlowGraph graph = NULL;
    for_each_in_list(graph, struct FlowGraph_, _graph, &graphs) { compute_dfn_(graph); }
}

///======================= 计算回边 ====================///

void add_backedge(FlowGraph graph, BasicBlock block, BasicBlock backedge) {
    Panic_on(block == NULL, "block is NULL");
    Panic_on(backedge == NULL, "backedge is NULL");
    BackEdge node = (BackEdge)malloc(sizeof(struct BackEdge_));
    node->from = block;
    node->to = backedge;

    list_append(&node->node, &graph->back_edges);
}

/// @brief 计算基本块的回边（单个流图）
void compute_backedges_(FlowGraph graph) {
    for (int i = 0; i < graph->blockCount; i++) {
        BasicBlock block = graph->blocks[i];
        FlowNode successor = NULL;
        for_each_in_list(successor, struct FlowNode_, node, &block->successors) {
            if (graph->dfn[successor->block->bid] > graph->dfn[block->bid]) {
                // 如果后继基本块的dfn大于当前基本块的dfn，那么这条边就是前向边
                continue;
            } else if (graph->dfn[successor->block->bid] < graph->dfn[block->bid]) {
                // 如果后继基本块的dfn小于当前基本块的dfn，那么这条边就是回边
                add_backedge(graph, block, successor->block);
            } else {
                // 如果后继基本块的dfn等于当前基本块的dfn，那么这条边就是横向边
                continue;
            }
        }
    }
#ifdef DEBUG_ON
    Debug("compute_backedge done\n");
    BackEdge edge = NULL;
    for_each_in_list(edge, struct BackEdge_, node, &graph->back_edges) {
        Debug("backedge: %d -> %d\n", edge->from->bid, edge->to->bid);
    }
#endif
}

void compute_backedges() {
    FlowGraph graph = NULL;
    for_each_in_list(graph, struct FlowGraph_, _graph, &graphs) { compute_backedges_(graph); }
}

///======================= 求出循环 ====================///

/// @brief 求出一个循环的出口基本块（bitset存储）
void compute_loop_exits(FlowGraph graph, Loop loop) {
    // 求出循环的出口基本块
    loop->exits = create_bitset(graph->blockCount);
    for (int i = 0; i < graph->blockCount; i++) {
        BasicBlock block = graph->blocks[i];
        if (is_set(loop->body, block->bid)) {
            // 如果是循环内的基本块
            // 枚举其所有的出边（successor），如果不是循环内部的节点，那么就是出口基本块
            FlowNode successor = NULL;
            for_each_in_list(successor, struct FlowNode_, node, &block->successors) {
                if (!is_set(loop->body, successor->block->bid)) {
                    // 如果successor不在循环内部，那么就是出口基本块
                    set_bit(loop->exits, block->bid);
                    break;
                }
            }
        }
    }
}

void dfs_loop(BasicBlock current, BasicBlock loop_head, Bitset loop, int* visited) {
    Panic_on(current == NULL, "current is NULL");
    if (visited[current->bid]) return;
    visited[current->bid] = 1;
    if (current == loop_head) {
        set_bit(loop, current->bid);
        return;
    }
    FlowNode predecessor = NULL;
    for_each_in_list(predecessor, struct FlowNode_, node, &current->predecessors) {
        dfs_loop(predecessor->block, loop_head, loop, visited);
    }
    set_bit(loop, current->bid);
}

static inline Loop alloc_loop() {
    Loop lp = (Loop)malloc(sizeof(struct Loop_));
    memset(lp, 0, sizeof(struct Loop_));
    return lp;
}

/// @brief Helper function for add a loop to graph
static inline void add_loop(FlowGraph graph, Loop lp) {
    Panic_on(lp == NULL, "add_loop");
    list_append(&lp->node, &graph->loops);
}

/// @brief 计算基本块的循环（单个流图）
void compute_loops_(FlowGraph graph) {
    BackEdge bge = NULL;
    int visited[MAX_BLOCKS_NR];
    // 每一条回边都是一个循环？？？
    for_each_in_list(bge, struct BackEdge_, node, &graph->back_edges) {
        BasicBlock header = bge->to;  // 循环头部
        BasicBlock current = bge->from;
        Bitset loop = create_bitset(graph->blockCount);
        set_bit(loop, header->bid);
        set_bit(loop, current->bid);
        memset(visited, 0, sizeof(visited));
        dfs_loop(current, header, loop, visited);
        Loop lp = alloc_loop();
        lp->body = loop;
        lp->header = header;
        compute_loop_exits(graph, lp);
        add_loop(graph, lp);
    }

#ifdef DEBUG_ON
    Debug("compute_loops done\n");
    Loop lp = NULL;
    for_each_in_list(lp, struct Loop_, node, &graph->loops) {
        Debug("loop: header = %d, body = ", lp->header->bid);
        print_bitset(lp->body);
        print_bitset(lp->exits);
    }
#endif
}

/// @brief 计算基本块的循环（多个流图）
void compute_loops() {
    FlowGraph graph = NULL;
    for_each_in_list(graph, struct FlowGraph_, _graph, &graphs) { compute_loops_(graph); }
}

///======================= 常量传播相关 ====================///
// Use for constant propagation
struct Value_ NAC_VALUE = {.kind = NAC, .value = 0};
struct Value_ UNDEF_VALUE = {.kind = UNDEF, .value = 0};

static inline Value make_undef_value() {
    Value value = (Value)malloc(sizeof(struct Value_));
    value->kind = UNDEF;
    value->value = 0;
    return value;
}

static inline Value make_nac_value() {
    Value value = (Value)malloc(sizeof(struct Value_));
    value->kind = NAC;
    value->value = 0;
    return value;
}

/// @brief 创建一个新的Value，常量类型
static inline Value make_const_value(int val) {
    Value value = (Value)malloc(sizeof(struct Value_));
    value->kind = CONST;
    value->value = val;
    return value;
}

/// @brief 深拷贝一个Value
static void dump_value(Value dest, const Value src) {
    if (dest == NULL || src == NULL) {
        Panic("dump_value");
    }
    dest->kind = src->kind;
    dest->value = src->value;
}

/// @brief 判断两个Value是否相等
static inline bool value_equal(Value v1, Value v2) {
    Panic_ON(v1 == NULL || v2 == NULL, "value_equal");
    if (v1->kind == CONST && v2->kind == CONST) {
        return v1->value == v2->value;
    } else if (v1->kind == NAC && v2->kind == NAC) {
        return true;
    } else if (v1->kind == UNDEF && v2->kind == UNDEF) {
        return true;
    }
    return false;
}

/// @brief meet function（用于控制流约束）
Value meet_value(Value v1, Value v2) {
    Panic_on(v1 == NULL || v2 == NULL, "meet value");
    if (v1->kind == CONST && v2->kind == CONST) {
        if (v1->value == v2->value) {
            return make_const_value(v1->value);
        } else {
            return make_nac_value();
        }
    } else if (v1->kind == NAC || v2->kind == NAC) {
        return make_nac_value();
    } else if (v1->kind == CONST && v2->kind == UNDEF) {
        return make_const_value(v1->value);
    } else if (v1->kind == UNDEF && v2->kind == CONST) {
        return make_const_value(v2->value);
    }
    return make_undef_value();
}

/// @brief 将v1中包含的变量的值赋给dest（Merge），arr_size是数组的大小.
// 这个函数用在 计算 IN[B] = ∪ out[P]，其中P是B的前驱基本块
void meet_into(Value* v1, Value* dest, int map_size) {
    if (v1 == NULL) return;
    for (int i = 0; i < map_size; i++) {
        dest[i] = meet_value(v1[i], dest[i]);
    }
}

/// @brief 创建一个新的Value数组，初始化为UNDEF
static inline Value* new_value_map(int size, int val) {
    Value* arr = (Value*)malloc(sizeof(Value) * size);
    for (int i = 0; i < size; i++) {
        if (val == UNDEF)
            arr[i] = make_undef_value();
        else if (val == NAC)
            arr[i] = make_nac_value();
        else {
            Panic("new_value_map");
        }
    }
    return arr;
}

/// @brief 打印一个Value数组（for debugging）:: 只打印常量
static void print_value_map(Value* map, int size) {
    for (int i = 0; i < FLOW_G_MAX_VAR; i++) {
        if (map[i]->kind == CONST) {
            Debug("v%d = %d", i, map[i]->value);
        }
    }
    for (int i = 0; i < FLOW_G_MAX_TEMP; i++) {
        if (map[i + FLOW_G_MAX_VAR]->kind == CONST) {
            Debug("t%d = %d", i, map[i + FLOW_G_MAX_VAR]->value);
        }
    }
}

/// @brief 释放一个Value数组
void free_value_map(Value* map, int size) {
    for (int i = 0; i < size; i++) {
        free(map[i]);
    }
    free(map);
}

/// @brief 给定一个map，将op转换为Value
Value get_op_value(Operand op, Value* map) {
    Panic_on(op == NULL, "op is NULL");
    Panic_on(map == NULL, "map is NULL");
    if (op->kind == OP_CONSTANT) {
        return make_const_value(op->u.value);
    } else if (op->kind == OP_VARIABLE) {
        return map[op->u.var_no];
    } else if (op->kind == OP_TEMP) {
        return map[FLOW_G_MAX_VAR + op->u.tmp.tmp_no];
    }
    Panic("unknown kind");
}

/// @brief 补丁函数，用于考虑*和&的情况
Value get_op_value2(Operand op, char* type, Value* map) {
    Panic_ON(op == NULL || type == NULL || map == NULL, "get_op_value2");
    Panic_on(op->kind != OP_VARIABLE && op->kind != OP_TEMP, "get_op_value2");
    int offset = op->kind == OP_VARIABLE ? op->u.var_no : op->u.tmp.tmp_no + FLOW_G_MAX_VAR;
    if (strcmp(type, "*") == 0) {
        return map[offset + FLOW_G_MAX_VAR + FLOW_G_MAX_TEMP];
    } else if (strcmp(type, "&") == 0) {
        return map[offset + (FLOW_G_MAX_VAR + FLOW_G_MAX_TEMP) * 2];
    }
    Panic("unknown type");
}

/// @brief 补丁函数，用于考虑*和&的情况
static inline void update_map2(Operand op, char* type, Value* map, Value* new) {
    Panic_ON(op == NULL || type == NULL || map == NULL || new == NULL, "update_map2");
    Panic_on(op->kind != OP_VARIABLE && op->kind != OP_TEMP, "update_map2");
    int offset = op->kind == OP_VARIABLE ? op->u.var_no : op->u.tmp.tmp_no + FLOW_G_MAX_VAR;
    if (strcmp(type, "*") == 0) {
        dump_value(map[offset + (FLOW_G_MAX_VAR + FLOW_G_MAX_TEMP)], new);
    } else if (strcmp(type, "&") == 0) {
        dump_value(map[offset + (FLOW_G_MAX_VAR + FLOW_G_MAX_TEMP) * 2], new);
    } else {
        Panic("update_map2");
    }
}

/// @brief 修改map中operand对应的value，更新为new(内容上的拷贝，不是指针的修改)
static inline void update_map(Operand op, Value* map, Value new) {
    Panic_on(op == NULL || map == NULL || new == NULL, "update_map");
    if (op->kind == OP_VARIABLE) {
        dump_value(map[op->u.var_no], new);
    } else if (op->kind == OP_TEMP) {
        dump_value(map[op->u.tmp.tmp_no + FLOW_G_MAX_VAR], new);
    } else {
        Panic("update_map");
    }
    update_map2(op, "*", map, &NAC_VALUE);  // 当op变化时，*op也会变化
}

/// @brief  条件表达式求值
Value evaluate_cond_exp(InterCode cond, Value* in) {
    Panic_on(cond == NULL || cond->kind != IR_IFGOTO, "evaluate_cond_exp");
    Value v1 = get_op_value(cond->u.ifgoto.x, in);
    Value v2 = get_op_value(cond->u.ifgoto.y, in);
    if (!strcmp(cond->u.ifgoto.relop, "==")) {
        if (v1->kind == CONST && v2->kind == CONST) {
            if (v1->value == v2->value) {
                return make_const_value(1);
            } else {
                return make_const_value(0);
            }
        }
    } else if (!strcmp(cond->u.ifgoto.relop, "!=")) {
        if (v1->kind == CONST && v2->kind == CONST) {
            if (v1->value != v2->value) {
                return make_const_value(1);
            } else {
                return make_const_value(0);
            }
        }
    } else if (!strcmp(cond->u.ifgoto.relop, ">")) {
        if (v1->kind == CONST && v2->kind == CONST) {
            if (v1->value > v2->value) {
                return make_const_value(1);
            } else {
                return make_const_value(0);
            }
        }
    } else if (!strcmp(cond->u.ifgoto.relop, "<")) {
        if (v1->kind == CONST && v2->kind == CONST) {
            if (v1->value < v2->value) {
                return make_const_value(1);
            } else {
                return make_const_value(0);
            }
        }
    } else if (!strcmp(cond->u.ifgoto.relop, ">=")) {
        if (v1->kind == CONST && v2->kind == CONST) {
            if (v1->value >= v2->value) {
                return make_const_value(1);
            } else {
                return make_const_value(0);
            }
        }
    } else if (!strcmp(cond->u.ifgoto.relop, "<=")) {
        if (v1->kind == CONST && v2->kind == CONST) {
            if (v1->value <= v2->value) {
                return make_const_value(1);
            } else {
                return make_const_value(0);
            }
        }
    }
    return make_nac_value();
}

/// @brief  二元表达式的求值函数
Value evaluate_bi_exp(InterCode exp, Value* in) {
    Value v1 = get_op_value(exp->u.binop.op1, in);
    Value v2 = get_op_value(exp->u.binop.op2, in);
    switch (exp->kind) {
        case IR_ADD: {
            if (v1->kind == CONST && v2->kind == CONST) {
                return make_const_value(v1->value + v2->value);
            } else if (v1->kind == UNDEF || v2->kind == UNDEF) {
                return &UNDEF_VALUE;
            }
            break;
        }
        case IR_SUB: {
            if (v1->kind == CONST && v2->kind == CONST) {
                return make_const_value(v1->value - v2->value);
            } else if (v1->kind == UNDEF || v2->kind == UNDEF) {
                return &UNDEF_VALUE;
            }
            break;
        }
        case IR_MUL: {
            if (v1->kind == CONST && v2->kind == CONST) {
                return make_const_value(v1->value * v2->value);
            } else if (v1->kind == CONST && v1->value == 0) {
                return make_const_value(0);
            } else if (v2->kind == CONST && v2->value == 0) {
                return make_const_value(0);
            } else if (v1->kind == UNDEF || v2->kind == UNDEF) {
                return &UNDEF_VALUE;
            }
            break;
        }
        case IR_DIV: {
            if (v1->kind == CONST && v2->kind == CONST) {
                if (v2->value == 0) return &UNDEF_VALUE;
                return make_const_value(v1->value / v2->value);
            } else if (v1->kind == CONST && v1->value == 0) {
                return make_const_value(0);
            } else if (v1->kind == UNDEF || v2->kind == UNDEF) {
                return &UNDEF_VALUE;
            }
            break;
        }
        default:
            Panic("Not binary kind!");
    }
    return &NAC_VALUE;
}

/// @brief 赋值表达式的求值函数
Value evaluate_ass_exp(InterCode ass, Value* in) {
    Value right = get_op_value(ass->u.assign.right, in);
    if (right->kind == CONST) {
        return right;
    } else if (right->kind == UNDEF) {
        return &UNDEF_VALUE;
    }
    return &NAC_VALUE;
}

/// @brief const propagation transfer function(常量传播的转移函数， 输入一个语句，更新map in)
//  in = f(code)
void cp_transfer_func(InterCode code, Value* in) {
    Panic_on(code == NULL, "code is NULL");

    switch (code->kind) {
        case IR_ASSIGN: {
            /**
             * 注意到 *t||*v 与 t||v在value map中对应不同的内容
             * 而因为&v和v在含义上保持一致，所以&v和v可以map到一致的内容
             */
            if (code->u.assign.ass_type == ASS_NORMAL) {  // x := y
                Value new_val = evaluate_ass_exp(code, in);
                if (!value_equal(new_val, get_op_value(code->u.assign.left, in)))
                    update_map(code->u.assign.left, in, new_val);
            } else if (code->u.assign.ass_type == ASS_GETADDR) {  // x := &y
                Value new_val = get_op_value(code->u.assign.right, in);
                if (!value_equal(new_val, get_op_value(code->u.assign.left, in))) {
                    update_map(code->u.assign.left, in, new_val);
                }
                break;
            } else if (code->u.assign.ass_type == ASS_GETVAL) {  // x := *y
                Value new_val = get_op_value2(code->u.assign.right, "*", in);
                if (!value_equal(new_val, get_op_value(code->u.assign.left, in))) {
                    update_map(code->u.assign.left, in, new_val);
                }
            } else if (code->u.assign.ass_type == ASS_SETVAL) {  // *x := y
                Value new_val = evaluate_ass_exp(code, in);
                if (!value_equal(new_val, get_op_value2(code->u.assign.left, "*", in))) {
                    update_map2(code->u.assign.left, "*", in, new_val);
                }
            }
            break;
        }
        case IR_ADD:
        case IR_SUB:
        case IR_MUL:
        case IR_DIV: {
            Value new_val = evaluate_bi_exp(code, in);
            if (!value_equal(new_val, get_op_value(code->u.binop.result, in)))
                update_map(code->u.binop.result, in, new_val);
            break;
        }
        case IR_ADDRADD: {
            Panic("Not imple");
            break;
        }
        case IR_CALL: {
            // 函数调用返回值不确定（比如 ret = f(); while() {ret = 1;}，此时ret = 1可以外提）
            Value new_val = &UNDEF_VALUE;
            if (!value_equal(new_val, get_op_value(code->u.two.left, in))) {
                update_map(code->u.two.left, in, new_val);
            }
            break;
        }
        case IR_READ: {
            Value new_val = &NAC_VALUE;  // 读入的值不是常量
            if (!value_equal(new_val, get_op_value(code->u.one.op, in))) {
                update_map(code->u.one.op, in, new_val);
            }
            break;
        }
        case IR_PARAM: {
            Value new_val = &NAC_VALUE;
            if (!value_equal(new_val, get_op_value(code->u.one.op, in))) {
                update_map(code->u.one.op, in, new_val);
            }
        }
        case IR_DEC:       // TODO: 思考这里需不需要加代码
        case IR_LABEL:     // Nothing to do
        case IR_FUNCTION:  // Nothing to do
        case IR_GOTO:      // Nothing to do
        case IR_IFGOTO:
        case IR_RETURN:
        case IR_WRITE:
        case IR_ARG: {
            break;
        }
        default:
            Panic("unknown kind");
    }
}

static inline Value* get_block_out(BasicBlock block) {
    Panic_on(block == NULL, "block is NULL");
    if (block->out == NULL) {
        block->out = new_value_map(FLOW_G_MAX_MAP, UNDEF);
    }
    return block->out;
}

/// @brief 当block的in和out都不再发生变化时，停止迭代，此时需要进行每一个block内部的常量传播
bool block_const_propagation(FlowGraph graph, BasicBlock block) {
    Panic_on(block == NULL, "block is NULL");
    Value* in_copy = (Value*)malloc(sizeof(Value) * FLOW_G_MAX_MAP);
    memcpy(in_copy, block->in, sizeof(Value) * FLOW_G_MAX_MAP);

    Instruction instruction = NULL;
    Instruction next = NULL;
    for (instruction = container_of(block->interCodes.next, struct Instruction_, node);
         &instruction->node != &block->interCodes; instruction = next) {
        InterCode code = instruction->code;
        next = container_of(instruction->node.next, struct Instruction_, node);  // 保存下一个节点的指针
        switch (instruction->code->kind) {
            case IR_LABEL:  // Nothing to do
                break;
            case IR_FUNCTION:  // Nothing to do
                break;
            case IR_ASSIGN: {
                if (code->u.assign.ass_type != ASS_NORMAL) break;
                Value right = get_op_value(instruction->code->u.assign.right, in_copy);
                if (right->kind == CONST && code->u.assign.right->kind != OP_CONSTANT) {
                    code->u.assign.right = new_const_op(right->value);
                    update_map(instruction->code->u.assign.left, in_copy, right);
                }
                break;
            }
            case IR_ADD:
            case IR_SUB:
            case IR_MUL:
            case IR_DIV: {
                Value new_val = evaluate_bi_exp(code, in_copy);
                if (new_val->kind == CONST) {
                    code->kind = IR_ASSIGN;
                    code->u.assign.left = code->u.binop.result;
                    code->u.assign.right = new_const_op(new_val->value);
                    code->u.assign.ass_type = ASS_NORMAL;
                    update_map(code->u.assign.left, in_copy, new_val);
                }
                break;
            }
            case IR_ADDRADD:
                break;
            case IR_GOTO:  // Nothing to do
                break;
            case IR_IFGOTO: {
                Value cond_val = evaluate_cond_exp(code, in_copy);
                if (cond_val->kind == CONST) {
                    BasicBlock go_to = find_label_in_flwgraph(graph, code->u.ifgoto.z);
                    if (cond_val->value == 0) {
                        del_instruction(block, instruction);
                        delete_node_in_flwgraph(graph, go_to);
                    }
                }
                break;
            }
            case IR_RETURN: {
                // Panic("Not implemented");
                break;
            }
            case IR_DEC:  // Nothing to do
                break;
            case IR_ARG: {
                // Panic("Not implemented");
                break;
            }
            case IR_CALL: {
                // Panic("Not implemented");
                break;
            }
            case IR_PARAM: {
                // Panic("Not implemented");
                break;
            }
            case IR_READ: {
                // Panic("Not implemented");
                break;
            }
            case IR_WRITE: {
                // Panic("Not implemented");
                break;
            }
            default:
                Panic("unknown kind");
        }
    }
}

bool transfer_block(FlowGraph graph, BasicBlock block, Value* in, Value* out) {
    // 检查in和out是否发生了变化
    Value* old_out = new_value_map(FLOW_G_MAX_MAP, UNDEF);
    for (int i = 0; i < FLOW_G_MAX_MAP; i++) {
        dump_value(old_out[i], out[i]);
    }

    for (int i = 0; i < FLOW_G_MAX_MAP; i++) {
        dump_value(out[i], in[i]);  // out[i] = in[i]
    }

    bool update = false;
    Instruction instruction = NULL;
    for_each_in_list(instruction, struct Instruction_, node, &block->interCodes) {
        cp_transfer_func(instruction->code, out);
    }
    for (int i = 0; i < FLOW_G_MAX_MAP; i++) {
        if (!value_equal(old_out[i], out[i])) {
            update = true;
            Debug("update block %d\n", block->bid);
            break;
        }
    }
    free_value_map(old_out, FLOW_G_MAX_MAP);
    return update;
}

/// @brief 从entry出发，删除所有不可达的基本块, dfs
static void dfs(FlowGraph graph, BasicBlock block, bool* visited) {
    if (graph->is_deleted[block->bid]) return;
    if (visited[block->bid]) return;
    visited[block->bid] = true;
    FlowNode successor = NULL;
    for_each_in_list(successor, struct FlowNode_, node, &block->successors) { dfs(graph, successor->block, visited); }
}

// 从entry出发，删除所有不可达的基本块, dfs
void flwg_raph_delete_dead_node(FlowGraph graph) {
    Panic_on(graph == NULL, "graph is NULL");
    BasicBlock block = graph->entry;
    bool* visited = (bool*)malloc(sizeof(bool) * graph->blockCount);
    memset(visited, 0, sizeof(bool) * graph->blockCount);
    dfs(graph, block, visited);
    for (int i = 0; i < graph->blockCount; i++) {
        if (!visited[i]) {
            delete_node_in_flwgraph(graph, graph->blocks[i]);
        }
    }
}

/// @brief 输入一个流图，进行常量传播
/// @param graph
static void global_const_propagation_(FlowGraph graph) {
    Panic_on(graph == NULL, "graph is NULL");
    BasicBlock block = graph->entry;

    bool go = true;
    while (go) {
        go = false;
        for (int i = 0; i < graph->blockCount; i++) {
            /// 从entry出发，按照dfn的顺序遍历每一个基本块 (TODO: 这里可以优化)
            int index = i;
            block = graph->blocks[index];
            if (graph->is_deleted[index]) continue;            // block is deleted
            Value* in = new_value_map(FLOW_G_MAX_MAP, UNDEF);  // It is not rigrious(for simplicity) ，这里需要仔细斟酌
            Value* out = get_block_out(block);
            FlowNode predecessor = NULL;
            for_each_in_list(predecessor, struct FlowNode_, node, &block->predecessors) {
                meet_into(predecessor->block->out, in, FLOW_G_MAX_MAP);
            }
            if (transfer_block(graph, block, in, out)) {
                go = true;
            }
            if (block->in != NULL) free_value_map(block->in, FLOW_G_MAX_MAP);  // free old in
            block->in = in;
            block->out = out;
        }
    }
#ifdef DEBUG_ON
    // 打印每一个block的in和out中为常量的变量以及值
    for (int i = 0; i < graph->blockCount; i++) {
        if (graph->is_deleted[i]) continue;
        block = graph->blocks[i];
        printf("======================\n");
        Debug("block %d", block->bid);
        Debug("in:");
        print_value_map(block->in, FLOW_G_MAX_MAP);
        Debug("out:");
        print_value_map(block->out, FLOW_G_MAX_MAP);
    }
#endif
    // 当每一个block的in和out都不再发生变化时，停止迭代，此时需要进行每一个block内部的常量传播
    for (int i = 0; i < graph->blockCount; i++) {
        if (graph->is_deleted[i]) continue;
        block = graph->blocks[i];
        block_const_propagation(graph, block);
    }

    // 从entry出发，删除所有不可达的基本块
    // flwg_raph_delete_dead_node(graph);
}

/// @brief 全局常量传播
void global_const_propagation() {
    FlowGraph graph = NULL;
    for_each_in_list(graph, struct FlowGraph_, _graph, &graphs) { global_const_propagation_(graph); }
}

/// @brief 将一个基本块的中间代码导出到文件
void block_export_code(BasicBlock block, FILE* fp) {
    Instruction instruction = NULL;
    for_each_in_list(instruction, struct Instruction_, node, &block->interCodes) {
        print_intercode(instruction->code, fp);
    }
}

/// @brief 将流图导出到文件
void flw_graph_export_code_(FlowGraph graph, FILE* fp) {
    for (int i = 0; i < graph->blockCount; i++) {
        if (graph->is_deleted[i]) continue;
        block_export_code(graph->blocks[i], fp);
    }
}

/// @brief 将所有流图导出到文件
void flw_graphs_export_code(FILE* fp) {
    FlowGraph graph = NULL;
    for_each_in_list(graph, struct FlowGraph_, _graph, &graphs) { flw_graph_export_code_(graph, fp); }
}

///======================= 循环优化相关 ====================///

/// @brief 在循环头前插入一个新的基本块，需要修改前驱后驱关系
BasicBlock new_pre2loop(FlowGraph graph, Loop loop) {
    // TODO: 待检查
    BasicBlock header = loop->header;
    BasicBlock new_insert = alloc_block();
    // 删除那些位于循环外的header的基本块与header之间的前驱关系
    // 使用安全删除模式：
    FlowNode predecessor = NULL;
    FlowNode next = NULL;
    for (predecessor = container_of(header->predecessors.next, struct FlowNode_, node);
         &predecessor->node != &header->predecessors; predecessor = next) {
        next = container_of(predecessor->node.next, struct FlowNode_, node);
        if (!is_set(loop->body, predecessor->block->bid)) {
            delete_successor(predecessor->block, header);
            delete_predecessor(header, predecessor->block);
            add_successor(predecessor->block, new_insert);
            add_predecessor(new_insert, predecessor->block);
        }
    }
    add_predecessor(header, new_insert);
    add_successor(new_insert, header);
    // 将pre添加到流图中
    insert2graph(graph, new_insert, header->bid);
    return new_insert;
}

/// 将基本块内变量的def和use情况记录到var_info中
void block_var_info(Lp_Var* var_info, BasicBlock block, FlowGraph graph) {
    Panic_on(var_info == NULL || block == NULL || graph == NULL, "block_var_info");
    Instruction instruction = NULL;
    Value* in_copy = (Value*)malloc(sizeof(Value) * FLOW_G_MAX_MAP);
    memcpy(in_copy, block->in, sizeof(Value) * FLOW_G_MAX_MAP);
    for_each_in_list(instruction, struct Instruction_, node, &block->interCodes) {
        InterCode code = instruction->code;
        switch (code->kind) {
            case IR_LABEL:  // Nothing to do
                break;
            case IR_FUNCTION:  // Nothing to do
                break;
            case IR_ASSIGN: {
                Operand left = code->u.assign.left, right = code->u.assign.right;
                Value rval = get_op_value(right, in_copy);
                int lidx = (left->kind == OP_VARIABLE) ? left->u.var_no : left->u.tmp.tmp_no + FLOW_G_MAX_VAR;
                int ridx = (right->kind == OP_VARIABLE) ? right->u.var_no : right->u.tmp.tmp_no + FLOW_G_MAX_VAR;
                Lp_Var lvar = var_info[lidx], rvar = var_info[ridx];
                set_bit(lvar->def, block->bid);
                set_bit(rvar->use, block->bid);
                if (rval->kind == CONST) {
                    // TODO: 这里会存在一些问题，需要仔细考虑
                    lvar->is_const = true;
                    lvar->instr = instruction;
                } else {
                    lvar->is_const = false;
                }
                break;
            }
        }
    }
}

/// @brief 给定一个循环，进行循环优化，接口独立，仅仅依赖于loop
/// 可移动代码需要满足以下特征：
/// （1）循环不变
/// （2）所处的基本快能够支配所有的出口基本块（循环内对外部有出边的基本块）
void loop_optimize_(FlowGraph graph, Loop loop) {
    Panic_on(graph == NULL || loop == NULL, "loop_optimize_");
    // 对于每一个v%d和t%d，需要知道其def和use的循环内的基本块，使用bitset记录
    Lp_Var* var_info = (Lp_Var*)malloc(sizeof(Lp_Var) * FLOW_G_MAX_MAP);
    for (int i = 0; i < FLOW_G_MAX_MAP; i++) {
        var_info[i] = (Lp_Var)malloc(sizeof(struct Lp_Var_));
        var_info[i]->def = create_bitset(graph->blockCount);
        var_info[i]->use = create_bitset(graph->blockCount);
        var_info[i]->is_const = false;
    }
    // 遍历循环内的每一个基本块，找到所有的循环不变量
    for (int i = 0; i < graph->blockCount; i++) {
        if (is_set(loop->body, i)) {
            BasicBlock block = graph->blocks[i];
            block_var_info(var_info, block, graph);
        }
    }
    for (int i = 0; i < FLOW_G_MAX_MAP; i++) {
        Lp_Var var = var_info[i];
        if (var->is_const) {
            print_bitset(var->def);
            int def_id = is_one_bitset(var->def);  // 如果def_id == -1，说明这个变量在循环内没有被定义或者被多次定义
            Debug("def_id = %d\n", def_id);
            if (def_id == -1) continue;
            BasicBlock def_block = graph->blocks[def_id];
            Debug("v%d的def基本块为%d\n", i, def_id);
            Debug("v%d的use基本块为：", i);
            print_bitset(var->use);
            Debug("循环出口基本块为：");
            print_bitset(loop->exits);
            // 条件1：def_block支配了所有的出口基本块 loop->exits \subset def_block->dominators
            // 条件2：def_block支配了所有使用该变量的基本块 var->use \subset def_block->dominators
            bool cond1 = true, cond2 = true;
            print_bitset(def_block->subordinates);
            cond1 = is_subset(loop->exits, def_block->subordinates);
            cond2 = is_subset(var->use, def_block->subordinates);
            if (cond1 && cond2) {
                Debug("找到循环不变量：v%d\n", i);
                // 将循环不变量的赋值语句移动到循环头之前
                BasicBlock pre = new_pre2loop(graph, loop);
                append_intercode(pre, var->instr->code);
                // 将循环不变量的赋值语句从循环内删除
                del_instruction(def_block, var->instr);
            }
        }
    }
}

void loop_optimize(FlowGraph graph) {
    Loop loop = NULL;
    for_each_in_list(loop, struct Loop_, node, &graph->loops) { loop_optimize_(graph, loop); }
}

/// @brief 循环优化
void loop_const_optimize() {
    FlowGraph graph = NULL;
    for_each_in_list(graph, struct FlowGraph_, _graph, &graphs) { loop_optimize(graph); }
}