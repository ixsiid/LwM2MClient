#include "list.hpp"

#include <stdlib.h>

using namespace LwM2M;

List::List() {
	list = (Item *)malloc(sizeof(Item));

	list->id	 = -1;
	list->next = nullptr;
	list->data = nullptr;

	_count = 0;
}

void *List::find(long id) {
	Item *current = (Item *)list->next;
	while (current != nullptr) {
		if (current->id == id) return current->data;
		current = (Item *)current->next;
	}
	return nullptr;
}

void *List::first() {
	return list->next ? ((Item *)list->next)->data : nullptr;
}

void *List::removeAt(std::function<bool(void *data)> callback) {
	Item *prev    = list;
	Item *current = (Item *)list->next;
	while (current != nullptr) {
		if (callback(current->data)) {
			void *data = current->data;
			prev->next = current->next;
			free(current);
			_count--;
			return data;
		}
		current = (Item *)current->next;
	}
	return nullptr;
}

void *List::add(long id, void *data) {
	Item *current = list;
	while (current->next != nullptr) {
		current = (Item *)current->next;
		if (current->id == id) {
			void *removeData = current->data;
			current->data	  = data;
			return removeData;
		}
	}
	Item *x	    = (Item *)malloc(sizeof(Item));
	x->id	    = id;
	x->next	    = nullptr;
	x->data	    = data;
	current->next = x;

	_count++;
	return nullptr;
}

void *List::remove(long id) {
	Item *current = list;
	while (current->next != nullptr) {
		if (((Item *)current->next)->id == id) {
			Item *removeItem = (Item *)current->next;
			void *removeData = removeItem->data;
			current->next	  = removeItem->next;
			free(removeItem);
			_count--;
			return removeData;
		}
		current = (Item *)current->next;
	}
	return nullptr;
}

void List::all(std::function<void(long id, void *data)> callback) {
	Item *current = (Item *)list->next;
	while (current != nullptr) {
		callback(current->id, current->data);
		current = (Item *)current->next;
	}
}

void List::clear() {
	Item *current = (Item *)list->next;
	while (current != nullptr) {
		Item *next = (Item *)current->next;
		free(current);
		free(current->data);
		current = next;
	}

	list->id	 = -1;
	list->next = nullptr;
	list->data = nullptr;
}