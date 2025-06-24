#This makefile was lifted from the following website:
#http://www.raspberry-projects.com/pi/programming-in-c/compilers-and-ides/geany/creating-a-project
#and modified to fit this project.

#Set all your object files - this will force the corresponding C file to be included in the make
#(the object files of all the .c files in your project, e.g. main.o my_sub_functions.o)
OBJ = 4X3D.o Gerber2.o Image3.o Initialize2.o Job9.o Model_Data13.o Model_UI.o Print3.o Polygon9.o Sensor4.o Settings2.o Slice18.o STLLoader.o System_utils.o Thermal6.o Tool.o Vector10.o Vulcan.o XML2.o

#Set any dependant header files so that if they are edited they cause a complete
#re-compile (e.g. main.h some_subfunctions.h some_definitions_file.h ), or leave blank
DEPS = Global.h

#Any special libraries you are using in your project 
#(e.g. -lbcm2835 -lrt `pkg-config --libs gtk+-3.0` ), or leave blank
LIBS = -lpigpio -lm -pthread `pkg-config --libs gtk4` -lgtk-4 -lvulkan


#Set any compiler flags you want to use 
#(e.g. -I/usr/include/somefolder `pkg-config --cflags gtk+-3.0` ), or leave blank
CFLAGS += -I/usr/include -I/usr/local/include -L/usr/lib `pkg-config --cflags gtk4 gstreamer-1.0`

#Set the compiler you are using ( gcc for C or g++ for C++ )
CC = gcc

#Set the filename extensiton of your C files (e.g. .c or .cpp )
EXTENSION = .c

#define a rule that applies to all files ending in the .o suffix, 
#which says that the .o file depends upon the .c version of the file and 
#all the .h files included in the DEPS macro.  Compile each object file
#gcc `pkg-config --cflags gtk+-3.0` -o example-1 example-1.c `pkg-config --libs gtk+-3.0`
%.o: %$(EXTENSION) $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

#Combine them into the output file
#Set your desired exe output file name here
4X3D: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

#Cleanup
.PHONY: clean

clean:
	rm -f *.o *~ core *~ 
