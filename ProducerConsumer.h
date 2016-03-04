#pragma once

#include <thread>
#include <chrono>

#include <sstream>

#include <queue>

namespace ProducerConsumer {
	class Item {
	public:
		int id;
		Item(int anId = -1) {
			id = anId;
		}
		static Item *createItem() {
			static int count = 0;
			return new Item(count++);
		}
	};
	
	template<typename TItem>
	class Storage {
		std::queue<TItem *> items;
		std::mutex mtx;
		bool empty() {
			return items.empty();
		}
	public:
		bool get(TItem *&item) {
			std::unique_lock<std::mutex> lck(mtx);
			if (empty())
				return false;
			item = items.front();
			items.pop();
			
			std::ostringstream msg;
			msg << "Item: "
			<< item -> id
			<< " *Consumed*"
			<< std::endl;
			std::cout << msg.str();
			
			return true;
		}
		void put(TItem *item) {
			std::unique_lock<std::mutex> lck(mtx);
			items.push(item);
			
			std::ostringstream msg;
			msg << "Item: "
			<< item -> id
			<< " *Produced*"
			<< std::endl;
			std::cout << msg.str();
		}
	};
	
	template<typename TItem, template<typename> class TStorage>
	class Worker {
		std::thread job;
	protected:
		TStorage<TItem> &storage;
		virtual void work() = 0;
	public:
		Worker(TStorage<TItem> &aStorage) : storage(aStorage) {
		}
		void start() {
			job = std::thread([this](){
				work();
			});
		}
		void wait () {
			job.join();
		}
	};
	
	template<typename TItem, template<typename> class TStorage>
	class Producer : public Worker<TItem, TStorage> {
		int numToProd;
	protected:
		virtual void work() {
			for (int count = 0; count < numToProd; ++count) {
				Item *item = Item::createItem();
				
				chrono::seconds secs(1);
				std::this_thread::sleep_for(secs);
				
				this -> storage.put(item);
			}
		}
	public:
		Producer(TStorage<TItem> &aStorage, int aNumToProd = 0) : Worker<TItem, TStorage>(aStorage), numToProd(aNumToProd) {}
	};
	
	template<typename TItem, template<typename> class TStorage>
	class Consumer : public Worker<TItem, TStorage> {
		int numToCons;
	protected:
		virtual void work() {
			for (int count = 0; count < numToCons;) {
				TItem *item = nullptr;
				if (this -> storage.get(item)) {
					++count;
					
					chrono::seconds secs(2);
					std::this_thread::sleep_for(secs);

					delete item;
					item = nullptr;
				}
			}
		}
	public:
		Consumer(TStorage<TItem> &aStorage, int aNumToCons = 0) : Worker<TItem, TStorage>(aStorage), numToCons(aNumToCons) {}
	};
	
	void TestSuite() {
		int numOfItems = 10;
		Storage<Item> storage;
		Producer<Item, Storage> producer(storage, numOfItems);
		Consumer<Item, Storage> consumer(storage, numOfItems);
		producer.start();
		consumer.start();
		producer.wait();
		consumer.wait();
	}
}