Hopefully we can make minor changes to add support for
the LIS3DSH (cf LIS3DH) as a new subclass of the LIS3DHCore super class that already exists.

There's obvious key changes like the 3DSH uses different hardcoded I2C addresses. It looks like the current class setup can already handle that. The big question is how different the register subaddresses are between the 3DH and 3DSH. If they're all the same then I don't think we need to change much about the superclass, but if they're different it's a much bigger modification.

The other possible difference is settings. If there are settings that exist for the 3DSH but not the 3DH (or vice versa) then the Settings class might need to change.

Looking at the datasheets there are significant differences between the register layouts, and not just the 3dSh having extra registers that the 3dh doesn't have. Now, that's not necessarily a problem. At a glance, it looks like the code is nicely structured so LIS3DHCore only defines stuff like readRegister(subaddr) and then LIS3DH defines purposeful things like readAcceleration() which call e.g. readRegister(address).

I don't see any settings differences right off the bat. If there are we can easily make a new settings class because the 3dh subclass owns the settings, not the superclass. It's like whoever wrote this planned this all along.

applySettins for 3dSh
Which registers look like they have settings

STAT looks like it's mostly status flags for reading
but its INT_SM1 and INT_SM2 bits look kind of like option flags
    

VFC_X? Is that for setting or reading? Vector coefficients for diff filter
    Vector filter can be used before input to SM2
    These sete the coefficients in the 7th order FIR filter

THRSX Threshold value register
    For use in certain SM event opcodes

CTRL_REG4
    ODR output data rate and power mode selection, bits 3:0
        ODR3, ODR2, ODR1, ODR0
    BDU block data update
    
    Zen, Yen, Xen
        Axis enabling

CTRL_REG1
    Hysteresis value to be added or subtracted from threshold in SM1 (what are the SMs?)

    SM1 pin to use
    SM1 enable

CTRL_REG2
    Same but for SM2

CTRL_REG3
    DR_EN enable Data ready signal
    IEA: whether interrupt should be high or low for signally
    IEL: whther interrupt latches or pulses
    INT2_EN enable INT pins
    INT1_EN
    VFILT enable vector filter
    STRT: does a soft reset

CTRL_REG5
    Anti-aliasing filter bandwidth setting
    FSCALE select the scale
    ST2: enable self test mode
    SIM: select SPI wire mode

CTRL_REG6
    force reboot
    FIFO enable
    WTM_EN -- limits FIFO to watermark value
    ADD_INC automatically increment register address during multiple byte access
    P1_EMPTY FIFO empty indication
    P1_WTM FIFO watermark interrupt on/off
    P1_OVERRUN FIFO overrun interrupt on/off
    P2_BOOT put boot interrupt on Int2

STATUS
    Data availability etc

FIFO_CTRL
    FMODE 3 bits
    WTMP: 5 bits -- the number of data samples that triggers the watermark interrupt. FIFO depth is limited to 32

FIFO_SRC
    watermark status markers
    WTM currently above or below status
    OVRN_FIFO currently overrun or no
    EMPTY if FIFO empty
    FSS 5 bits for current number of data points in FIFO

STx_1
    State machine. Is this the SM1? What does the state machine do?
    Oh we can program it here?
    Yes

TIMX I don't think we set these? I don't think we even read them, they're for the SMs

THRS also for SM operation
    Can be set for use in programming SMs

MASK1_X axis and sign mask for SMs
    Can be set for use in programming SMs

SETT1
    Settings of threshold peak detection and flags for SM1

PR1
    More SM1 settings



Okay
THRS

THe settings paradigm in this library isn't super efficient. If you want to change a single setting you have two write to all the settings registers. Of course, that's not terrible given that there are only a handful. In theory we could make it more efficient from a write perspective, but do we really need to? No.

Not working
LIS3DSH_CTRL_REG4: 0x67
LIS3DSH_CTRL_REG5: 0xC0
LIS3DSH_CTRL_REG6: 0x0 --> should be 0x10
LIS3DSH_FIFO_CTRL: 0x14 --> Should be 0x1F? This one shouldn't matter. It's watermark = 14 vs watermark = 31. Everyhing else the same.

Working
...
LIS3DSH_CTRL_REG4: 0x67
...
LIS3DSH_CTRL_REG3: 0x18
LIS3DSH_CTRL_REG5: 0xC0
LIS3DSH_CTRL_REG6: 0x10
LIS3DSH_FIFO_CTRL: 0x1F

So somehow I got Reg4 and Reg5 right but reg6 and fifo_ctrl wrong.

Reg6 needed to have autoincrement set to work as expected. I'm going to move that oout of applySettings and put it in begin() because we don't have a setting for that and we don't want to unexpectedly overwrite a user trying to change the setting.

I'm also going to make applySettings read the settings
first in case there are any hidden extra bits the user changes later.