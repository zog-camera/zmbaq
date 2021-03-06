ZOG Zmbaq : ultimately buggy code at the moment. It's under re-writing process.
It's under development and needs the goals of the project to be documented instead of being
"spherical horse inbound into 4-dimensional cube".

It pretends to be soon an open source video surveillance system -- a server for incoming video streams,
GUI client for PC and R.Pi, R.Pi h264 video streamer.

Database design in progress, not finished yet.
![ZMBAQ prototype of DB scheme](https://github.com/zog-camera/zmbaq/blob/master/docs/db.png)

Internal implementation of the software may be denoted as the following scheme
 (it's not electrical scheme, just using that symbols for some processes like data copying, logic, some processing)
![ZMBAQ internals scheme](https://github.com/zog-camera/zmbaq/blob/master/docs/zmbaq_internal.png)

At the moment, there is only 1 working piece: a small utility "rtsp_dumper" that dumps RTSP/RTP(from .sdp file) H264 streams
to filesystem, just to test existing implementation entities of the C++ code.
Current work is focused on motion detection algorithms via OpenCV, GUI client is next.


Authors: Alexander Sorvilov(ideas,concept, financial aid), Bohdan Maslovskyi(development), Olexii Shevchenko(development).

# Dependencies:
+ libzip
+ libasan
+ libx264 (in system)
+ >= ffmpeg-3.0 (flags: --with-encoder=libx264 --enable-gpl)
+ ZeroMQ 4.x
+ CZMQ 3.x (depends on ZeroMQ4.x)
+ OpenCV 3.x
+ libboost_filesystem, libboost_asio >= 1.55
+ Urho3D game engine for the GUI
+ RethinkDB
+ rapidjson

## Preconditions
   Create a directory for the dependencies installation
  (let's name it "broot"). Install Git, CMake, CMake-GUI, MSVC (if needed)
```BASH
	mkdir "C:\Users\Max\broot" #windows
	mkdir $HOME/broot #linux
    export BROOT=$HOME/broot # dependencies directory
```

### submodules checkout
```BASH
cd external/
git submodule init avcpp
git submodule update avcpp
```

### FFMPEG
+ For linux:
> sudo apt-get install libx264-dev

```BASH
wget http://ffmpeg.org/releases/ffmpeg-snapshot-git.tar.bz2
tar xvf ffmpeg-snapshot-git.tar.bz2
cd ffmpeg
./configure --prefix=$HOME/broot --enable-gpl --enable-libx264 --enable-decoder=h264
```
+ For MacOS
```BASH
brew install ffmpeg
```
+ For Windows:
Download from http://ffmpeg.zeranoe.com/builds/ and deploy libraries into "broot"

### ZeroMQ 4.x http://zeromq.org
https://github.com/zeromq/libzmq
+ Linux
```BASH
   # linux:
   git clone https://github.com/zeromq/libzmq.git
   ./autogen.sh
   ./configure --prefix=$HOME/broot
   make && make install
```

+ MacOS
```
brew install zmq czmq
```

+ Windows
```
   # get the build from http://zeromq.org/distro:microsoft-windows (latest 4.x branch)
```

### CZMQ 3.x (depends on ZeroMQ)
CZMQ 3.x(more modern ZMQ C API):
	http://czmq.zeromq.org/page:get-the-software

> http://download.zeromq.org/czmq-3.0.0-rc1.zip  #for windows

CZMQ will probably look for ZMQ at /usr/local prefix.

### Download and install BOOST libraries
  For windows: install Boost libraries into "broot" directory.
  Linux/MacOS: install libboost_system >=1.55 libboost_thread >= 1.55
  using your package management system or build it.


### Install or compile OpenCV
http://opencv.org
Should be deployed in "broot" directory, for example: "C:\Users\Max\broot"

+ Windows:
> http://sourceforge.net/projects/opencvlibrary/files/opencv-win/3.0.0-beta/
> #deploy it into "broot" directory

+Linux
```
git clone https://github.com/Itseez/opencv.git
cd opencv
mkdir build
cmake .. -DCMAKE_INSTALL_PREFIX=$HOME/broot
make -j2 && make install
```

+ MacOS
```
brew tap homebrew/science
brew install opencv3
```
### Download and build RethinkgDB C++ driver
```
git clone https://github.com/AtnNn/librethinkdbxx.git
cd librethinkdbxx
make
#install to our BROOT directory
cp -R build/include/* $BROOT/include
cp -R build/lib* $BROOT/lib
```

### Compile the project:
> cd zmbaq
> mkdir build && cd build
> cmake .. -DDEPENDS_ROOT=$HOME/broot




