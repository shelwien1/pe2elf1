CXX      := clang++
CXXFLAGS := -Ofast -march=native -fpermissive -s
TARGET   := dummy

$(TARGET): dummy.cpp
	$(CXX) $(CXXFLAGS) dummy.cpp -o $(TARGET)

gcc:
	g++ $(CXXFLAGS) dummy.cpp -o $(TARGET)

clang:
	clang++ $(CXXFLAGS) dummy.cpp -o $(TARGET)

clean:
	rm -f $(TARGET) 1 2

.PHONY: gcc clang clean
