# abmof_libcaer
A C++ version abmof based on libcaer. 
This branch contains a modified version of the original code, present in the [master](https://github.com/wzygzlm/abmof_libcaer/tree/master) branch. This version receives a *.txt* file containing event data and outputs a *.txt* file containing optical flow vectors.

<!-- TABLE OF CONTENTS -->
<details open="open">
  <summary>Table of Contents</summary>
  <ol>
    <li><a href="#usage">Usage</a></li>
    <li><a href="#considerations">Considerations</a></li>
  </ol>
</details>


<!-- Usage -->
## Usage

In order to use this code, first of all, the user needs to have a *.txt* file with event data in the following format:

> **TIMESTAMP**	**X**	**Y**
> 1000000	100	256
> 1000521	53	12
> 1001028	243	118

Information regarding polarity is discarded, so there is no need to include it in this file.
The event files that will be used for optical flow extraction need to be in the same folder as the executable file `abmof_libcaer`. For an event file with the name `eventSample.txt`, to extract its optical flow, the user needs to run the following bash command:
```sh
sudo ./abmof_libcaer eventSample.txt
```

Then, a file containing optical flow vectors will be originated with the name `OF_eventSample.txt`. Its outputs appears as follows:

> **TIMESTAMP**	**X**	**Y**	**Vx**	**Vy**	**Norm**
> 1000000	100	256	2	5	5.39
> 1000521	53	12	-1	1	1.41
> 1001028	243	118	7	-2	7.28


<!-- Considerations -->
## Considerations

The user can make changes to the algorithm's parameters. In this branch's published code, the parameters were defined to be compatible with downsampled data originally obtained from a CeleX5 event camera. As such, if an user utilizes data obtained from another event camera, a changing of specific parameters will be required. These parameters can be changed in the file called `abmof_hw_accel.h`, contained inside the `/src/ABMOF/` folder.
If a user changes the source code, it can be easily compiled due to the already existing Makefile. Just run:
 ```sh
sudo ./abmof_libcaer eventSample.txt
```

Special thanks to Min Liu for helping me in the process of adapting his original implementation.
