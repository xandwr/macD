# macD - A Basic C Process Monitor

**Author**: Alexander Pickering \
**Last Updated**: 2024/11/22

## Overview

`macD` is a C program that functions as a process monitor. \
\
It reads a configuration file specifying a maximum time limit and a list of processes (with support for arguments) to run, executes those processes, and monitors their resource usage. \
\
If a process exceeds the specified time limit, `macD` terminates it. The program provides periodic status updates and supports clean shutdowns via signals.


## Features

- **Process Execution**: Runs processes specified in a configuration file with optional arguments.
- **Time Monitoring**: Terminates processes exceeding the defined time limit.
- **Resource Usage**: Tracks CPU and memory usage for running processes.
- **Signal Handling**: Supports clean termination using `SIGINT` or `SIGABRT`.
- **Status Updates**: Provides periodic status reports for all monitored processes.


## Usage

```bash
./macD -i config.conf
```

> NOTE: `-i` is a **mandatory** flag, and the program will not run without a valid config file.


## Compilation
`macD` requires `gcc-10` and `make` to compile, and only works on UNIX-based systems. \
\
To build the program, simply run `make` in the root directory. \
\
The compiled executable can be located in `/build`.