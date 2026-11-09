#include "SckWatchdog.h"

SckWatchdog wdt;

// Survives a WDT reset (not zeroed by the startup code). The bootloader may
// overwrite it, so it is only trusted if the magic matches.
#define SCK_WDT_HANG_MAGIC 0x57445448  // "WDTH"
#define SCK_FAULT_MAGIC 0x46415554     // "FAUT"
struct NoInitHang { uint32_t magic; uint32_t pc; uint32_t lr; uint32_t check; };
static NoInitHang noInitHang __attribute__((section(".noinit")));

// The stacked exception frame (r0, r1, r2, r3, r12, lr, pc, xpsr) tells us
// where the code was running when the Early Warning (or a HardFault) happened.
extern "C" void WDT_HandlerC(uint32_t *stackFrame)
{
    wdt.earlyWarning(stackFrame);
}
// Gets the stack frame of the interrupted code and calls handlerC with it
#define SCK_STACKFRAME_TRAMPOLINE(handlerC) \
    __asm volatile ( \
        "movs r0, #4        \n" \
        "mov  r1, lr        \n" \
        "tst  r0, r1        \n" \
        "beq  1f            \n" \
        "mrs  r0, psp       \n" \
        "b    2f            \n" \
        "1:                 \n" \
        "mrs  r0, msp       \n" \
        "2:                 \n" \
        "ldr  r1, =" #handlerC " \n" \
        "bx   r1            \n" \
    )
extern "C" __attribute__((naked)) void WDT_Handler(void)
{
    SCK_STACKFRAME_TRAMPOLINE(WDT_HandlerC);
}
extern "C" void HardFault_HandlerC(uint32_t *stackFrame)
{
    wdt.hardFault(stackFrame);
}
// Instead of hanging forever (Arduino default) save the crash location and reset
extern "C" __attribute__((naked)) void HardFault_Handler(void)
{
    SCK_STACKFRAME_TRAMPOLINE(HardFault_HandlerC);
}

void SckWatchdog::setup()
{
    // Recover info from a previous WDT reset
    if (noInitHang.check == (noInitHang.pc ^ noInitHang.lr)) {
        if ((PM->RCAUSE.reg & PM_RCAUSE_WDT) && noInitHang.magic == SCK_WDT_HANG_MAGIC) _lastHang = { true, HANG_WDT, noInitHang.pc, noInitHang.lr };
        else if ((PM->RCAUSE.reg & PM_RCAUSE_SYST) && noInitHang.magic == SCK_FAULT_MAGIC) _lastHang = { true, HANG_FAULT, noInitHang.pc, noInitHang.lr };
    }
    noInitHang.magic = 0;

    // GCLK5 = OSCULP32K / 32 = 1024 Hz (DIVSEL: divide by 2^(DIV+1))
    GCLK->GENDIV.reg = GCLK_GENDIV_ID(SCK_WDT_GCLK) | GCLK_GENDIV_DIV(4);
    while (GCLK->STATUS.bit.SYNCBUSY);
    GCLK->GENCTRL.reg = GCLK_GENCTRL_ID(SCK_WDT_GCLK) | GCLK_GENCTRL_GENEN | GCLK_GENCTRL_SRC_OSCULP32K | GCLK_GENCTRL_DIVSEL;
    while (GCLK->STATUS.bit.SYNCBUSY);

    // WDT clock = GCLK5
    GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID_WDT | GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN(SCK_WDT_GCLK);
    while (GCLK->STATUS.bit.SYNCBUSY);

    disable();

    WDT->CONFIG.reg = WDT_CONFIG_PER_16K;              // 16384 cycles -> 16 seconds
    WDT->EWCTRL.reg = WDT_EWCTRL_EWOFFSET_8K;          // 8192 cycles -> 8 seconds
    _sync();
    WDT->INTFLAG.reg = WDT_INTFLAG_EW;
    WDT->INTENSET.reg = WDT_INTENSET_EW;

    // Lowest priority: if some other interrupt hangs this one will not run and the hardware WDT will reset us
    NVIC_ClearPendingIRQ(WDT_IRQn);
    NVIC_SetPriority(WDT_IRQn, 3);
    NVIC_EnableIRQ(WDT_IRQn);

    enable();
}
void SckWatchdog::enable()
{
    feed();
    WDT->CLEAR.reg = WDT_CLEAR_CLEAR_KEY;
    _sync();
    WDT->CTRL.reg = WDT_CTRL_ENABLE;
    _sync();
    _enabled = true;
}
void SckWatchdog::disable()
{
    WDT->CTRL.reg = 0;
    _sync();
    _enabled = false;
}
void SckWatchdog::feed()
{
    // The Early Warning interrupt also uses _ewCounter: don't let it run between the check and the reset
    uint32_t primask = __get_PRIMASK();
    __disable_irq();

    // We came back just after the last Early Warning (hardware WDT was left without clearing)
    if (_ewCounter >= _ewLimit) {
        _sync();
        WDT->CLEAR.reg = WDT_CLEAR_CLEAR_KEY;
        noInitHang.magic = 0;
    }
    _ewCounter = 0;

    __set_PRIMASK(primask);
    lastFeed = millis();
}
void SckWatchdog::suspend()
{
    _suspended = true;
}
void SckWatchdog::resume()
{
    feed();
    _suspended = false;
}
bool SckWatchdog::enabled()
{
    return _enabled;
}
bool SckWatchdog::suspended()
{
    return _suspended;
}
SckWatchdog::HangInfo SckWatchdog::lastHang()
{
    return _lastHang;
}
void SckWatchdog::earlyWarning(uint32_t *stackFrame)
{
    WDT->INTFLAG.reg = WDT_INTFLAG_EW;

    if (!_suspended) _ewCounter++;

    if (_ewCounter < _ewLimit) {
        // Main loop is still alive (or suspended on purpose): re-arm the hardware WDT
        _sync();
        WDT->CLEAR.reg = WDT_CLEAR_CLEAR_KEY;
    } else {
        // Don't clear: the hardware WDT will reset the kit in SCK_WDT_EW_PERIOD_S seconds.
        // Save where the code was stuck so we can report it after reset.
        noInitHang.pc = stackFrame[6];
        noInitHang.lr = stackFrame[5];
        noInitHang.check = noInitHang.pc ^ noInitHang.lr;
        noInitHang.magic = SCK_WDT_HANG_MAGIC;
    }
}
void SckWatchdog::hardFault(uint32_t *stackFrame)
{
    noInitHang.pc = stackFrame[6];
    noInitHang.lr = stackFrame[5];
    noInitHang.check = noInitHang.pc ^ noInitHang.lr;
    noInitHang.magic = SCK_FAULT_MAGIC;
    NVIC_SystemReset();
}
void SckWatchdog::_sync()
{
    while (WDT->STATUS.bit.SYNCBUSY);
}
