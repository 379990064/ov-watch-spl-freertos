#include "main.h"

extern volatile uint32_t TimingDelay;
void TimingDelay_Decrement(void);
void Delay_ms(uint32_t nTime);

volatile uint32_t TimingDelay = 0;

void TimingDelay_Decrement(void)
{
    if (TimingDelay != 0x00)
    {
        TimingDelay--;
    }
}

int main(void)
{

}


