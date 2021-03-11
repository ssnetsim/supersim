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
- c++ (g++ must be 4.8+, last tested using 7.3)
- git
- python3
- numpy
- matplotlib
- bazel
- wget
- column

The following command installs the package dependencies:
``` sh
[apt-get]
sudo apt-get install g++ git python3 python3-dev util-linux wget clang-format

[yum]
sudo yum install gcc-c++ git python3 python3-devel util-linux wget clang-format

pip3 install setuptools numpy matplotlib psutil --user
```

## Create a development directory
Create a directory to hold the development environment:

``` sh
mkdir -p ~/ssdev
cd ~/ssdev
```

## Install Python projects
The required Python packages will be installed with `--user` which means
they won't effect the system installation. These can be installed with
following commands:

``` sh
for prj in simplecsv percentile taskrun; do
    git clone git://github.com/nicmcd/${prj} ~/ssdev/${prj}
    cd ~/ssdev/${prj}
    python3 setup.py install --user --record files.txt
done
for prj in sssweep ssplot; do
    git clone git://github.com/ssnetsim/${prj} ~/ssdev/${prj}
    cd ~/ssdev/${prj}
    python3 setup.py install --user --record files.txt
done
```

If you wanted to, you could uninstall any of these packages by running `rm $(cat files.txt)` inside the package directory. The following commands should all work:
``` sh
python3 -c "import simplecsv"
python3 -c "import percentile"
python3 -c "import taskrun"
python3 -c "import sssweep"
python3 -c "import ssplot"
ssplot --help
```

## Build C++ projects
The C++ projects use [Bazel][bazel] for building binaries. To install Bazel, follow the directions at [here][bazelinstall].

The C++ projects are built as stand-alone executables. No system installation takes place. Use the following commands to build and test the C++ programs:

``` sh
for prj in supersim ssparse; do
    git clone git://github.com/ssnetsim/${prj} ~/ssdev/${prj}
    cd ~/ssdev/${prj}
    bazel test -c opt :*
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
