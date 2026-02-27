CPP             = g++
CXXFLAGS        = -g -O3 -Wall -fPIC -D_REENTRANT -Wno-deprecated

ROOTCFLAGS      := $(shell root-config --cflags)
ROOTGLIBS       := $(shell root-config --glibs)

CXXFLAGS        += $(ROOTCFLAGS)
LIBS            = $(ROOTGLIBS)

ifeq ($(shell uname),Darwin)
ASSIMPLIBS       = $(ASSIMP)/lib/libassimp.dylib
else
ASSIMPLIBS       = $(ASSIMP)/bin/libassimp.so
endif

ASSIMPINC        = $(ASSIMP)/include
LIBS            += -lm $(ASSIMPLIBS)
CXXFLAGS        += -I$(ASSIMPINC)

CPPSRC = Charged.cc Gui.cc Global.cc Geometry.cc Material.cc Mixture.cc Source.cc Triangle.cc Simulator.cc
CPPOBJ = $(CPPSRC:.cc=.o)

DICT_HDR = Gui.hh
DICT_LINKDEF = LinkDef.hh
DICT_SRC = Dict.cc
DICT_OBJ = Dict.o

TARGET = spiros
OBJS = spiros.o

all: $(TARGET)

$(TARGET): $(OBJS) $(CPPOBJ) $(DICT_OBJ)
	@echo "Now making $@"
	@$(CPP) -o $@ $(OBJS) $(CPPOBJ) $(DICT_OBJ) $(CXXFLAGS) $(LIBS)
	@echo "Compile done! \(^o^)/"

# Generate ROOT dictionary
$(DICT_SRC): $(DICT_HDR) $(DICT_LINKDEF)
	@echo "Generating ROOT dictionary..."
	@rootcint -f $@ -c $^

# Compile dictionary
$(DICT_OBJ): $(DICT_SRC)
	@$(CPP) -c $(CXXFLAGS) $<

.cc.o:
	@echo "Compiling object file $<"
	@$(CPP) -c $(CXXFLAGS) $<

clean:
	@echo "Now cleaning up"
	rm -f $(TARGET) *~ *.o *.o~ core $(DICT_SRC) $(DICT_OBJ)
