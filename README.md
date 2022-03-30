# yaw: effortless package management

Yaw is a nautical term that describes the six degrees of freedom that a ship,
boat or any other craft can experience. Similarly, UNIX package management can
be broken into six core operations, which describe how a user can interact with
a package: creation, building, installation, verification, inspection and  
removal. <code>yaw</code>, the package manager, attempts to accomplish these 
tasks with as little "fluff" as possible.

![yaw](https://upload.wikimedia.org/wikipedia/commons/5/55/USS_Langley_%28CVL-27%29_and_battleship_in_typhoon_1944.jpeg)

<code>yaw</code> is considered a minimal viable product; therefore, it is up to
the user to expand the functionality.  Included is a barebones RFC 1952
compliant tarball (*.tar.gz) expander and web socket downloader (HTTP only). 
These basic utilities allow <code>yaw</code> to be self-sufficient and handle 
most Github project repositories.

There is no infrastructure, community or central repositories associated with 
his package manager. Instead, the user intended to be self-sufficient and the 
repository to be hosted local to the target machine.

In order to manage the actual package build process, <code>yaw</code> uses 
shell script to specify the appropriate commands. The built binaries (located 
in <code>YAW_PATH</code>) are then symbolically linked to the target 
installation path. 

Package validation is performed by generating a checksum using the CRC32
algorithm and verifying that the downloaded tarball checksum matches the source
tarball checksum. If checksum verification fails at the time of building, the
user will not be allowed to proceed with installation.

## Installaton

Using a POSIX C compiler:

	$ git clone http://github.com/mcpcpc/yaw
	$ cd ./yaw
	$ make
	$ make install

## Usage

```
yaw -n [name] [source] [version] bootstraps a new package based on the given 
                                 input parameters, where [name] is the package 
                                 description, [source] is the URL to the 
                                 package tarball (*.tar.gz) file, and [version] 
                                 is the current tarball version
yaw -b [name]                    builds the target package binaries
yaw -c [name]                    generates a checksum for the target package
yaw -i [name]                    installs the target package binaries
yaw -r [name]                    un-installs the target ackage binaries
yaw -l                           prints the current installed packages
yaw -v                           prints the current yaw version
```

## Configuration

All yaw system defaults can be overwritten by specifying the following 
environment variables:

```
YAW_PATH                         working directory for project files 
                                 (default: `$HOME/.yaw`)
```

## Licensing

Free and (forever) open source under the MIT License.

## Contact

For any questions or concerns, please contact me using the information
provided below. 

```
author:     Michael Czigler
email:      michaelczigler [at] mcpcpc [dot] com
public key: https://mcpcpc.github.io/gpg.txt
irc:        libera.chat (user: mcpcpc)
donations:  https://www.paypal.com/paypalme/mcpcpc/
```
