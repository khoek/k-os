# K-OS
[![Build Status](https://travis-ci.org/escortkeel/k-os.png?branch=master)](https://travis-ci.org/escortkeel/k-os)

An operating system written by Keeley Hoek as a hobby. It was inspired by Charlie Somerville's [JSOS](https://github.com/charliesome/JSOS).

## Features

* Multiboot 1
* Text UI
* Memory Managment Suite
    * Page Frame Allocator
    * Cache Allocator
    * Page Mapping Manipulation
* Drivers
    * PIT
    * Keyboard
    * PATA
    * E1000 (Intel 825xx) Ethernet Driver
* Time
    * Clock and Timer APIs
* Devices
    * Unified Bus, Device and Driver APIs
* Networking (functional, in progress)
    * DHCP support
    * 4 layer packet switch
        * Link layer (Ethernet)
        * Network layer (IP, ARP)
        * Transport layer (TCP, UDP)
        * Application layer (ICMP, NBNS)
* Tasks and Threads (functional, in progress)
    * Multitasking
    * Scheduler
    * System Calls (exit, fork, sleep, log, uptime)
* Tools:
    * Secure Disk Erasure Tool

## Roadmap

* ACPI parsing
* VFS
* AHCI driver
* Better PATA driver

## License

Where not explicitly indicated otherwise, all files are licensed under the following terms:

    Copyright (c) 2012-2013, Keeley Hoek
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
    ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
