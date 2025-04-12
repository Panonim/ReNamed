<img src="assets/ReNamed-Banner.svg" alt="Renamed Banner" />
<p align="center">
<a href="https://github.com/Panonim/ReNamed/issues">
<img alt="GitHub Issues" src="https://img.shields.io/github/issues/Panonim/ReNamed?style=flat-square">
</a>
<img alt="GitHub repo size" src="https://img.shields.io/github/repo-size/Panonim/ReNamed?style=flat-square">
<img alt="GitHub last commit" src="https://img.shields.io/github/last-commit/Panonim/ReNamed?style=flat-square">
<a href="https://github.com/Panonim/ReNamed/releases">
<img alt="GitHub Release" src="https://img.shields.io/github/v/release/Panonim/ReNamed?style=flat-square">
</a>
<a href="https://github.com/sponsors/Panonim">
<img alt="GitHub Sponsors" src="https://img.shields.io/github/sponsors/Panonim?style=flat-square">
</a>
</p>

# Renamed 

**ReNamed** is a simple, terminal-based C tool that helps you automatically rename and organize episodes of TV shows — including specials — with clean, consistent filenames.

## 💡 What it Does

This utility scans a folder of video files (or optionally any files), detects the episode numbers using a variety of common patterns, and renames them based on a user-supplied show name. Special episodes (like OVAs, bonus content, or labeled "SP") are detected and moved into a separate `Specials/` subfolder.

## 🛠️ How to Use
1. Download the file
   ```bash
   curl -O https://raw.githubusercontent.com/Panonim/ReNamed/refs/heads/main/main.c
   ```

2. Compile the program:
   ```bash
   gcc -o renamed main.c -Wall
   ```

3. Run it:
   ```bash
   ./renamed
   ```
   
4. Follow the prompts:
   - Enter the show name (e.g., `Attack on Titan`)
   - Enter the path to the folder with episodes
   - Review the renaming plan
   - Confirm whether to proceed

**Optionally put it inside /usr/local/bin to use it anywhere**

## ⚙️ Options

You can also use flags:

- `-v` Show version info
- `-h` Show usage instructions
- `-f` Force mode – includes *all* files, not just video formats (`.mp4`, `.mkv`, `.avi`)

Example:
```bash
./renamed -f
```

## 🧠 Features

- Detects and extracts episode numbers from various common naming styles
- Groups and handles specials (e.g., OVA, SP, Bonus) separately
- Skips renaming if the episode number can't be detected
- Shows a full preview of all renames before making changes
- Interactive confirmation step before any file is renamed
- Optionally works on *any* file type with `-f`

## 📂 Example

Say you have:
```
Attack_on_Titan_E01.mkv
Attack_on_Titan_Special_01.mp4
Attack_on_Titan_E02.mkv
```

After running the tool, the folder becomes:
```
Attack on Titan - 01.mkv
Attack on Titan - 02.mkv
Specials/
  Attack on Titan - 01 - Special.mp4
```
© Artur Flis, 2025 All rights saved. 
