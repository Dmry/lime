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

```
git clone https://github.com/Dmry/lime.git
mkdir lime/build && cd "$_"
cmake ..
make lime
make install
```