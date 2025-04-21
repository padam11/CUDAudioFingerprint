# Compiler definition
#CC = g++

# Compilation flags
#CC_FLAGS = -std=c++11 -O2 -c

# Linker flags (no CUDA libs needed anymore)
#LD_FLAGS = 

# Executable and object files (CUDA removed)
#EXE = identify-audio
#OBJ = identify-audio.o cpu_helpers.o audiodatabase.o AudioFile.o

# Default target
#default: $(EXE)

# C++ source compilation
#cpu_helpers.o: ../common/cpu_helpers.cpp
#	$(CC) $(CC_FLAGS) -o $@ $<

#audiodatabase.o: ../common/audiodatabase.cpp
#	$(CC) $(CC_FLAGS) -o $@ $<

#AudioFile.o: ../common/AudioFile.cpp
#	$(CC) $(CC_FLAGS) -o $@ $<

#identify-audio.o: main.cpp
#	$(CC) $(CC_FLAGS) -o $@ $<

# Final link step using g++
#$(EXE): $(OBJ)
#	$(CC) $(OBJ) -o $(EXE) $(LD_FLAGS)
#	make clean_temp

# Temporary cleanup
#clean_temp:
#	del /Q *.o

# Full cleanup
#clean:
#	del /Q *.o $(EXE).exe

#WSL IMPLEMENTATION BELOW -- COMMENT OUT IN WINDOWS

NVCC        = nvcc
CC          = g++


NVCC_FLAGS  = -I/usr/local/cuda/include -gencode=arch=compute_60,code=\"sm_60\" -lcuda -lcudart -lcufft
CC_FLAGS    = -std=c++11


ifdef dbg
	NVCC_FLAGS  += -g -G
else
	NVCC_FLAGS  += -O2
endif

LD_FLAGS    = -lcudart -L/usr/local/cuda/lib64
EXE	        = identify-audio
OBJ	        = identify-audio.o gpu_helpers.o cpu_helpers.o audiodatabase.o AudioFile.o

default: $(EXE)


gpu_helpers.o: ../common/gpu_helpers.cu
	$(NVCC) -c -o $@ ../common/gpu_helpers.cu $(NVCC_FLAGS)

cpu_helpers.o: ../common/cpu_helpers.cpp
	$(CC) -c -o $@ ../common/cpu_helpers.cpp $(CC_FLAGS)

audiodatabase.o: ../common/audiodatabase.cpp
	$(CC) -c -o $@ ../common/audiodatabase.cpp $(CC_FLAGS)	

AudioFile.o: ../common/AudioFile.cpp
	$(CC) -c -o $@ ../common/AudioFile.cpp $(CC_FLAGS)	

identify-audio.o:  main.cpp
	$(CC) -c -o $@ main.cpp $(CC_FLAGS) 

$(EXE): $(OBJ)
	$(NVCC) $(OBJ) -o $(EXE) $(LD_FLAGS) $(NVCC_FLAGS)
	make clean_temp

clean_temp:
	rm -rf *.o

clean:
	rm -rf *.o $(EXE)