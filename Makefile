CXX=g++
CXXFLAGS=-std=c++17 -O2 -Wall -Wextra -pthread

TARGET=pipeline
SRC=src/main.cpp

.PHONY: build run results clean

build:
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

run: build
	./$(TARGET)

results: build
	./$(TARGET) > results.txt
	@echo "Resultados guardados en reCXX=g++
CXXFLAGS=-std=c++17 -O2 -Wall -Wextra -pthread

TARGET=pipeline
SRC=src/main.cpp

.PHONY: build run results clean

build:
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

run: build
	./$(TARGET)

results: build
	./$(TARGET) > results.txt
	@echo "Resultados guardados en results.txt"

clean:
	rm -f $(TARGET) *.csv results.txtsults.txt"

clean:
	rm -f $(TARGET) *.csv results.txtCXX=g++
CXXFLAGS=-std=c++17 -O2 -Wall -Wextra -pthread

TARGET=pipeline
SRC=src/main.cpp

build:
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

run: build
	./$(TARGET)

results: build
	./$(TARGET) > results.txt
	@echo "Resultados guardados en results.txt"

clean:
	rm -f $(TARGET) *.csv results.txt
