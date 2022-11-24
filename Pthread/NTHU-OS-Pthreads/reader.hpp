#include <fstream>
#include "thread.hpp"
#include "ts_queue.hpp"
#include "item.hpp"

#ifndef READER_HPP
#define READER_HPP

class Reader : public Thread {
public:
	// constructor
	Reader(int expected_lines, std::string input_file, TSQueue<Item*>* input_queue);

	// destructor
	~Reader();

	virtual void start() override;
private:
	// the expected lines to read,
	// the reader thread finished after input expected lines of item
	int expected_lines;

	std::ifstream ifs;
	TSQueue<Item*>* input_queue;

	// the method for pthread to create a reader thread
	static void* process(void* arg);
};

// Implementaion start

Reader::Reader(int expected_lines, std::string input_file, TSQueue<Item*>* input_queue)
	: expected_lines(expected_lines), input_queue(input_queue) {
	ifs = std::ifstream(input_file);
}

Reader::~Reader() {
	ifs.close();
}

void Reader::start() {
	pthread_create(&t, 0, Reader::process, (void*)this);
}

void* Reader::process(void* arg) {
	Reader* reader = (Reader*)arg;

	while (reader->expected_lines--) {
		Item *item = new Item;
		reader->ifs >> *item;
		reader->input_queue->enqueue(item);
		// std::cout << "reader: " << reader->expected_lines << "\n";
	}

	return nullptr;
}

#endif // READER_HPP
