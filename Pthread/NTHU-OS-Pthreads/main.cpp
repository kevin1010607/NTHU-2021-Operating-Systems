#include <assert.h>
#include <stdlib.h>
#include "ts_queue.hpp"
#include "item.hpp"
#include "reader.hpp"
#include "writer.hpp"
#include "producer.hpp"
#include "consumer_controller.hpp"

#define READER_QUEUE_SIZE 200
#define WORKER_QUEUE_SIZE 200
#define WRITER_QUEUE_SIZE 4000
#define CONSUMER_CONTROLLER_LOW_THRESHOLD_PERCENTAGE 20
#define CONSUMER_CONTROLLER_HIGH_THRESHOLD_PERCENTAGE 80
#define CONSUMER_CONTROLLER_CHECK_PERIOD 1000000

int main(int argc, char** argv) {
	assert(argc == 4);
   
	int n = atoi(argv[1]);
	std::string input_file_name(argv[2]);
	std::string output_file_name(argv[3]);
  
  // std::cout<<"CONSUMER_CONTROLLER_CHECK_PERIOD"<<CONSUMER_CONTROLLER_CHECK_PERIOD<<std::endl;
  // std::cout<<"CONSUMER_CONTROLLER_LOW_THRESHOLD_PERCENTAGE"<<CONSUMER_CONTROLLER_LOW_THRESHOLD_PERCENTAGE<<std::endl;
  // std::cout<<"CONSUMER_CONTROLLER_HIGH_THRESHOLD_PERCENTAGE"<<CONSUMER_CONTROLLER_HIGH_THRESHOLD_PERCENTAGE<<std::endl; 
  // std::cout<<"WORKER_QUEUE_SIZE"<<WORKER_QUEUE_SIZE<<std::endl;
  // std::cout<<"  WRITER_QUEUE_SIZE "<< WRITER_QUEUE_SIZE<<std::endl;
  // std::cout<<"  READER_QUEUE_SIZE "<< READER_QUEUE_SIZE<<std::endl;
	// TODO: implements main function
	TSQueue<Item*> reader_queue(READER_QUEUE_SIZE);
	TSQueue<Item*> worker_queue(WORKER_QUEUE_SIZE);
	TSQueue<Item*> writer_queue(WRITER_QUEUE_SIZE);
	Transformer transformer;

	Reader reader(n, input_file_name, &reader_queue);
	Writer writer(n, output_file_name, &writer_queue);
	Producer producer1(&reader_queue, &worker_queue, &transformer);
	Producer producer2(&reader_queue, &worker_queue, &transformer);
	Producer producer3(&reader_queue, &worker_queue, &transformer);
	Producer producer4(&reader_queue, &worker_queue, &transformer);
	ConsumerController consumerController(
		&worker_queue, &writer_queue, &transformer, 
		CONSUMER_CONTROLLER_CHECK_PERIOD, 
		CONSUMER_CONTROLLER_LOW_THRESHOLD_PERCENTAGE, 
		CONSUMER_CONTROLLER_HIGH_THRESHOLD_PERCENTAGE);
  
	reader.start();
	writer.start();
	producer1.start();
	producer2.start();
	producer3.start();
	producer4.start();
	consumerController.start();

	reader.join();
	writer.join();

	return 0;
}
