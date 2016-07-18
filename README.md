RK312x Userspace Watchdog Driver
================================

This repo contains a program that, when run as root, will enable the
RK312x watchdog timer to reset in 28 seconds.

It will then reset the watchdog timer every 15 seconds.

If you terminate the program, the board will reboot itself within 28 seconds.


About the Chip
--------------

The RK3126, which this was developed on, is a LQFP version of the RK3128.
Aside from the package difference, and the lack of pins on the LQFP and TFBGA,
the RK3126 and RK3128 are identical chips.

Therefore, this information should also apply to both the RK3126 and RK3128 chips.


Watchdog Timer Clocking
------------------------

The watchdog timer has its own clock, managed by the Clocking and Reset Unit (CRU).
It must be ungated before using the watchdog timer.

The lower 16 bits of any CRU register reflect the status of clock gating.  The upper
16 bits of any CRU register indicate whether a write updates a bit or not.

To ungate the clock to the watchdog timer, we want to clear bit 15 of CRU_CLKGATE7_CON.
Therefore, we need to set bit 31 to indicate that we want to update bit 15, and we must
clear bit 15.  As a result, write 0x80000000 to CRU_CLKGATE7_CON.

Upon doing this, the WDT block should be accessible.


Watchdog Timer Operation
------------------------

The timer is clocked by a 74.25 MHz clock.  Every tick, the Current Counter Value Register
(CCVR) will decrement by 1.  When the value reaches 0, the watchdog will fire.

When the watchdog fires, it can behave in one of two ways.  The first method is for it to
reboot the system right away.  This is the default value when resp_mode is set to 0.
The second method is for it to raise an interrupt and reset the counter value register.
If the watchdog fires again without the interrupt being cleared, the system is rebooted.

Note that, by default, the interrupt is asserted, so the first operation should be
to clear the watchdog.


Clearning the Watchdog
----------------------

Clearing the watchdog timer simply involves writing the value "0x76" to WDT_CRR.  You must
do this prior to enabling the timer, and periodically to prevent the system from rebooting.


Setting the Timeout
-------------------
There are 16 possible timeout periods, settable by writing values to WDT_TORR.  Values are
between 0xffff and 0x7fffffff ticks of the 74.25 MHz clock.  This will give you a reset
period of one of the following:

 * 880 microseconds
 * 1.7 milliseconds
 * 3.5 milliseconds
 * 7 milliseconds
 * 14 milliseconds
 * 28 milliseconds
 * 56 milliseconds
 * 113 milliseconds
 * 225 milliseconds
 * 452 milliseconds
 * 900 milliseconds
 * 1.8 seconds
 * 3.6 seconds
 * 7.2 seconds
 * 14.5 seconds
 * 28.9 seconds

You can effectively get a watchdog period of 57.8 seconds by enabling resp_mode, but this
is not an advisable solution.


Checking the timer
------------------

The amount of time left before the watchdog fires can be read in WDT_CCVR.


No Standard Lib
---------------

This code was written to be self-contained.  As such, it does not link against libc.

This was done because it was developed on a regular Linux machine, without libbionic.  By
implementing our own syscalls (in syscalls.S), we can significantly reduce the binary size.
