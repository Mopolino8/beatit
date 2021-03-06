############################################
#   Beat It     0.1
############################################

Install instructions:

Setup your project folder:

    BeatIt/
        beatit  (git repository)
        build   (build folder)
        install (prefix folder)

Copy the configure.sh file  from the git repository in your build folder.

BeatIt/build$ cp ../beatit/do-configure.sh my-configure.sh

Edit the my-configure.sh file to set your own libraries

From your build folder run
BeatIt/build$ sh my-configure.sh

############################################
#
#   Linking Error With Libmesh
#
############################################
The default debug mode in libmesh adds the _GLIBCXX_DEBUG flag.
If you compile libmesh with the option --disable-glibcxx-debugging
you need to remove the flag from the CMakelists.txt file in 
the base BeatIt directory.

############################################
#
#   VTK CONFIGURATION
#
############################################
#Specify VTK Path
VTK=/path/to/base/vtk/folder/vtk
#Specify Version
#VERSION=7.1.1
VERSION=X.X.X
cmake \
   -D CMAKE_INSTALL_PREFIX=/path/to/install/folder \
   -D CMAKE_CXX_COMPILER=mpicxx \
   -D CMAKE_C_COMPILER=mpicc \
   -D VTK_Group_MPI:BOOL=ON \
  $VTK/VTK-$VERSION
