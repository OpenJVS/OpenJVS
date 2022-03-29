[![Actions Status](https://github.com/openjvs/openjvs/workflows/Build/badge.svg)](https://github.com/openjvs/openjvs/actions)

# OpenJVS

OpenJVS is an emulator for I/O boards in arcade machines that use the JVS protocol. It requires a USB RS485 converter, or an official OpenJVS HAT.

The following arcade boards are supported:

- Naomi 1/2
- Triforce
- Chihiro
- Hikaru
- Lindbergh
- Ringedge 1/2
- Namco System 22/23
- Namco System 2x6
- Taito Type X+
- Taito Type X2
- exA-Arcadia

Questions can be asked in the discord channel: https://arcade.community. If it asks you to create an account, you can simply click anywhere away from the box  and it'll let you in!

## Installation

Installation is done from the git repository as follows:

```
sudo apt install build-essential cmake git file
git clone https://github.com/openjvs/openjvs
make
sudo make install
```

## Guides

- [Manual & Detailed Hat Guide](docs/OpenJVS_IO_Manual_1.2.pdf)
- [Software Guide](docs/guide.md) 
- [Hat Quickstart Guide](docs/hat-quickstart.md)


