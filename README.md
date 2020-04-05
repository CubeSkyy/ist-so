# Heat Simulation

Project for 2017-2018 Operative Systems Course in Instituto Superior Técnico.

### Installing
In the base project directory:
```
make
```


### Running
In the base project directory:
```
make run
```
or

#### Project 1
```
heatSim N tEsq tSup tDir tInf iter num_tasks delta
```

#### Project 4
```
heatSim N tEsq tSup tDir tInf iter num_tasks delta file_name savePeriod
```
Where:

N - Size of matrix (Will be a NxN matrix)
* tEsq - Initial temperature in the left most part of matrix.
* tSup - Initial temperature in the upper most part of matrix.
* tDir - Initial temperature in the right most part of matrix.
* tInf - Initial temperature in the bottom most part of matrix.
* iter - Number of iterations
* num_tasks - Number of simultaneous tasks.
* delta - If the difference of values between 2 iterations if lower than this value, stop the simulation.
* file_name - Name of the backup file (In case simulation crashes).
* savePeriod - Period to save the progress in iterations.


## Authors
* **Miguel Coelho** - *87687*
* **Diogo Sousa ** - *87651*
