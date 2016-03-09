#pragma once

#include <thread>
#include <chrono>

#include <sstream>

#include <queue>
#include <vector>

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
		std::condition_variable cvar;
	public:
		~Storage() {
			Item *item = nullptr;
			while (items.empty()) {
				item = items.front();
				items.pop();
				delete item;
				item = nullptr;
			}
		}
		bool tryGet(TItem *&item) {
			std::unique_lock<std::mutex> lck(mtx);
			if (items.empty())
				return false;
			
			item = items.front();
			items.pop();
			
			std::ostringstream msg;
			msg << "Consumed: " << item -> id << std::endl;
			std::cout << msg.str();
			
			return true;
		}
		void waitGet(TItem *&item) {
			std::unique_lock<std::mutex> lck(mtx);
			// #1
//			while (items.empty()) {
//				lck.unlock();
//				chrono::seconds secs(3);
//				std::this_thread::sleep_for(secs);
//				lck.lock();
//			}
			// #2
//			while (items.empty())
//				cvar.wait(lck);
			// #3
			if  (items.empty())
				cvar.wait(lck, [this]() {
					return !items.empty();
				});
			
			item = items.front();
			items.pop();
			
			std::ostringstream msg;
			msg << "Consumed: " << item -> id << std::endl;
			std::cout << msg.str();
		}
		void put(TItem *item) {
			std::unique_lock<std::mutex> lck(mtx);
			items.push(item);
			
			std::ostringstream msg;
			msg << "Produced: " << item -> id << std::endl;
			std::cout << msg.str();
			
			cvar.notify_one();
		}
	};
	
	template<typename TItem, template<typename> class TStorage>
	class Worker {
		std::thread job;
	protected:
		int id;
		TStorage<TItem> &storage;
		virtual void work() = 0;
	public:
		Worker(TStorage<TItem> &aStorage, int anId) : storage(aStorage), id(anId) {
		}
		void start() {
			job = std::thread([this]() {
				work();
			});
		}
		void wait() {
			job.join();
		}
	};
	
	template<typename TItem, template<typename> class TStorage>
	class Producer : public Worker<TItem, TStorage> {
		int numToProd;
		Item *produce() {
			chrono::seconds secs(2);
			std::this_thread::sleep_for(secs);
			return Item::createItem();
		}
	protected:
		virtual void work() {
			for (int count = 0; count < numToProd; ++count) {
				Item *item = produce();
				if (item)
					this -> storage.put(item);
			}
		}
	public:
		Producer(TStorage<TItem> &aStorage, int anId, int aNumToProd = 0) : Worker<TItem, TStorage>(aStorage, anId), numToProd(aNumToProd) {}
	};
	
	template<typename TItem, template<typename> class TStorage>
	class Consumer : public Worker<TItem, TStorage> {
		int numToCons;
		void consume(Item *item) {
			if (!item)
				return;
			
			chrono::seconds secs(1);
			std::this_thread::sleep_for(secs);
			delete item;
			item = nullptr;
		}
	protected:
		virtual void work() {
			for (int count = 0; count < numToCons; ++count) {
				TItem *item = nullptr;
				this -> storage.waitGet(item);
				consume(item);
			}
		}
	public:
		Consumer(TStorage<TItem> &aStorage, int anId, int aNumToCons = 0) : Worker<TItem, TStorage>(aStorage, anId), numToCons(aNumToCons) {}
	};
	
	void TestSuite() {
		Storage<Item> storage;
		
		std::vector<Producer<Item, Storage>> producers;
		vector<int> pPlan = {4, 7, 3, 3};
		for (int id = 0; id < pPlan.size(); ++id)
			producers.push_back(Producer<Item, Storage>(storage, id, pPlan[id]));
		
		std::vector<Consumer<Item, Storage>> consumers;
		vector<int> cPlan = {10, 17, 1, 1};
		for (int id = 0; id < cPlan.size(); ++id)
			consumers.push_back(Consumer<Item, Storage>(storage, id, cPlan[id]));

		for (auto &prod : producers)
			prod.start();
		for (auto &cons : consumers)
			cons.start();
		for (auto &prod : producers)
			prod.wait();
		for (auto &cons : consumers)
			cons.wait();
	}
}






















