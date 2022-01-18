# SuperSim - Installation

## Summary
This document outlines how to install SuperSim, its dependencies, and its
accompanying tools. The instructions in the document will build a stand-alone
environment that can be modified and rebuilt easily.

## System requirements
SuperSim is designed to run on Linux, although it doesn't necessarily rely
on Linux features. Clever coders will be able to adapt these instructions
for Windows or Mac, however, this is not officially supported.

These installation instructions require the following software:
- c++ (must support c++17)
- python3 (3.7+)
- bazel

The following command installs the necessary software:
``` sh
# Debian/Ubuntu
sudo apt install g++ git python3 python3-dev python3-pip util-linux wget clang-format

# Fedora/RedHat/Centos
sudo yum install gcc-c++ git python3 python3-devel python3-pip util-linux wget clang-format

pip3 install sssweep
```

## Create a development directory
Create a directory to hold the development environment:

``` sh
mkdir -p ~/ssdev
cd ~/ssdev
```

## Build C++ projects
The C++ projects use [Bazel][bazel] for building binaries. To install Bazel, follow the directions at [here][bazelinstall].

The C++ projects are built as stand-alone executables. No system installation takes place. Use the following commands to build and test the C++ programs:

``` sh
for prj in supersim ssparse; do
    git clone git@github.com:ssnetsim/${prj} ~/ssdev/${prj}
    cd ~/ssdev/${prj}
    bazel test -c opt ...
done
```

By default, Bazel places the binaries in the `bazel-bin` in the project directory. The following commands should all work:

``` sh
~/ssdev/supersim/bazel-bin/supersim --help
~/ssdev/ssparse/bazel-bin/ssparse --help
```

Now you are ready to run your first simulations! Visit the [Basic Simulations][basicsims] guide.

[bazel]: https://bazel.build/ "Bazel Build"
[bazelinstall]: https://docs.bazel.build/versions/master/install.html "Bazel Install"
[basicsims]: basic_sims.md "Basic Simulations"
