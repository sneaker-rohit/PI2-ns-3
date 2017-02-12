# Implementation of PI2 AQM in ns-3

You can find the Implementation for AQM PI2 [1] In ns-3 in this repository. PI2 offers a simple design and achieves no worse or in some cases better responsiveness and stability in comparison to PIE [2].

# Reproduce simulation results as presented in paper

### Prerequisites    
1. ns-3.26 or higher. It is necessary that the ns-3 version you use has PIE Queue Disc available. PIE has been made available since 3.26 release.#     


### Steps to reproduce results ###

1. In this repository, navigate to src -> traffic control -> model and copy pi-square-queue-disc.h and pi-square-queue-disc.cc and paste in at the same path under your ns-3.xx directory.

2. Copy the examples from scratch folder in this repository and place it in under the scratch folder of your ns-3.xx directory.

3. Use the below command, in order to execute the example.

```
./waf --run scratch/pie-first
```

### Description of Example programs ###

PIE Programs

1. pie-first.cc -> simulates light TCP traffic with Queue Disc = PIE and reference delay = 20ms 

2. pie-second.cc -> simulates heavy TCP traffic with Queue Disc = PIE and reference delay = 20ms 

3. pie-third.cc -> simulates a mix of TCP and UDP traffic with Queue Disc = PIE and reference delay = 20ms 

4. pie-third-lowdelay.cc -> simulates a mix of TCP and UDP traffic with Queue Disc = PIE and reference delay = 5ms

5. pie-fourth.cc -> simulates 20 TCP flows with Queue Disc = PIE and reference delay = 20ms 
 
PI Square Programs

1. pi-square-first.cc -> simulates light TCP traffic with Queue Disc = PI2 and reference delay = 20ms 

2. pi-square-second.cc -> simulates heavy TCP traffic with Queue Disc = PI2 and reference delay = 20ms 

3. pi-square-third.cc -> simulates a mix of TCP and UDP traffic with Queue Disc = PI2 and reference delay = 20ms 

4. pi-square-third-lowdelay.cc -> simulates a mix of TCP and UDP traffic with Queue Disc = PI2 and reference delay = 5ms

5. pi-square-fourth.cc -> simulates 20 TCP flows with Queue Disc = PI2 and reference delay = 20ms 
 
# References

[1] De Schepper, K., Bondarenko, O., Tsang, J., & Briscoe, B. (2016, November). PI2: A Linearized AQM for both Classic and Scalable TCP. In Proceedings of the 12th International on Conference on emerging Networking EXperiments and Technologies (pp. 105-119). ACM.

[2] Pan, R., Natarajan, P., Piglione, C., Prabhu, M. S., Subramanian, V., Baker, F., & VerSteeg, B. (2013, July). PIE: A lightweight control scheme to address the bufferbloat problem. In High Performance Switching and Routing (HPSR), 2013 IEEE 14th International Conference on (pp. 148-155). IEEE.

