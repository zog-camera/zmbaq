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
+ libzip (in system)
+ libasan (bundled with the GCC,Clang compilers)
+ libx264 (in system(*.deb, HomeBrew package))
+ yasm (in system(*.deb, HomeBrew package), for ffmpeg)
+ >= ffmpeg-3.0 (flags: --with-encoder=libx264 --enable-gpl) [https://ffmpeg.org]
+ libSodium [git://github.com/jedisct1/libsodium.git]
+ ZeroMQ 4.x [git://github.com/zeromq/libzmq.git]
+ CZMQ 3.x (depends on ZeroMQ4.x) [git://github.com/zeromq/czmq.git]
+ Malamute (depends on CZMQ 3.x, libSodium) [git://github.com/zeromq/malamute.git]
+ OpenCV 3.x [https://github.com/Itseez/opencv.git]
+ libboost_filesystem, libboost_asio >= 1.55
+ Urho3D game engine for the GUI [https://github.com/urho3d/Urho3D.git]
+ RethinkDB [https://www.rethinkdb.com] [https://github.com/AtnNn/librethinkdbxx.git]
+ rapidjson [http://rapidjson.org/md_doc_tutorial.html][https://github.com/Tencent/rapidjson]
+ libv4l2cpp (depends on log4cpp) [https://github.com/mpromonet/libv4l2cpp.git]

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
### download thirdparty repositories into ./external:
for item in rapidjson glm libsodium libzmq czmq malamute avcpp opencv librethinkdbxx libv4l2cpp Urho3D; do git submodule init $item && git submodule update $item; done;
```
repositories sources:
### Build FFMPEG
+ For linux:
> sudo apt-get install libx264-dev

```BASH
wget http://ffmpeg.org/releases/ffmpeg-snapshot-git.tar.bz2
tar xvf ffmpeg-snapshot-git.tar.bz2
cd ffmpeg
./configure --prefix=$HOME/broot --enable-shared --enable-gpl --enable-libx264 --enable-decoder=h264
make -j$(nproc) install
```
+ For MacOS
```BASH
brew install ffmpeg
```
+ For Windows:
Download from http://ffmpeg.zeranoe.com/builds/ and deploy libraries into "broot"

### Malamute message broker with dependencies: CZMQ,ZeroMQ,Sodium
First, install the following configuration tools into your system: m4, automake
Then compile *Malamute* with the dependencies
```
for project in libsodium libzmq czmq malamute; do
    cd $project
    ./autogen.sh
    ./configure --prefix=$BROOT && make check
    make install
    ldconfig
    cd ..
done
```


### Download and install BOOST libraries
  For windows: install Boost libraries into "broot" directory.
  Linux/MacOS: install libboost_system >=1.55 libboost_thread >= 1.55
  using your package management system or build it.

Ubuntu:
> apt-get install libboost-filesystem-dev libboost-thread-dev libboost-coroutine-dev

MacOS:
> brew install boost

### Install or compile OpenCV
http://opencv.org
Should be deployed in "broot" directory, for example: "C:\Users\Max\broot"

+ Windows:
> http://sourceforge.net/projects/opencvlibrary/files/opencv-win/3.0.0-beta/

Then deploy it into "broot" directory

+Linux
```
cd ./external/opencv
mkdir build
cmake .. -DCMAKE_INSTALL_PREFIX=$HOME/broot
make -j$(nproc) && make install
```

+ MacOS
```
brew tap homebrew/science
brew install opencv3
```
### RethinkDB
Install RethinkDB server from https://www.rethinkdb.com/

Download and build RethinkgDB C++ driver
```
cd ./external/librethinkdbxx
make -j$(nproc)
#install to our BROOT directory
cp -R build/include/* $BROOT/include
cp -R build/lib* $BROOT/lib
```

### Compile Urho3D engine:
#### System dependencies
Ubuntu Linux:
> sudo apt-get install libx11-dev libgl1-mesa-dev # for Ubuntu or similar package for others

#### Compilation

```
cd ./external/Urho3D
mkdir build
cmake -DCMAKE_INSTALL_PREFIX=$BROOT && make -j$(nproc) install
```
### (Linux only) Dependency for libv4l2pp:
#### Log4Cpp 
> sudo apt-get install liblog4cpp5-dev

### Compile the project:
> cd zmbaq
> mkdir build && cd build
> cmake .. -DDEPENDS_ROOT=$HOME/broot




