#ifndef __DELAY_H
#define __DELAY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* ★ 基于 DWT 周期计数器，不再使用 SysTick，
 *   因此在 FreeRTOS 任务/中断里调用也不会破坏系统节拍。 */
void Delay_Init(void);
void Delay_us(uint32_t us);
void Delay_ms(uint32_t ms);
void Delay_s(uint32_t s);

#ifdef __cplusplus
}
#endif

#endif
