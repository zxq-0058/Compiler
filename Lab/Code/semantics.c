#include "semantics.h"
#include "logger.h"

// 计算符号名字的哈希值
static unsigned int hash_symbol_name(const char *name)
{
    unsigned int hash = 0, i;
    for (; *name; ++name)
    {
        hash = (hash << 2) + *name;
        if (i = hash & ~0x3fff)
            hash = (hash ^ (i >> 12)) & 0x3fff;
    }
    return hash;
    // 后续拿到hash值，需要对哈希表大小mod
}

// 查找符号表中的符号
Symbol *lookup_symbol(SymbolTable *table, char *name)
{
    unsigned int index = hash_symbol_name(name) % table->size;
    SymbolTableEntry *entry = table->table[index]; // 这里应该是一个链表
    // TODO: 考虑作用链的实现
    while (entry != NULL)
    {
        Symbol *sym = entry->symbol;
        if (strcmp(sym->name, name) == 0)
        {
            return sym;
        }
        entry = entry->next;
    }
    return NULL; // 符号未定义
}

// 向符号表中添加符号
void add_symbol(SymbolTable *table, Symbol *sym)
{
    unsigned int index = hash_symbol_name(sym->name) % table->size;
    // 链表插入过程
    SymbolTableEntry *entry = (SymbolTableEntry *)malloc(sizeof(SymbolTableEntry));
    entry->symbol = sym;
    entry->next = table->table[index];
    table->table[index] = entry;
}

// 进入新的作用域
SymbolTable *enter_scope(SymbolTable *table)
{
    SymbolTable *new_table = (SymbolTable *)malloc(sizeof(SymbolTable));
    new_table->scope = table->scope + 1;
    new_table->prev = table;
    new_table->next = NULL;
    new_table->size = table->size;
    new_table->table = (SymbolTableEntry **)calloc(table->size, sizeof(SymbolTableEntry *));
    table->next = new_table;
    return new_table;
}

// 退出当前作用域
SymbolTable *exit_scope(SymbolTable *table)
{
    SymbolTable *prev_table = table->prev;
    for (int i = 0; i < table->size; i++)
    {
        SymbolTableEntry *entry = table->table[i];
        while (entry != NULL)
        {
            SymbolTableEntry *next = entry->next;
            free(entry);
            entry = next;
        }
    }
    free(table->table);
    free(table);
    prev_table->next = NULL;
    return prev_table;
}

// 初始化符号表
SymbolTable *init_symbol_table(int size)
{
    SymbolTable *table = (SymbolTable *)malloc(sizeof(SymbolTable));
    table->scope = 0;
    table->prev = NULL;
    table->next = NULL;
    table->size = size;
    table->table = (SymbolTableEntry **)calloc(size, sizeof(SymbolTableEntry *));
    return table;
}

// 释放符号表
void free_symbol_table(SymbolTable *table)
{
    while (table != NULL)
    {
        SymbolTable *next_table = table->next;
        exit_scope(table);
        table = next_table;
    }
}
