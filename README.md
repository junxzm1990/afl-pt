# Ptrix: Efficient Hardware-Assisted Fuzzing for COTS Binary

It should be noted that **we only tested Ptrix on bare metal machine running Ubuntu 14.04**.

## Introduction

PTrix is carefully designed to take full advantage of hardware feature - Intel Processor Trace as its underpinning tracing component. With PTrix, we demonstrate newly available hardware feature can significantly accelerate binary-only fuzzing through two elaborate designs, including a parallel scheme of trace parsing and a newly designed PT-friendly feedback.

## Installation

Please refer to [docs/INSTALL.md](docs/INSTALL.md)

## Publication

Our paper can be found in the [Ptrix - AsiaCCS 2019](ptrix.pdf).

```
@inproceedings{yaohui2019,
 author = {Chen, Yaohui and Mu, Dongliang and Xu, Jun and Sun, Zhichuang and Shen, Wenbo and Xing, Xinyu and Lu, Long and Mao, Bing},
 title = {Ptrix: Efficient Hardware-Assisted Fuzzing for COTS Binary},
 booktitle = {Proceedings of the 2019 on Asia Conference on Computer and Communications Security},
 series = {ASIACCS '19},
 year = {2019},
 publisher = {ACM},
} 
```

## CVEs discovered by Ptrix

|   CVE ID       | Vulnerability Type     | Vulnerable Software |
| -------------- | ---------------------- | ------------------- |
| CVE-2018-8816  | Stack Exhaustion       | perl-5.26.1         |
| CVE-2018-8881  | Heap buffer overflow   | nasm-2.13.02rc2     |
| CVE-2018-8882  | Stack buffer overflow  | nasm-2.13.02rc2     |
| CVE-2018-8883  | Global buffer overflow | nasm-2.13.02rc2     |
| CVE-2018-10016 | Division-by-zero       | nasm-2.14rc0        |
| CVE-2018-9138  | Stack Exhaustion       | binutils-2.29       |
| CVE-2018-9996  | Stack Exhaustion       | binutils-2.29       |
| CVE-2018-10316 | Denial-of-Service      | nasm-2.14rc0        |
| CVE-2018-9251  | Denial-of-Service      | libxml2-2.9.8       |

## Technical details

PTrix uses a series of optimization techniques to exploit intel PT for maximum fuzzing efficiencies, including: 

On the PT module side,
1. Direct feedback translation
2. Parallel packet decoding

On the AFL side,
1. Switch feedback scheme from counted edge to slice to improve path sensitivity
2. Use a bit instead of a byte of memory to record each slice to improve cache locality

Additionally, PTrix supports fork-server mode and multiplexes PT buffer for different fuzzing instances.

## Credits

- [Yaohui Chen](https://github.com/evanmak) 
- [Jun Xu](https://github.com/junxzm1990)
- [Dongliang Mu](https://github.com/mudongliang)
- [Zhichuang Sun](https://github.com/sunzc)
- [Yueqi Chen](https://github.com/chenyueqi)

## Contributing

Please refer to [docs/CONTRIBUTING.md](docs/CONTRIBUTING.md)

## ToDo List

1. Automatically patch the target binary if fuzz in PT mode (maybe create a copy and fuzz the copy instead).
2. Change power scheduling function of AFL to deprioritize seeds that triggered crashes (avoid traping into local region).
3. Gracely handle PMU interrupts.

