OUTPUT = test

SOURCES = $(wildcard *.cpp) $(wildcard */*.cpp) $(wildcard */*.c)
OBJECTS = $(patsubst %.c, %.o, $(patsubst %.cpp, %.o, $(SOURCES)))

VERBOSE = 0
CC_O     = [CC ]
CPP_O    = [C++]
LD_O     = [LD ]

CC = gcc
CXX = g++

OS=$(shell lsb_release -si)
ARCH=$(shell uname -m | sed 's/x86_//;s/i[3-6]86/32/')
VER=$(shell lsb_release -sr)
$(info OS= $(OS))

FLAGS = -O0 -fpermissive -Wall -Wextra -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-parameter -Wno-deprecated -pedantic -ggdb
CFLAGS = -std=c99
CXXFLAGS = -std=c++0x



	
	CPPFLAGS = -I. -isystem /usr/include/vtk-6.0/
	LDFLAGS = -lpthread -lSDL2 -lGL -lpng -lvtkIOXML-6.0 -lvtkIOLegacy-6.0 -lvtkIOPLY-6.0 -lvtkIOGeometry-6.0 -lvtkFiltersModeling-6.0 -lvtkImagingCore-6.0 -lvtkRenderingFreeType-6.0 -lvtkRenderingCore-6.0 -lvtkIOImage-6.0 -lvtkDICOMParser-6.0 -lvtkmetaio-6.0 -lvtkFiltersSources-6.0 -lvtkFiltersGeometry-6.0 -lvtkIOXMLParser-6.0 -lvtkIOCore-6.0 -lvtkFiltersExtraction-6.0 -lvtkFiltersGeneral-6.0 -lvtkFiltersCore-6.0 -lvtkCommonExecutionModel-6.0 -lvtkCommonComputationalGeometry-6.0 -lvtkCommonDataModel-6.0 -lvtkCommonMisc-6.0 -lvtkCommonTransforms-6.0 -lvtkCommonSystem-6.0 -lvtkCommonMath-6.0 -lvtkCommonCore-6.0 -lvtksys-6.0 



ifeq ($(VERBOSE),1)
	QUIET = @\#
	VERBOSE =
	VERBOSE_OUTPUT =
else
	QUIET = @echo " "
	VERBOSE = @
	VERBOSE_OUTPUT = $(SILENT)
endif

all: $(OUTPUT)

$(OUTPUT): $(OBJECTS)
	$(QUIET)$(LD_O) $@
	$(VERBOSE)$(CXX) $^ $(LDFLAGS) -o $@

%.o: %.cpp
	$(QUIET)$(CPP_O) $@
	$(VERBOSE)$(CXX) -c $< $(FLAGS) $(CXXFLAGS) $(CPPFLAGS) -o $@

%.o: %.c
	$(QUIET)$(CPP_O) $@
	$(VERBOSE)$(CC) -c $< $(FLAGS) $(CFLAGS) $(CPPFLAGS) -o $@

clean:
	rm -f *.o $(OUTPUT)
