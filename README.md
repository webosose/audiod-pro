audiod
===========

Summary
-------
AudioD is the audio deamon which controls and configures audio.

Dependencies
============

Below are the tools (and their minimum versions) required to build audiod:

- cmake (version 2.8.7 required by webosose/cmake-modules-webos)
- gcc 4.6.3
- glib-2.0 2.32.1
- make (any version)
- webosose/cmake-modules-webos 1.0.0 RC2
- webosose/libpbnjson 2.15.0
- webosose/libpmloglib 3.0.0
- webosose/libpulse 9.0
- webosose/liburcu 0.9.0
- webosose/lttng-ust 2.8.0
- webosose/luna-prefs 3.0.0
- webosose/luna-service2 3.0.0
- pkg-config 0.26

How to Build on Linux
=====================

## Building

Once you have downloaded the source, enter the following to build it (after
changing into the directory under which it was downloaded):

    $ mkdir BUILD
    $ cd BUILD
    $ cmake ..
    $ make
    $ sudo make install

The directory under which the files are installed defaults to `/usr/local/webos`.
You can install them elsewhere by supplying a value for `WEBOS_INSTALL_ROOT`
when invoking `cmake`. For example:

    $ cmake -D WEBOS_INSTALL_ROOT:PATH=$HOME/projects/webosose ..
    $ make
    $ make install

will install the files in subdirectories of `$HOME/projects/webosose`.

Specifying `WEBOS_INSTALL_ROOT` also causes `pkg-config` to look in that tree
first before searching the standard locations. You can specify additional
directories to be searched prior to this one by setting the `PKG_CONFIG_PATH`
environment variable.

If not specified, `WEBOS_INSTALL_ROOT` defaults to `/usr/local/webos`.

To configure for a debug build, enter:

    $ cmake -D CMAKE_BUILD_TYPE:STRING=Debug ..

To see a list of the make targets that `cmake` has generated, enter:

    $ make help

## Uninstalling

From the directory where you originally ran `make install`, enter:

    $ [sudo] make uninstall

You will need to use `sudo` if you did not specify `WEBOS_INSTALL_ROOT`.

Limitation
==============

1) AudioD is only build as library right now.
2) To buid as executable it has dependencies on PulseAudio and ALSA.
Cmake conversion of these modules is planned in near future.

Copyright and License Information
=================================
Unless otherwise specified, all content, including all source code files and
documentation files in this repository are:

Copyright (c) 2002-2018 LG Electronics, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

SPDX-License-Identifier: Apache-2.0
