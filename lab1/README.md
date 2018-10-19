# Linux character device driver example

## How to use 
- Build the module in vm

```sh
$ vagrant up
$ vagrant ssh # go into vagrant
vagrant~$ cd /vagrant
vagrant~$ make
vagrant~$ make insmod
```

- Test and exit vm
```sh
$ vagrant halt
```