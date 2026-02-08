# Noteworthy Server
This is the backend for a live slides annotation web app made for a group project in my software development methodology course. We wanted to make a web app, but the course required over 80% of the code to be in C++. To sidestep this limitation, our group decided to use QT with emscripten to create the frontend.

It is currently hosted at [noteworthy.howdoesthiseven.work](https://noteworthy.howdoesthiseven.work). Give it a try with two browser windows open or on two different devices! Not optimized for phones but does work on tablets.

## Note on code contents
This project is only ~70% C++ which is lower than the required 80%. However the non-C++ code is only used for testing, build and deployment configuration. 100% of the behavior of the project is implemented in C++. Also, if you also consider the Noteworthy QT repository the code is 94% C++ which is well over the limit.
Calculation (valid as of November 25th)

| Repository        | Lines of code | % C++ code |
| ----------------- | ------------- | ---------- | 
| Noteworthy Server |          2760 |        70% |
| Noteworthy QT     |         24458 |        97% |
| Total             |         27218 |    **94%** |

Calculation: $$\frac{2760 \cdot 70\% + 24458 \cdot 97\%}{27218} = 94\%$$

## Compiling/Debugging Locally
There are a few shell scripts to run to get started. Note this was only tested on Ubuntu with WSL.
- Run `./scripts/setup_build.sh` to get the required dependencies using apt
- Run `./scripts/compile_locally.sh` to compile the project
- Run `./scripts/run_server.sh` to run the server, or use the `launch.json` with the VSCode debugger to debug it.

## Using Docker
To run the server platform independently you can install Docker Desktop and run `docker compose up`
