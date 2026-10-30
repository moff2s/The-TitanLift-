# The Titan Lift

[ONE-LINE DESCRIPTION: what Titan Lift does]

Team 9 (Group 5) project for our C programming course. Written in C with a graphical interface built on GTK.

## Features

- [FEATURE 1]
- [FEATURE 2]
- [FEATURE 3]
- Graphical interface (`titanlift_gui`)
- Command-line version (`the_TitanLift`)

## Project Structure

| File | Purpose |
|------|---------|
| `the_TitanLift.c` | Main program logic |
| `titanlift_gui.c` | GUI version |
| `demo.c` | Demo program |
| `Team_9_Project/` | Full project folder |
| `Team_9_Code.c.zip` | Source code archive |
| `Team_9_Report.pdf` | Project report |
| `G5_T9_Project_Proposal.pdf` | Project proposal |
| `Team_9_Slide.pdf` / `.pptx` | Presentation slides |
| `Team_9_video_demo.mp4` | Video demo |
| `*.dll` | GTK runtime libraries needed to run on Windows |

## Run

1. Download or clone the repo.
2. Keep the `.exe` files in the same folder as the `.dll` files.
3. Double-click `titanlift_gui.exe` to open the GUI, or `the_TitanLift.exe` for the command-line version.

## Build from Source

Requires GCC and GTK 3 (for example via MSYS2).

```bash
gcc titanlift_gui.c -o titanlift_gui.exe `pkg-config --cflags --libs gtk+-3.0`
gcc the_TitanLift.c -o the_TitanLift.exe
```

## Team

- Choeun Linda
- Oeurn Meylih
