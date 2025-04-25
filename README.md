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
   gcc -o renamed main.c
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
You can use the following command-line flags:

- `-v` Show version info
- `-h` Show usage instructions
- `-f` Force mode – includes *all* files, not just video formats (`.mp4`, `.mkv`, `.avi`)
- `-p <path>` Specify custom output path
- `-k` Keep original files (copy instead of rename)
- `-d` Dry run mode - show what would happen without making changes
- `--log[=file]` Create log file (default: renamed_log.txt)
- `--pattern=<regex>` Specify custom regex pattern for episode detection

Examples:
```bash
# Force include all file types
./renamed -f

# Create a log file of all operations
./renamed --log

# Use a custom pattern for episode detection
./renamed --pattern='Season (\d+)-Episode (\d+)'

# Combination of options
./renamed -k -f --log -p /path/to/output
```

## 🧠 Features
- Detects and extracts episode numbers from various common naming styles
- Groups and handles specials (e.g., OVA, SP, Bonus) separately
- Skips renaming if the episode number can't be detected
- Shows a full preview of all renames before making changes
- Interactive confirmation step before any file is renamed
- Optionally works on *any* file type with `-f`
- Dry run mode to preview changes without modifying files
- Detailed logging of all operations for troubleshooting
- Custom regex pattern support for specialized naming schemes

## 📜 Logging
When using the `--log` option, ReNamed creates a detailed log file that includes:
- Session start and end timestamps
- All operations performed (rename/copy)
- Success or failure status of each operation
- Warnings and error messages

This is especially useful for batch operations or when troubleshooting issues.

## 🔍 Custom Patterns
The `--pattern` option allows you to specify your own regular expression for detecting episode numbers, which is useful for shows with unconventional naming. The pattern should include at least one capture group for the episode number.

Examples:
```bash
# For filenames like "Show.S01E05.mp4"
./renamed --pattern='S[0-9]+E([0-9]+)'

# For filenames with season and episode like "Show.S01-E05.mp4" 
# (uses the second capture group)
./renamed --pattern='S([0-9]+)-E([0-9]+)'
```

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

<hr>
<p align="center">
<a href="https://www.star-history.com/#Panonim/ReNamed&Date">
 <picture>
   <source media="(prefers-color-scheme: dark)" srcset="https://api.star-history.com/svg?repos=Panonim/ReNamed&type=Date&theme=dark" />
   <source media="(prefers-color-scheme: light)" srcset="https://api.star-history.com/svg?repos=Panonim/ReNamed&type=Date" />
   <img alt="Star History Chart" src="https://api.star-history.com/svg?repos=Panonim/ReNamed&type=Date" />
 </picture>
</a>
</p>

© 2025 Artur Flis. All Rights Reserved.
