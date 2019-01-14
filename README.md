Runlog is similar to [GNU time][gtime]. It reports the running time and peak
memory of a command line. Runlog additionally gives system information such as
number of CPUs, x86 SIMD, OS version, host name, installed RAM and available
RAM, as those reported separately by uname, nproc and free commands. File
[runlog.c][runlog.c] also shows how to obtain such system information on Mac
and Linux.

To compile runlog,
```sh
git clone https://github.com/attractivechaos/runlog
cd runlog
make
```
It has been tested on CentOS 6/7 and macOS High Sierra 10.13.

[gtime]: https://www.gnu.org/software/time/
