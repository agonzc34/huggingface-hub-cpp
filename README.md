# huggingface-hub-cpp

This library is a C++ port of the [Hugging Face Hub](https://github.com/huggingface/huggingface_hub) that provides an implementation of hf_hub_download for C++. It enables seamless downloading of models and other assets from the Hugging Face Hub while maintaining full compatibility with its Python counterpart.

With this library, C++ developers can integrate Hugging Face model downloads directly into their applications, ensuring efficient and reliable access to AI resources without requiring Python dependencies.

[![GitHub License](https://img.shields.io/github/license/agonzc34/huggingface-hub-cpp)](https://opensource.org/license/mit) [![GitHub release](https://img.shields.io/github/release/agonzc34/huggingface-hub-cpp.svg)](https://github.com/agonzc34/huggingface-hub-cpp/releases) [![Code Size](https://img.shields.io/github/languages/code-size/agonzc34/huggingface-hub-cpp.svg?branch=main)](https://github.com/agonzc34/huggingface-hub-cpp?branch=main) [![Last Commit](https://img.shields.io/github/last-commit/agonzc34/huggingface-hub-cpp.svg)](https://github.com/agonzc34/huggingface-hub-cpp/commits/main) [![GitHub issues](https://img.shields.io/github/issues/agonzc34/huggingface-hub-cpp)](https://github.com/agonzc34/huggingface-hub-cpp/issues) [![GitHub pull requests](https://img.shields.io/github/issues-pr/agonzc34/huggingface-hub-cpp)](https://github.com/agonzc34/huggingface-hub-cpp/pulls) [![Contributors](https://img.shields.io/github/contributors/agonzc34/huggingface-hub-cpp.svg)](https://github.com/agonzc34/huggingface-hub-cpp/graphs/contributors) [![Build](https://github.com/agonzc34/huggingface-hub-cpp/actions/workflows/cmake-build-status.yml/badge.svg)](https://github.com/agonzc34/huggingface-hub-cpp/actions/workflows/cmake-build-status.yml?branch=main) [![Doxygen Deployment](https://github.com/agonzc34/huggingface-hub-cpp/actions/workflows/doxygen-deployment.yml/badge.svg)](https://agonzc34.github.io/huggingface-hub-cpp/)

## Table of Contents

- [huggingface-hub-cpp](#huggingface-hub-cpp)
  - [Table of Contents](#table-of-contents)
  - [Installation](#installation)
    - [Prerequisites](#prerequisites)
    - [Build](#build)
  - [Usage](#usage)
    - [Integrating the library](#integrating-the-library)
    - [Running the demo app](#running-the-demo-app)
  - [License](#license)

## Installation

### Prerequisites

Before installing, ensure you have the following dependencies installed:

- CMake (Minimum version recommended: 3.x)
- libcurl (Required for handling downloads)

On Debian-based systems, you can install libcurl using:

```shell
sudo apt-get install libcurl4-openssl-dev libssl-dev
```

### Build

To build the project, follow these steps:

```shell
git clone https://github.com/agonzc34/huggingface-hub-cpp
mkdir build
cd build
cmake ..
make
```

## Usage

### Integrating the library

To use this library in your C++ project, include the necessary headers and call `hf_hub_download`.

```cpp
#include "huggingface_hub.h"

int main() {
  std::string repo_id = "<huggingface-repo>";
  std::string filename = "<file-to-download>";

  if (huggingface_hub::hf_hub_download(repo_id, filename).success) {
    std::cout << "Model downloaded!" << std::endl;
  } else {
    std::cout << "Error" << std::endl;
  }
}
```

### Running the demo app

A demo application is included to showcase the library in action. To build and run the demo:

```shell
./build/hfhub_demo
```

This will execute a sample download for the qwen2.5-0.5b-instruct-q2_k.gguf model

## License

This project is licensed under the MIT License. See the [LICENSE](./LICENSE) file for more details.
