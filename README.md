# IV/OS Kernel
A GPLv3 microkernel I'm working on. I will eventually get around to making a userspace hopefully, but who knows honestly.

Build instructions ig???
```make clean all``` - cleans the directory and makes the kernel, should only take a couple seconds in it's current state.

```make clean all run-min-kvm``` - runs the OS in QEMU KVM with the minimal amount of features required to run the OS.

```make clean all run-min``` - runs the OS in QEMU TCG with the minimal amount of features, requires QEMU to be compiled with the following features.