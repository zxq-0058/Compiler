#include "bitset.h"

/// @brief Create a bitset(initialize to 0)
/// @param size size of the bitset
Bitset create_bitset(size_t size) {
    Bitset bitset = (Bitset)malloc(sizeof(Bitset_));
    bitset->size = size;

    size_t arraySize = (size + sizeof(unsigned int) * 8 - 1) / (sizeof(unsigned int) * 8);

    bitset->data = (unsigned int *)calloc(arraySize, sizeof(unsigned int));

    // 清空
    for (size_t i = 0; i < arraySize; i++) {
        bitset->data[i] = 0;
    }
    return bitset;
}

/// @brief Set all bits in a bitset(1)
void setall_bit(Bitset bitset) {
    size_t arraySize = (bitset->size + sizeof(unsigned int) * 8 - 1) / (sizeof(unsigned int) * 8);

    for (size_t i = 0; i < arraySize; i++) {
        bitset->data[i] = ~0;
    }
}

/// @brief Copy a bitset
Bitset copy_bitset(Bitset bitset) {
    Bitset copy = (Bitset)malloc(sizeof(Bitset_));
    copy->size = bitset->size;

    size_t arraySize = (bitset->size + sizeof(unsigned int) * 8 - 1) / (sizeof(unsigned int) * 8);

    copy->data = (unsigned int *)calloc(arraySize, sizeof(unsigned int));

    for (size_t i = 0; i < arraySize; i++) {
        copy->data[i] = bitset->data[i];
    }

    return copy;
}

void destory_bitset(Bitset bitset) {
    free(bitset->data);
    free(bitset);
}

/// @brief Check if two bitsets are equal
bool bitset_equal(Bitset bitset1, Bitset bitset2) {
    if (bitset1->size != bitset2->size) {
        return false;
    }

    size_t arraySize = (bitset1->size + sizeof(unsigned int) * 8 - 1) / (sizeof(unsigned int) * 8);

    for (size_t i = 0; i < arraySize; i++) {
        if (bitset1->data[i] != bitset2->data[i]) {
            return false;
        }
    }

    return true;
}

/// @brief Set a bit in a bitset
void set_bit(Bitset bitset, size_t index) {
    if (index >= bitset->size) {
        printf("Error: Index out of range\n");
        return;
    }

    size_t arrayIndex = index / (sizeof(unsigned int) * 8);
    size_t bitOffset = index % (sizeof(unsigned int) * 8);

    bitset->data[arrayIndex] |= (1 << bitOffset);
}

void clear_bit(Bitset bitset, size_t index) {
    if (index >= bitset->size) {
        printf("Error: Index out of range\n");
        return;
    }

    size_t arrayIndex = index / (sizeof(unsigned int) * 8);
    size_t bitOffset = index % (sizeof(unsigned int) * 8);

    bitset->data[arrayIndex] &= ~(1 << bitOffset);
}

bool is_set(Bitset bitset, size_t index) {
    if (index >= bitset->size) {
        printf("Error: Index out of range\n");
        return false;
    }

    size_t arrayIndex = index / (sizeof(unsigned int) * 8);
    size_t bitOffset = index % (sizeof(unsigned int) * 8);

    return (bitset->data[arrayIndex] & (1 << bitOffset)) != 0;
}

void and_bitsets(Bitset result, Bitset bitset1, Bitset bitset2) {
    if (bitset1->size != bitset2->size || result->size != bitset1->size) {
        printf("Error: Bitset_ sizes do not match\n");
        return;
    }

    size_t arraySize = (result->size + sizeof(unsigned int) * 8 - 1) / (sizeof(unsigned int) * 8);

    for (size_t i = 0; i < arraySize; i++) {
        result->data[i] = bitset1->data[i] & bitset2->data[i];
    }
}

/// @brief result = result & bitset1
void bitset_and(Bitset result, Bitset bitset1) {
    size_t arraySize = (result->size + sizeof(unsigned int) * 8 - 1) / (sizeof(unsigned int) * 8);

    for (size_t i = 0; i < arraySize; i++) {
        result->data[i] = result->data[i] & bitset1->data[i];
    }
}

void or_bitsets(Bitset result, Bitset bitset1, Bitset bitset2) {
    if (bitset1->size != bitset2->size || result->size != bitset1->size) {
        printf("Error: Bitset_ sizes do not match\n");
        return;
    }

    size_t arraySize = (result->size + sizeof(unsigned int) * 8 - 1) / (sizeof(unsigned int) * 8);

    for (size_t i = 0; i < arraySize; i++) {
        result->data[i] = bitset1->data[i] | bitset2->data[i];
    }
}

int calculate_highest_bit_position(unsigned int value) {
    int position = 0;
    while (value > 1) {
        value >>= 1;
        position++;
    }
    return position;
}

int is_one_bitset(Bitset bitset) {
    int ret = -1;
    for (int i = 0; i < bitset->size; i++) {
        if (is_set(bitset, i)) {
            if (ret == -1) {
                ret = i;
            } else {
                return -1;
            }
        }
    }
    return ret;
}

void subtract_bitsets(Bitset result, Bitset bitset1, Bitset bitset2) {
    if (bitset1->size != bitset2->size || result->size != bitset1->size) {
        printf("Error: Bitset_ sizes do not match\n");
        return;
    }

    size_t arraySize = (result->size + sizeof(unsigned int) * 8 - 1) / (sizeof(unsigned int) * 8);

    for (size_t i = 0; i < arraySize; i++) {
        result->data[i] = bitset1->data[i] & (~bitset2->data[i]);
    }
}

int is_subset(Bitset bitset1, Bitset bitset2) {
    if (bitset1->size != bitset2->size) {
        printf("Error: Bitset_ sizes do not match\n");
        return -1;
    }

    size_t arraySize = (bitset1->size + sizeof(unsigned int) * 8 - 1) / (sizeof(unsigned int) * 8);

    for (size_t i = 0; i < arraySize; i++) {
        if ((bitset1->data[i] & bitset2->data[i]) != bitset1->data[i]) {
            return 0;
        }
    }
    return 1;
}

void print_bitset(Bitset bitset) {
    for (size_t i = 0; i < bitset->size; i++) {
        if (is_set(bitset, i)) {
            printf("1");
        } else {
            printf("0");
        }
    }
    printf("\n");
}
