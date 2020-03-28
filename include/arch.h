#ifndef _ARCH_H_
#define _ARCH_H_

#include "stdint.h"

void init_arch();
void init_arch_dev(int hartid, void *dtb);

void arch_call_user(void *pt, uintptr_t pc, uintptr_t sp);

extern uintptr_t user_start_time;
extern uintptr_t user_end_time;

#include "task.h"
extern user_task_t user_task;

#endif
