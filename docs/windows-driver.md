# Windows driver install

The FPM enumerates as a standard USB device but reports
`USB\VID_045E&PID_02A9`, which the in-box Xbox 360 driver doesn't bind
automatically. You don't need to edit any `.inf` or disable driver-signature
enforcement — just **force-pick the stock Microsoft driver**, which is already
on every Windows 10/11 machine. This is the method that worked here, cleanly.

## Steps (the easy way — no file editing)

1. Connect the receiver's USB to the PC. In **Device Manager** it appears as an
   unknown / "Other" device (⚠️), Hardware ID `USB\VID_045E&PID_02A9`.
2. Right-click it → **Update driver**.
3. **Browse my computer for drivers**.
4. **Let me pick from a list of available drivers on my computer.**
5. If asked for a device category, choose **Xbox 360 Peripherals**
   (or just untick **Show compatible hardware** to see all).
6. Manufacturer **Microsoft** → model **Xbox 360 Wireless Receiver for
   Windows** → **Next**.
7. Accept the **"this driver may not be compatible / not recommended"**
   warning.

The device renames itself to **Xbox 360 Wireless Receiver for Windows** with no
error mark. That's it — the binding **persists across reboots**.

## Verify

1. Press the **sync button** on the adapter (Zero LED flashes blue; the module
   ring does the searching sweep).
2. Press the **sync button on the controller**.
3. The controller connects (its ring settles to a player quadrant).
4. Run **`joy.cpl`** (Win+R → `joy.cpl`) — the controller shows up; sticks and
   buttons register. Or just launch a game.

## Fallback — editing the `.inf` (only if force-pick isn't offered)

Some locked-down Windows setups don't expose the manual driver list. If so:

1. Copy `C:\Windows\System32\DriverStore\FileRepository\xusb22.inf_amd64_*\`
   to a writable folder, e.g. `C:\xbox360driver\`.
2. In `xusb22.inf`, search for `PID_0719`. For each line, add a duplicate below
   it with your PID, e.g.:
   ```
   %XUSB22.DeviceName%=XUSB22_Install, USB\VID_045E&PID_02A9
   ```
   (do it in both `NTamd64` and `NTx86` sections if present; change only the
   PID).
3. Editing the `.inf` breaks its signature, so **disable driver-signature
   enforcement** for the install: Settings → System → Recovery → Advanced
   startup → Restart now → Troubleshoot → Advanced options → Startup Settings →
   Restart → press **7**. Then Update driver → Browse → `C:\xbox360driver\`.

The force-pick method (above) avoids all of this and is strongly preferred.
