TARGET  := libLpcAnalyzer.dylib

LINK := -L "../AnalyzerSDK/lib" -lAnalyzer

CC       := clang++
HFILE    := ../../inc/*.h
SRC      := ../src/*.cpp
INC      := -I ../AnalyzerSDK/include/
CXXFLAGS := -Wall -O2 -c
FPIC     := -fPIC
SHARE    := -dynamiclib -o
OBJ      := *.o
cpp		 :=  -std=c++11

$(TARGET) : $(HFILe) $(SRC)
	$(CC) $(CXXFLAGS) $(CPP_STD) $(FPIC) $(SRC) $(INC)
	$(CC) $(SHARE) $(TARGET) $(OBJ) $(LINK)

