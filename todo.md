Hopefully we can make minor changes to add support for
the LIS3DSH (cf LIS3DH) as a new subclass of the LIS3DHCore super class that already exists.

There's obvious key changes like the 3DSH uses different hardcoded I2C addresses. It looks like the current class setup can already handle that. The big question is how different the register subaddresses are between the 3DH and 3DSH. If they're all the same then I don't think we need to change much about the superclass, but if they're different it's a much bigger modification.

The other possible difference is settings. If there are settings that exist for the 3DSH but not the 3DH (or vice versa) then the Settings class might need to change.

Looking at the datasheets there are significant differences between the register layouts, and not just the 3dSh having extra registers that the 3dh doesn't have. Now, that's not necessarily a problem. At a glance, it looks like the code is nicely structured so LIS3DHCore only defines stuff like readRegister(subaddr) and then LIS3DH defines purposeful things like readAcceleration() which call e.g. readRegister(address).

I don't see any settings differences right off the bat. If there are we can easily make a new settings class because the 3dh subclass owns the settings, not the superclass. It's like whoever wrote this planned this all along.