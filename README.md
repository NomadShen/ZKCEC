# ZK-CEC

## Requirement
- **OS:** Linux (Recommended), macOS, or Windows (WSL2).

## Use the Pre-built Docker Image (Recommended)
### Pull the Image
```bash
docker pull ufsirv/zkcec_image:v1
```

### Running the Evaluation
Runs the default benchmark on the 2-bit adder.
```bash
docker run -it zkcec_image
```

Runs the default benchmark on the 4-bit multiplier.
```bash
docker run -it -e "-o -d mult_4x4" zkcec_image
```

+ To run with optimization, add `-o`.
+ To run with alternative design, add `-d $(design_name)`.
+ To run on specific port, add `-p $(port_name)`.

### Own Input file
If you want test your own design, generate the refutation proof and infomation file using our [CNF-GEN](https://github.com/NomadShen/CNF-GEN). You can run:
```bash
git submodule update --init --recursive
```
Follow the README and copy `*.sorted.unfold`, `*.sorted`, and `*.info` to `./input`.

## Build Locally

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
First configure CMakeLists.txt to set the path to NTL lib and build:
```
cmake -B build
cmake --build build
```

Then runs the script:
```bash
bash run_experiment.sh
```

+ To run with optimization, add `-o`.
+ To run with alternative design, add `-d $(design_name)`.
+ To run on specific port, add `-p $(port_name)`.

### Example
1. Evaluate 4-bit multiplers with optimization:
```bash
bash run_experiments.sh -d mult_4x4 -o
```
2. Evaluate AES S-box at port 1500:
```bash
bash run_experiments.sh -d sbox_aes -p 1500
```

#### Get our Full Benchmark

- You can unzip the `full_benchmark.zip` to get the full benchmark.
- You can download the design file of the full benchmark for the experiments from our Google drive.
    + [Original Designs](https://drive.google.com/file/d/1umyJBWoxnXRAWMBeO5RZvsaq1vrz-Rtx/view?usp=sharing)

## License

This project is licensed under the MIT License.
See the LICENSE file for details.
