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

#Home computer && Aviz one
#CPPFLAGS = -I. -isystem /usr/local/include/vtk-6.3/
#	LDFLAGS = -lpthread -lSDL2 -lGL -lpng -L /usr/local/lib/ -Wl,-rpath=/usr/local/lib/ -lvtkIOXML-6.3 -lvtkIOLegacy-6.3 -lvtkIOPLY-6.3 -lvtkIOGeometry-6.3 -lvtkFiltersModeling-6.3 -lvtkImagingCore-6.3 -lvtkRenderingFreeType-6.3 -lvtkRenderingCore-6.3 -lvtkIOImage-6.3 -lvtkDICOMParser-6.3 -lvtkmetaio-6.3 -lvtkpng-6.3 -lvtktiff-6.3 -lvtkjpeg-6.3 -lvtkFiltersSources-6.3 -lvtkFiltersGeometry-6.3 -lvtkIOXMLParser-6.3 -lvtkIOCore-6.3 -lvtkexpat-6.3 -lvtkFiltersExtraction-6.3 -lvtkFiltersGeneral-6.3 -lvtkFiltersCore-6.3 -lvtkCommonExecutionModel-6.3 -lvtkCommonComputationalGeometry-6.3 -lvtkCommonDataModel-6.3 -lvtkCommonMisc-6.3 -lvtkCommonTransforms-6.3 -lvtkCommonSystem-6.3 -lvtkCommonMath-6.3 -lvtkCommonCore-6.3 -lvtksys-6.3 -lvtkfreetype-6.3 -lvtkzlib-6.3




	
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
