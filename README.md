# Terminal CLI Utilities

![C++](https://img.shields.io/badge/C%2B%2B-17-blue)
![Python](https://img.shields.io/badge/Python-3.6%2B-green)
![ncurses](https://img.shields.io/badge/ncurses-6.0%2B-lightblue)
![Platform](https://img.shields.io/badge/Platform-Linux-orange)
![License](https://img.shields.io/badge/License-Free-brightgreen)

This repository contains three terminal-based CLI utilities:

1. **DirMon** - A directory monitor that logs all file changes in real-time
2. **FileView** - A visual CLI tool that displays directory structure with highlights
3. **FileSearch** - A CLI utility for fuzzy searching and opening files

## Requirements

- Linux system (for inotify support)
- C/C++ compiler (GCC or Clang)
- ncurses development library
- Python 3.6+ (for the wrapper script)

### Installing Dependencies

On Debian/Ubuntu:
```bash
sudo apt-get install build-essential libncurses5-dev libncursesw5-dev
```

On Fedora/RHEL/CentOS:
```bash
sudo dnf install gcc-c++ ncurses-devel
```

On Arch Linux:
```bash
sudo pacman -S gcc ncurses
```

## Building and Installation

### Step 0: Download

```bash
git clone https://github.com/ArmaLv/fin-view.git
cd fin-view
```

### Step 1: Build the utilities

```bash
make
```

This will compile all utilities and create the necessary aliases in the `bin/` directory.

### Step 2: Install for easy access (recommended)

For user-level installation (no sudo required):

```bash
make install-user
```

This will install all utilities to `~/.local/bin/` so you can run them from anywhere.

For system-wide installation (requires sudo):

```bash
sudo make install
```

## Utilities

### DirMon

Monitor a directory and log all changes (add, remove, edit) in real-time.

**If installed:**
```bash
dirmon /path/to/directory [--log-file=logfile.txt] [--curses]
# or use the alias
dr /path/to/directory
```

**If not installed (from project directory):**
```bash
bin/dirmon /path/to/directory [--log-file=logfile.txt] [--curses]
# or use the alias
bin/dr /path/to/directory
```

### FileView

Display directory structure with file sizes, types, and highlights.

**If installed:**
```bash
fileview /path/to/directory [--sizes] [--times] [--perms] [--type=.cpp] [--minsize=1MB]
# or use the alias
fv /path/to/directory
```

**If not installed (from project directory):**
```bash
bin/fileview /path/to/directory [--sizes] [--times] [--perms] [--type=.cpp] [--minsize=1MB]
# or use the alias
bin/fv /path/to/directory
```

### FileSearch

Fuzzy search for files and open/edit them instantly.

**If installed:**
```bash
filesearch [search_term] [--path=/path/to/search] [--rebuild-cache]
# or use the alias
fs main.cpp
```

**If not installed (from project directory):**
```bash
bin/filesearch [search_term] [--path=/path/to/search] [--rebuild-cache]
# or use the alias
bin/fs main.cpp
```

## Finview Command-Line Interface

A unified interface for all utilities is provided through the `finview` script:

```bash
# Interactive mode with menus
finview

# Direct command execution
finview dirmon /path/to/directory
finview fileview /path/to/directory --sizes
finview filesearch main.cpp

# Using aliases
finview dr /path/to/directory
finview fv /path/to/directory
finview fs main.cpp
```

### Error Logging

Finview includes automatic error logging to help diagnose issues:

- Logs are stored in `~/.finview/logs/` directory
- Each day gets its own log file (e.g., `finview_20250513.log`)
- Logs include command execution details, errors, and warnings
- Old logs are automatically rotated (keeping only the last 7 days)

To view logs:

```bash
less ~/.finview/logs/finview_YYYYMMDD.log
```

This is particularly useful for debugging issues that may not be visible in the UI.

### Installing Finview

You have several options to make the finview script and its aliases accessible from anywhere:

#### Option 1: Using the Makefile (recommended)

For user-level installation (no sudo required):

```bash
make install-user
```

For system-wide installation (requires sudo):

```bash
sudo make install
```

#### Option 2: Using the script directly

```bash
./finview install
```

After installation, you can run from any directory:

```bash
finview               # Launch the interactive menu
dr /path/to/directory # Run dirmon directly
fv /path/to/directory # Run fileview directly
fs main.cpp           # Run filesearch directly
```

If the installation directory is not in your PATH, the installer will provide instructions to add it.

## License

Made by ArmaLv

This is free to use, modify and distribute project. No warranty is provided.
Project was made with goal of learning and practicing C++.
