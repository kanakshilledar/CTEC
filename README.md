# CTEC - Controlled Temperature Experiments Chamber

This is the repository for the CTEC chamber. More data will be added progressively.

### Note
We will try to follow the below directory structure.

```console
CTEC/
├── README.md
├── LICENSE
├── CHANGELOG.md
│
├── docs/
│   ├── architecture/
│   ├── requirements/
│   ├── design/
│   ├── protocols/
│   └── bringup.md
│
├── hardware/
│   ├── schematic/
│   │   ├── main/
│   │   └── power/
│   ├── pcb/
│   │   ├── main/
│   │   └── footprints/
│   ├── libraries/
│   │   ├── symbols/
│   │   └── footprints/
│   ├── 3d/
│   └── manufacturing/
│       ├── gerbers/
│       ├── drill/
│       ├── bom/
│       └── pick-and-place/
│
├── firmware/
│   ├── bootloader/
│   ├── application/
│   ├── drivers/
│   ├── boards/
│   ├── tests/
│   └── CMakeLists.txt
│
├── software/
│   ├── desktop/
│   ├── mobile/
│   ├── cli/
│   └── tools/
│
├── protocols/
│   ├── serial/
│   ├── usb/
│   └── wireless/
│
├── tests/
│   ├── hardware/
│   ├── firmware/
│   └── integration/
│
├── scripts/
│   ├── build/
│   ├── flash/
│   ├── test/
│   └── release/
│
├── config/
│
└── .github/
    └── workflows/
```
