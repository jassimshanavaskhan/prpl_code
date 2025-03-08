# tr181-dslite

## Introduction

This is an Ambiorix plug-in for a TR-181 compatible DSLite manager.

## Building

### Prerequisites

- [libamxc](https://gitlab.com/prpl-foundation/components/ambiorix/libraries/libamxc)
- [libamxp](https://gitlab.com/prpl-foundation/components/ambiorix/libraries/libamxp)
- [libamxd](https://gitlab.com/prpl-foundation/components/ambiorix/libraries/libamxd)
- [libamxb](https://gitlab.com/prpl-foundation/components/ambiorix/libraries/libamxb)
- [libamxo](https://gitlab.com/prpl-foundation/components/ambiorix/libraries/libamxo)
- [libamxm](https://gitlab.com/prpl-foundation/components/ambiorix/libraries/libamxm)
- [libsahtrace](https://gitlab.com/soft.at.home/network/libsahtrace)
- [libnetmodel](https://gitlab.com/prpl-foundation/components/core/libraries/libnetmodel)
- libc-ares-dev

You can install these libraries from source or using their debian packages. To install them from source, refer to their corresponding repositories for more information.
To install them using debian packages, you can run

```bash
sudo apt update
sudo apt install sah-lib-sahtrace-dev libamxc libamxp libamxm libamxd libamxb libamxo libnetmodel libc-ares-dev
```
### Build and install tr181-dslite

1. Clone the repository

    To be able to build it, you need the source code. So open the desired target directory and clone this plug-in in it.

    ```bash
    mkdir ~/workspace/amx/plugins
    cd ~/workspace/amx/plugins
    git clone git@gitlab.com:prpl-foundation/components/core/plugins/tr181-dslite.git
    ``` 

1. Build it

    When using the internal gitlab, you must define an environment variable `VERSION_PREFIX` before building.

    ```bash
    export VERSION_PREFIX="master_"
    ```

    After the variable is set, you can build the plug-in.

    ```bash
    cd ~/workspace/amx/plugins/tr181-dslite
    make
    ```
1. Install it

    You can use the install target in the makefile to install the plug-in

    ```bash
    cd ~/workspace/amx/plugins/tr181-dslite
    sudo -E make install
    ```

### Running the plug-in

During installation a symbolic link is created to amxrt:

```text
/usr/bin/tr181-dslite -> /usr/bin/amxrt
```

This allows you to run the IP manager using the `tr181-dslite` command. `amxrt` will find the relevant odl files in `/etc/amx/tr181-dslite`. In the current configuration (see `odl/tr181-dslite.odl`) all files from the directory `/etc/amx/tr181-dslite/defaults` are loaded on startup of the TR181-DSLite plug-in. You can add your own odl files here if you want to add your own `DSLite.InterfaceSetting.` instances.