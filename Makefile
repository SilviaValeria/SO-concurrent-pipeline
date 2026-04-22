CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -pthread

TARGET = pipeline
SRC = src/main.cpp

.PHONY: build run results clean docker-build docker-run

build:
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

run: build
	./$(TARGET)

results: build
	./$(TARGET) > results.txt
	@echo "Resultados guardados en results.txt"

clean:
	rm -f $(TARGET) *.csv results.txt

docker-build:
	docker build --no-cache -t so-pipeline .

docker-run:
	docker run --rm so-pipeline
