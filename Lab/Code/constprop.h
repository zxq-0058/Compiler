#ifndef _CONSTPROP_H_
#define _CONSTPROP_H_

#include "basicblock.h"
#include "intercode.h"

void const_propagate(BasicBlock block);

#endif  // _CONSTPROP_H_