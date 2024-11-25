#Noteworthy Server

## Note on code contents
This project is only ~70% C++ which is lower than the required 80% but the non-C++ code is only used for testing, build and deployment configuration. 100% of the behavior of the project is implemented in C++. Also, if you also consider the Noteworthy QT repository the code is ___ C++
Calculation (valid as of November 25th)

| Repository        | Lines of code | % C++ code |
| ----------------- | ------------- | ---------- | 
| Noteworthy Server |          2760 |        70% |
| Noteworthy QT     |         24458 |        97% |
| Total             |         27218 |    **94%** |

Calculation: $$\frac{2760 \cdot 70\% + 24458 \cdot 97\%}{27218}$$

Noteworthy Server lines of code: 2760

Noteworthy QT lines of code

## Compiling/Debugging Locally
There are a few shell scripts to run to get started. Note this was only tested on Ubuntu with WSL.
- Run `./scripts/setup_build.sh` to get the required dependencies using apt
- Run `./scripts/compile_locally.sh` to compile the project
- Run `./scripts/run_server.sh` to run the server, or use the `launch.json` with the VSCode debugger to debug it.

## Using Docker
To run the server platform independently you can install Docker Desktop and run `docker compose up`

