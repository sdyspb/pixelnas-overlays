# PixelNAS Overlays for Firefly AIO-3568J

This repository contains Device Tree overlays and customization scripts for building an Armbian image tailored for the **PixelNAS** device based on the **Firefly AIO-3568J** board.

The Device Tree sources in this project are based on the **Firefly Station P2** (also known as `rk3568-roc-pc`) project. Station P2 is a single-board computer by Firefly based on the Rockchip RK3568 SoC, with mainline Linux kernel support added in 2023. This provides a solid foundation for enabling various hardware interfaces on the AIO-3568J platform.

These files enable various hardware interfaces and provide a custom SSH login banner. They are designed to be used with Armbian's build system, specifically via the `customize-image.sh` hook.

## File Descriptions

### Device Tree Overlays (`.dts` files)
Each overlay activates a specific hardware interface on the Firefly AIO-3568J. These overlays extend the base Station P2 device tree with PixelNAS-specific functionality. Place these files in the Armbian build overlay directory.

| File | Purpose |
|------|---------|
| `pixelnas-can.dts` | CAN bus interface |
| `pixelnas-dis.dts` | Disable unused nodes |
| `pixelnas-exp.dts` | GPIO expander |
| `pixelnas-gmacs.dts` | Gigabit Ethernet controllers (GMAC) |
| `pixelnas-gpio-butt.dts` | GPIO‑connected buttons |
| `pixelnas-hdmi.dts` | HDMI output |
| `pixelnas-i2c.dts` | I2C bus |
| `pixelnas-i2s.dts` | I2S audio interface |
| `pixelnas-pcie2.dts` | PCIe 2.0 controller |
| `pixelnas-pcie3.dts` | PCIe 3.0 controller |
| `pixelnas-sd.dts` | SD card interface |
| `pixelnas-spi.dts` | SPI bus |
| `pixelnas-uarts.dts` | UART ports |
| `pixelnas-usb.dts` | USB ports |

### Customization Script
- **`customize-image.sh`** – This script is executed automatically during the Armbian build process. It copies the overlays to the appropriate directories, enables them, and installs the custom SSH banner.

### SSH Login Banner
- **`10-header`** – A text file (e.g., an ASCII logo or welcome message) displayed when users log in via SSH. It is installed by `customize-image.sh` into `/etc/update-motd.d/`.

## How to Use These Files with Armbian Build

Follow the official Armbian [user customization guide](https://docs.armbian.com/Developer-Guide_User-Configurations/) for detailed instructions.

### File Placement Before Building

1. **Overlay `.dts` files**  
   Place **all `.dts` overlay files** in the `userpatches/overlay` directory of your Armbian build tree.

2. **`customize-image.sh`**  
   Place **`customize-image.sh`** in the root of the `userpatches` directory (i.e., `userpatches/customize-image.sh`). Make sure it is executable (`chmod +x`).

3. **`10-header` (SSH banner)** – you have two options:

**Option A (automatic copy via overlay structure)**  
Place the file maintaining the target filesystem path inside `userpatches/overlay/`:
```text
   userpatches/overlay/etc/update-motd.d/10-header
```
 Armbian will automatically copy the whole tree into the final image, so no extra commands are needed.

**Option B (explicit copy in `customize-image.sh`)**  
Place `10-header` directly in `userpatches/overlay/` (flat) and add a copy command to your `customize-image.sh`:

```bash
cp /tmp/overlay/10-header /etc/update-motd.d/
```
(The /tmp/overlay directory contains the contents of userpatches/overlay during the build.)

### Example Directory Structure (Option A)
```text
userpatches/
├── customize-image.sh
└── overlay/
    ├── pixelnas-can.dts
    ├── pixelnas-dis.dts
    ├── pixelnas-exp.dts
    ├── pixelnas-gmacs.dts
    ├── pixelnas-gpio-butt.dts
    ├── pixelnas-hdmi.dts
    ├── pixelnas-i2c.dts
    ├── pixelnas-i2s.dts
    ├── pixelnas-pcie2.dts
    ├── pixelnas-pcie3.dts
    ├── pixelnas-sd.dts
    ├── pixelnas-spi.dts
    ├── pixelnas-uarts.dts
    ├── pixelnas-usb.dts
    └── etc/
        └── update-motd.d/
            └── 10-header
```
Build Example
To build an Armbian image for the Firefly AIO-3568J including these overlays, run from the Armbian build root:

```bash
./compile.sh build BOARD=station-p2 BRANCH=current BUILD_DESKTOP=no BUILD_MINIMAL=no KERNEL_CONFIGURE=no RELEASE=trixie
```
The customize-image.sh script will be called automatically during the finalization stage and will apply all customizations.

### License
This project is licensed under GPL-2.0 (consistent with Armbian). The base device tree sources are derived from the mainline Linux kernel rk3568-roc-pc.dts.  
