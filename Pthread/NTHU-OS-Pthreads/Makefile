CXX = g++
CXXFLAGS = -static -std=c++11 -O3
LDFLAGS = -pthread
TARGETS = main reader_test producer_test consumer_test writer_test ts_queue_test
DEPS = transformer.cpp

.PHONY: all
all: $(TARGETS)

.PHONY: docker-build
docker-build:
	docker-compose run --rm build

.PHONY: clean
clean:
	rm -f $(TARGETS)

%: %.cpp $(DEPS)
	$(CXX) -o $@ $(CXXFLAGS) $(LDFLAGS) $^
