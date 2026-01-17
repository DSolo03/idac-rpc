
# idac-rpc

Discord Rich Prescence bridge for Initial D The Arcade



## Features

- Injectable DLL, that reading course data right from game memory
- Python bridge, that inject DLL into game, recieving course data from it and send Discord Rich Presence
- Configuration file



## Installation

Injectable DLL can be compiled via CMAKE

```cmd
  cmake -B build
  cmake --build build --config Release
```
Install Python libraries

```cmd
pip install -r /path/to/requirements.txt
```

## Usage

First, you need to launch the game and only then

```cmd
python src\python\main.py
```

_Or simply run the pre-compiled idac-rpc.exe from the [Releases](https://github.com/DSolo03/idac-rpc/releases) page._