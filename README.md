# PETAce-SetOps

## Introduction

<!-- start-petace-setops-overview -->

PETAce-SetOps is a collection of protocols that perform private set operations.
It is one of the many components in [the framework PETAce](https://github.com/tiktok-privacy-innovation/PETAce).

Private set operations generally include private set intersection (PSI), private join and compute (PJC), and private information retrieval (PIR) protocols.
Currently, PETAce-SetOps implements the ECDH-PSI protocol based on Elliptic-Curve Diffie-Hellman, the [KKRT-PSI](https://dl.acm.org/doi/abs/10.1145/2976749.2978381) protocol based on Oblivious Pseudorandom Functions (OPRF), and the PJC protocol based on [Circuit-PSI](https://www.researchgate.net/publication/356421123_Circuit-PSI_With_Linear_Complexity_via_Relaxed_Batch_OPPRF).

<!-- end-petace-setops-overview -->

## Requirements

<!-- start-petace-setops-getting-started -->

| System | Toolchain                                             |
|--------|-------------------------------------------------------|
| Linux  | Clang++ (>= 5.0) or GNU G++ (>= 5.5), CMake (>= 3.15) |

| Required dependency                                                            | Tested version | Use                                  |
|--------------------------------------------------------------------------------|----------------|--------------------------------------|
| [PETAce-Solo](https://github.com/tiktok-privacy-innovation/PETAce-Solo)        | 0.3.0          | Cryptography primitives              |
| [PETAce-Verse](https://github.com/tiktok-privacy-innovation/PETAce-Verse)      | 0.3.0          | Primitive cryptographic protocols    |
| [PETAce-Duet](https://github.com/tiktok-privacy-innovation/PETAce-Duet)        | 0.3.0          | Two-party secure computing protocols |
| [PETAce-Network](https://github.com/tiktok-privacy-innovation/PETAce-Network)  | 0.3.0          | Network communication protocols      |
| [Google Logging](https://github.com/google/glog)                               | 0.4.0          | Logging                              |
| [JSON for Modern C++(JSON)](https://github.com/nlohmann/json)                  | 3.10.1         | PSI parameter configuration          |

| Optional dependency                                | Tested version | Use                    |
|----------------------------------------------------|----------------|------------------------|
| [GoogleTest](https://github.com/google/googletest) | 1.12.1         | For running tests      |
| [gflags](https://github.com/gflags/gflags)         | 2.2.2          | For running benchmarks |

## Building PETAce-SetOps

We assume that all commands presented below are executed in the root directory of PETAce-SetOps.

First, build [JSON for Modern C++ (JSON)](https://github.com/nlohmann/json) using the following scripts.
Assume that JSON is cloned into the directory `${JSON}`.

```bash
cmake -B ${JSON}/build -S ${JSON}
cmake --build ${JSON}/build -j
```

Then, build PETAce-SetOps library (optionally with test and example):

```bash
cmake -S . -B build -Dnlohmann_json_DIR=${JSON}/build -DSETOPS_BUILD_TEST=ON -DSETOPS_BUILD_EXAMPLE=ON
cmake --build build
```

Output binaries can be found in `build/lib/` and `build/bin/` directories.

| Compile Options            | Values        | Default | Description                                         |
|----------------------------|---------------|---------|-----------------------------------------------------|
| `CMAKE_BUILD_TYPE`         | Release/Debug | Release | Debug mode decreases run-time performance.          |
| `SETOPS_BUILD_SHARED_LIBS` | ON/OFF        | OFF     | Build a shared library if set to ON.                |
| `SETOPS_BUILD_EXAMPLE`     | ON/OFF        | ON      | Build C++ example if set to ON.                     |
| `SETOPS_BUILD_TEST`        | ON/OFF        | ON      | Build C++ test if set to ON.                        |
| `SETOPS_BUILD_DEPS`        | ON/OFF        | ON      | Download and build unmet dependencies if set to ON. |

Here we give a simple example to run protocols in PETAce-SetOps.

To run as Party A (a sender):

```bash
bash build/example/scripts/ecdh_psi_sender_example.sh
```

To run as Party B (a receiver):

```bash
bash build/example/scripts/ecdh_psi_receiver_example.sh
```

<!-- end-petace-setops-getting-started -->

## Contribution

Please check [Contributing](CONTRIBUTING.md) for more details.

## Code of Conduct

Please check [Code of Conduct](CODE_OF_CONDUCT.md) for more details.

## License

This project is licensed under the [Apache-2.0 License](LICENSE).

## Citing PETAce

To cite PETAce in academic papers, please use the following BibTeX entries.

### Version 0.3.0

```tex
    @misc{petace,
        title = {PETAce (release 0.3.0)},
        howpublished = {\url{https://github.com/tiktok-privacy-innovation/PETAce}},
        month = Jun,
        year = 2024,
        note = {TikTok Pte. Ltd.},
        key = {PETAce}
    }
```
