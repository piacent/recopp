# recopp
** work in progress **

reconstruction library in C++

## Dependencies

* ROOT
* ROOTANA
* OpenCV

Before compiling, set the variables `ROOTANASYS` and `OPENCVSYS` in your environment and setup the `RECOPPSYS` variable (see Installation).


## Installation

`git clone https://github.com/piacent/recopp.git`

`cd recopp`

`export RECOPPSYS="/path/to/recopp"`

`mkdir build-dir`

`cmake ..`

`cmake --build .`

Generate documentation inside the `doc/html` folder:

`doxygen doc/doxygen.cfg`

Documentation should be then available at `doc/html/index.html.`.
