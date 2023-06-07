#ifndef _BITSET_H_
#define _BITSET_H_  // _BITSET_H_ is a macro

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct Bitset_ {
    unsigned int *data;
    size_t size;
} Bitset_;
// Bitset_ is a struct that contains an array of unsigned ints and the size of the bitset
typedef struct Bitset_ *Bitset;

/// @brief Create a bitset
/// @param size size of the bitset
Bitset create_bitset(size_t size);
/// @brief Set all bits in a bitset(1)
void setall_bit(Bitset bitset);
/// @brief Copy a bitset
Bitset copy_bitset(Bitset bitset);
void destory_bitset(Bitset bitset);
/// @brief Check if two bitsets are equal
bool bitset_equal(Bitset bitset1, Bitset bitset2);
/// @brief Set a bit in a bitset
void set_bit(Bitset bitset, size_t index);
void clear_bit(Bitset bitset, size_t index);
bool is_set(Bitset bitset, size_t index);
void and_bitsets(Bitset result, Bitset bitset1, Bitset bitset2);
/// @brief result = result & bitset1
void bitset_and(Bitset result, Bitset bitset1);
void or_bitsets(Bitset result, Bitset bitset1, Bitset bitset2);
void subtract_bitsets(Bitset result, Bitset bitset1, Bitset bitset2);
// 判断是不是只有一个bit为1，如果是，返回这个bit的位置，否则返回-1
int is_one_bitset(Bitset bitset);
/// @brief Check if bitset1 is a subset of bitset2
int is_subset(Bitset bitset1, Bitset bitset2);
void print_bitset(Bitset bitset);

#endif