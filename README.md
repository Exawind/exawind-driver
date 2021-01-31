# ExaWind Simulation Environment 

This repository contains a pure C++ driver API for coupling
[Nalu-Wind](https://github.com/exawind/nalu-wind) and
[AMR-Wind](https://github.com/exawind/amr-wind) CFD codes. Compared to
[exawind-sim](https://github.com/exawind-sim), this does not contain all the
features, but provides a prototype to couple codes.

## Compilation

```
git clone https://github.com/sayerhs/exwsim-cpp.git 
cd exwsim-cpp

cmake -Bbuild .
cmake --build build 
```

## Execution

```
mpiexec -np <N> <path>/exwsim-cpp/build/nalu_amr_wind exwsim.yaml
```
