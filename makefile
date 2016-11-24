TARGET := asteroids

CPP_FILES := $(wildcard *.cpp)
OBJ_FILES := $(CPP_FILES:.cpp=.o)

CC_FLAGS := -I/Library/Frameworks/SDL2.framework/Headers -std=c++0x
LD_FLAGS := -F/Library/Frameworks -framework SDL2 -framework OpenGL -std=c++0x 

all: $(TARGET)

%.o: %.cpp
	clang++ $(CC_FLAGS) -o $@ -c $<

$(TARGET): $(OBJ_FILES)
	clang++ $(LD_FLAGS) -o $(TARGET) $(OBJ_FILES)

client: $(TARGET)
	@./$(TARGET) "client"

server: $(TARGET)
	@./$(TARGET) "server"

clean:
	rm $(TARGET) $(OBJ_FILES)
