---
title: Installing
nav_order: 2
---

# Installing

RMX-0 installs with the module's own updater, the same way as an official update. The updater itself is never changed, so a failed or interrupted update can be repeated, and you can always go back to the stock firmware.

1. Download `LXRE_RMX0_V<version>.img` from the [release notes](release-notes.md) or the repository's Releases page.
2. Copy it to the top folder of a FAT32 SD card. Delete any other `LXRE*.img` files from the card.
3. With the module powered off, insert the card. Hold the encoder button and power on. Keep holding until **LXR Updater** appears.
4. Wait for **Update OK!**. The module restarts and shows the RMX-0 logo with its version.

The [restart menu](other-changes.md#restart-menu) can also enter the updater without powering down the rack.

## Before you update

- **Samples are erased** by every firmware update, including going back to stock. [Reinstall them](samples.md#installing) from the card afterwards.
- Kits, projects and global settings on the SD card are not touched.

## Going back to the stock firmware

Download the official update from Erica Synths and install it the same way. Kits saved on RMX-0 still load; their RMX-0 settings are ignored.
