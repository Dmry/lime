[![Build Status](https://api.travis-ci.org/Dmry/lime.svg?branch=master)](https://travis-ci.org/Dmry/lime) [![CodeFactor](https://www.codefactor.io/repository/github/dmry/lime/badge/master)](https://www.codefactor.io/repository/github/dmry/lime/overview/master) [![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
 
# lime
Tool for the application of Likhtman&amp; McLeish' reptation theory under active development.

Currently only compiles on linux. Windows users should use the Docker version.

## Requirements

* Boost &ge; 1.66
* C++17 with parallel algorithms (GCC &ge; 9)
* GNU Scientific Library &ge; 2.4
* Intel-tbb &ge; 2018

## Compiling from source

### Requirements

#### Fedora
```
sudo yum install gcc git g++ cmake make boost-devel gsl-devel tbb-devel
# GUI
sudo yum install pybind11-devel python3-devel python3-matplotlib python3-matplotlib-gtk3 python3-gobject python3-numpy
```

#### Arch linux
```
sudo pacman -S gcc git g++ cmake make boost gsl tbb
```

#### Ubuntu
```
sudo apt install gcc g++ cmake git make libboost-dev libgsl-dev libtbb-dev
# GUI
sudo apt install python3-pybind11 python3-matplotlib python3-cairo python3-cairocffi python3-gi
```

### Compiling
```
git clone https://github.com/Dmry/lime.git
mkdir lime/build && cd "$_"
cmake ..
make lime
make install
```

Beware! Pybind11 seems to have issues with python3.8 on Ubuntu 20.04.1. If you want to use the UI, use the following instead of `cmake ..`
```
cmake .. -DPYTHON_INCLUDE_DIRS=$(python -c "from distutils.sysconfig import get_python_inc; print(get_python_inc())")  -DPYTHON_LIBRARY=$(python -c "import distutils.sysconfig as sysconfig; print(sysconfig.get_config_var('LIBDIR'))") -DPYTHON_EXECUTABLE:FILEPATH=`which python3`
```

### Packaging

#### Fedora
Assuming you have followed the instructions under 'Requirements' and 'Compiling', from the 'build' directory:
```
sudo yum install rpm-build
cpack
```
