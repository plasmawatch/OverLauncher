# OverLauncher
Plasmawatch is an Overwatch 1 Private Server recreation project. This is the patcher and launcher for the project.

Join the discord: https://discord.gg/T5RncE2RkN

Written in C++.

## Setup (REQUIRED)
In order for this to work, you need to point the domain `bnet-emu.fish` to `127.0.0.1`.
1. Open Notepad or any text editor with ADMINISTRATOR.
2. Head to `C:\Windows\System32\drivers\etc\hosts` and open the file.
3. Add `127.0.0.1 bnet-emu.fish` to the file.
4. Click save.

(This won't mess with your computer in any way.)

## Usage
`OverLauncher "path in quotes"`
(You need quotes or it will not load, blame how strings work in c++ than me please)

Example: `OverLauncher.exe "D:\t\Overwatch 1\Overwatch Beta 0.8.0.24919\GameClientApp.exe"`

### Note
This is like really bad so I would 100% advise someone else to make a 3rd party launcher just using the patcher code.
