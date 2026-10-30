# The TitanLift

A smart elevator system built in C for a 10-floor building, with a GTK graphical interface. It gives VIP cardholders temporary priority without permanently ranking one group above another.

Inspired by the RMS Titanic, which had separate elevators for First Class and everyone else. TitanLift reimagines that as a fair, temporary access system.

**Course:** Algorithm & Computational Thinking 2, Cambodia Academy of Digital Technology (CADT)
**Lecturer:** Han Leangsiv
**Group 5, Team 9**

## The Building

- **Floors 1-8:** open to everyone
- **Floor 9:** VIP floor, authorized cardholders only
- **Floor 10:** rooftop sky bar

## Features

- **Priority request queue:** stores passenger requests and processes the highest priority first
- **VIP Express Mode:** a valid card scan sends the lift non-stop to Floor 9, then returns to normal service
- **Card scan authentication:** validates VIP card IDs; fake or unknown cards are rejected
- **Emergency alarm mode:** triggers the alarm, returns the lift to the ground floor, opens the doors, and blocks card access
- **Passenger capacity limit:** rejects entry at 8 passengers or over 560 kg

## How It Works

1. A user presses a floor button or scans a VIP card.
2. If the alarm is active, a lockout message is shown.
3. If the cabin is full (8 passengers or over 560 kg), the request is rejected.
4. Otherwise the request is added to the queue. A VIP request triggers express travel to Floor 9.
5. The lift moves floor by floor, the door opens, and passengers board or exit (weight changes by 70 kg per person).
6. The lift repeats until the queue is empty, then goes idle.

## Project Structure

| File | Purpose |
|------|---------|
| `the_TitanLift.c` | Main program logic |
| `titanlift_gui.c` | GTK graphical interface |
| `demo.c` | Demo program |
| `Team_9_Project/` | Full project folder |
| `Team_9_Report.pdf` | Project report |
| `G5_T9_Project_Proposal.pdf` | Project proposal |
| `Team_9_Slide.pdf` / `.pptx` | Presentation slides |
| `Team_9_video_demo.mp4` | Video demo |
| `*.dll` | GTK runtime libraries for Windows |

## Run

1. Download or clone the repo.
2. Keep the `.exe` files in the same folder as the `.dll` files.
3. Open `titanlift_gui.exe` for the GUI, or `the_TitanLift.exe` for the non-GUI version.

## Build from Source

Requires GCC and GTK 3 (for example via MSYS2).

```bash
gcc titanlift_gui.c -o titanlift_gui.exe `pkg-config --cflags --libs gtk+-3.0`
```

## Team

- Choeun Linda
- Oeurn Meylih
