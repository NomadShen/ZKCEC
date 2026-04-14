# ZK-CEC

## Install

### Requirements
- Emp-tool (latest version)
- Emp-zk
- Emp-ot
- OpenSSL
- NTL
- pkg-config
- LibSodium
- gmp

### Compile 
First configure CMakeLists.txt to set the path to NTL lib.

Then run:
```
cmake -B build
cmake --build build
```

### Input file
If you want test your own design, generate the refutation proof and infomation file using our tool, and copy `*.sorted.unfold`, `*.sorted`, and `*.info` to `./input`.

## Run experiments
First configure the $DESIGN in run_experiments.sh.

Then run:
```bash run_experiments.sh```

+ To run with optimization, add `-o`.
+ To run with alternative design, add `-d $(design_name)`.
+ To run on specific port, add `-p $(port_name)`.