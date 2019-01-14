Runlog is similar to [GNU time][gtime]. It reports the running time and peak
memory of a command line. Runlog additionally gives system information such as
number of CPUs, x86 SIMD, OS version, host name, installed RAM and available
RAM, as those reported separately by the [uname][uname], [nproc][nproc] and the
[free][ufree] commands. File [runlog.c](runlog.c) also showcases how to obtain
such system information on Mac and Linux. I developed runlog to collect host
information along with timing when a job is submitted to a cluster.

To compile runlog,
```sh
git clone https://github.com/attractivechaos/runlog
cd runlog
make
```
It has been tested on CentOS 6/7 and macOS High Sierra 10.13.

[gtime]: https://www.gnu.org/software/time/
[uname]: https://linux.die.net/man/1/uname
[nproc]: https://linux.die.net/man/1/nproc
[ufree]: https://linux.die.net/man/1/free
