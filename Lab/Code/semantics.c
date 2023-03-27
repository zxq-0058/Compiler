#include "semantics.h"

// 查找符号表中的符号
Symbol *lookup_symbol(SymbolTable *table, char *name) {
    // 从当前作用域向上查找
    while (table != NULL) {
        for (int i = 0; i < table->size; i++) {
            // TODO: 初步实现考虑符号表使用数组实现，后续可能会改成Hash
            Symbol *sym = table->symbols[i];
            if (sym != NULL && strcmp(sym->name, name) == 0) {
                return sym;
            }
        }
        table = table->prev;
    }
    return NULL; // 符号未定义
}

// 向符号表中添加符号
void add_symbol(SymbolTable *table, Symbol *sym) {
    // 将符号添加到当前作用域的符号表中
    // TODO: 符号表大小初步实现为动态申请（后续需要重新思考）
    if (table->size == 0) {
        table->symbols = (Symbol **) malloc(sizeof(Symbol *) * 16);
        table->size = 10;
    } else if (table->size == table->scope) {
        table->symbols = (Symbol **) realloc(table->symbols, sizeof(Symbol *) * (table->size + 16));
        table->size += 10;
    }
    table->symbols[table->scope - 1] = sym;
}

// 进入新的作用域
SymbolTable *enter_scope(SymbolTable *table) {
    SymbolTable *new_table = (SymbolTable *) malloc(sizeof(SymbolTable));
    new_table->scope = table->scope + 1;
    new_table->prev = table;
    new_table->next = NULL;
    new_table->size = 0;
    new_table->symbols = NULL;
    table->next = new_table;
    return new_table;
}

// 退出当前作用域
SymbolTable *exit_scope(SymbolTable *table) {
    SymbolTable *prev_table = table->prev;
    free(table->symbols);
    free(table);
    prev_table->next = NULL;
    return prev_table;
}
