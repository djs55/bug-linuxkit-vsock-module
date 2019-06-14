# AF_VSOCK on Hyper-V in v4.14.x bug

There seems to be a bug in v4.14.106 and later linuxkit kernels where `connect` to a
service on the host (e.g. the `vpnkit` ethernet service) fails with `ENODEV`.

## Isolating the problem

Using Docker for Desktop I bisected the v4.14 kernel versions:

| Kernel image                                                            | Working |
| ------------------------------------------------------------------------|---------|
| linuxkit/kernel:4.14.123-d7a38f4a4f25e7148fe5c799946305aee82cc62b-amd64 | ❌     |
| linuxkit/kernel:4.14.74-f6eca5e3144dbd3e5dfa9dcc02dfd52d9ce989d8-amd64  | ✔️     |
| linuxkit/kernel:4.14.99-3bff01b66b00f13d5e2dca04158675bdcc566d1f-amd64  | ✔️     |
| linuxkit/kernel:4.14.111-25e7f5243d0524b80d0d2603362f631ce5eb9511-amd64 | ❌     |
| linuxkit/kernel:4.14.105-3ab75a95aa445399ee532a86a44d41e2c85ec534-amd64 | ✔️     |
| linuxkit/kernel:4.14.108-e5d4620d4ab45eeae5e014c88afb12604447dc1b-amd64 | ❌     |
| linuxkit/kernel:4.14.107-483f6fa535f98256199c9fc91e30836cfe3d4670-amd64 | ❌     |
| linuxkit/kernel:4.14.106-b80b46e6e616e903291697caebc57725446efa58-amd64 | ❌     |

The bisection suggests the fault was introduced between 4.14.105 and 4.14.106.

The only relevant commits are: (from https://lwn.net/Articles/783071/ )

Stefano Garzarella (2):
- [vsock/virtio: fix kernel panic after device hot-unplug](https://github.com/linuxkit/linux/commit/6a78f5dccfff5c0b64b6956e6f6beadd669e0108#diff-c7455644fff4aabad361b451eec8b66e)
- [vsock/virtio: reset connected sockets on device removal](https://github.com/linuxkit/linux/commit/6a78f5dccfff5c0b64b6956e6f6beadd669e0108#diff-c7455644fff4aabad361b451eec8b66e)

Recompiling v1.14.106 with those 2 patches reverted fixes the issue.

Recompiling v1.14.106 with [vsock/virtio: fix kernel panic after device hot-unplug](https://github.com/linuxkit/linux/commit/6a78f5dccfff5c0b64b6956e6f6beadd669e0108#diff-c7455644fff4aabad361b451eec8b66e) reverted fixed the issue.

I rebuilt the v4.14.106 kernel without the revert but switching `AF_VSOCK` to modules: (since the patch mentioned moving code to module init time)
```
diff --git a/kernel/config-4.14.x-x86_64 b/kernel/config-4.14.x-x86_64
index c68eebc..bb6639f 100644
--- a/kernel/config-4.14.x-x86_64
+++ b/kernel/config-4.14.x-x86_64
@@ -1397,10 +1397,10 @@ CONFIG_OPENVSWITCH=m
 CONFIG_OPENVSWITCH_GRE=m
 CONFIG_OPENVSWITCH_VXLAN=m
 CONFIG_OPENVSWITCH_GENEVE=m
-CONFIG_VSOCKETS=y
-CONFIG_VIRTIO_VSOCKETS=y
-CONFIG_VIRTIO_VSOCKETS_COMMON=y
-CONFIG_HYPERV_VSOCKETS=y
+CONFIG_VSOCKETS=m
+CONFIG_VIRTIO_VSOCKETS=m
+CONFIG_VIRTIO_VSOCKETS_COMMON=m
+CONFIG_HYPERV_VSOCKETS=m
 CONFIG_NETLINK_DIAG=y
 CONFIG_MPLS=y
 CONFIG_NET_MPLS_GSO=m
```
and the `connect` works again, as long as the support is dynamically loaded
```
# uname -a
Linux docker-desktop 4.14.106-linuxkit #1 SMP Thu Jun 13 16:19:35 UTC 2019 x86_64 Linux
# lsmod
Module                  Size  Used by    Not tainted
hv_sock                16384 65
vsock                  36864 68 hv_sock
```

## Example: The last known good kernel: v4.14.105
 
First build the helper image:
```
cd detect
docker build -t detect .
```

Second build the .iso:
```
linuxkit-windows-amd64.exe build -format iso-efi .\good.yml
```

Third boot the .iso:
```
linuxkit-windows-amd64.exe run good-efi.iso
```

The VM should probe the `vpnkit` ethernet service and print a message to the console and halt after 5s:
```
Welcome to LinuxKit!

NOTE: This system is namespaced.
The namespace you are currently in may not be the root.
System services are namespaced; to access, use `ctr -n services.linuxkit ...`
linuxkit-00155d0a42c2:~#  _  __                    _   _             _
| |/ /___ _ __ _ __   ___| | (_)___    ___ | | __
| ' // _ \ '__| '_ \ / _ \ | | / __|  / _ \| |/ /
| . \  __/ |  | | | |  __/ | | \__ \ | (_) |   <
|_|\_\___|_|  |_| |_|\___|_| |_|___/  \___/|_|\_\

[   30.603637] reboot: System halted
Console returned: No process is on the other end of the pipe.

                                                             Stop the VM
Remove the VM
```

## Example: The first known broken kernel: v4.14.106

 
First build the helper image:
```
cd detect
docker build -t detect .
```

Second build the .iso:
```
linuxkit-windows-amd64.exe build -format iso-efi .\bad.yml
```

Third boot the .iso:
```
linuxkit-windows-amd64.exe run bad-efi.iso
```

The VM should probe the `vpnkit` ethernet service and print a message to the console and halt after 5s:
```
Welcome to LinuxKit!

NOTE: This system is namespaced.
The namespace you are currently in may not be the root.
System services are namespaced; to access, use `ctr -n services.linuxkit ...`
linuxkit-00155d0a42c3:~# ^[[46;26R[    3.842381] random: crng init done
 _  __                    _   _       _               _                   __
| |/ /___ _ __ _ __   ___| | (_)___  | |__  _ __ ___ | | _____ _ __    _ / /
| ' // _ \ '__| '_ \ / _ \ | | / __| | '_ \| '__/ _ \| |/ / _ \ '_ \  (_) |
| . \  __/ |  | | | |  __/ | | \__ \ | |_) | | | (_) |   <  __/ | | |  _| |
|_|\_\___|_|  |_| |_|\___|_| |_|___/ |_.__/|_|  \___/|_|\_\___|_| |_| (_) |
                                                                         \_\
[   20.506836] reboot: System halted
```



# Conclusion

The patch [vsock/virtio: fix kernel panic after device hot-unplug](https://github.com/linuxkit/linux/commit/6a78f5dccfff5c0b64b6956e6f6beadd669e0108#diff-c7455644fff4aabad361b451eec8b66e) introduced in v4.14.106
broke the Hyper-V `AF_VSOCK` transport when the support is compiled statically. It still works when loaded as a module. Other transports seem unaffected.