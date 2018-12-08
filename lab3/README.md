# Linux kernel-space traffic interceptor

## Build and run

```sh
$ vagrant up
$ vagrant ssh
$ cd /vagrant
$ make
$ make insmod
```

Check that the module successfully loaded

```sh
$ dmesg # look at 'network_driver' messages
```

Or

```
$ lsmod | grep driver
```

## Usage

This module:
- captures all UDP traffic from all interfaces and prints source and destination ports. To test it, try:
  ```sh
  $ nc -u localhost 32 < ${FILE_WITH_DATAGRAM_CONTENT}
  $ dmesg
  ```
  > Note: this module handle only `32` destination port. All other ports will be ignored. (See `DRV_TARGET_PORT` define)

## Useful articles/docs

- Detailed `sk_buff` description [here](http://vger.kernel.org/~davem/skb_data.html) and [here]( http://vger.kernel.org/~davem/skb.html)
- How to use `sk_buff` with new kernel (4.15) [here](https://www.paulkiddie.com/2009/11/creating-a-netfilter-kernel-module-which-filters-udp-packets/)
- Useful code snippet for old kernel [here](https://anukulverma.wordpress.com/2016/11/01/network-packet-capturing-for-linux-kernel-module/)