#ifndef _DRIVER_DUMMYETH_H_
#define _DRIVER_DUMMYETH_H_

#include "stdint.h"
#include "stdbool.h"
#include "eth.h"

extern const eth_ops_t dummyeth_ops;
void init_dummyeth();

#endif
