// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "test.h"
#include <climits>
#include <errno.h>
#include <algorithm>

using namespace coyote;
typedef unsigned long long llu;
Scheduler* scheduler = NULL;

// Uncomment this to debug pthread APIs.
//#define DEBUG_PTHREAD_API

/******************************************** CoyoteLock Start ******************************************/

/* This class is intended to model a pthread mutex or a condition variable (condV)
*  using Coyote resources. For every mutrex or condition variable, we should have a unique
*  Coyote resource ID.
*/
class CoyoteLock{
public:

	// Boolean variable to store whether this resource is locked or not.
	bool is_locked;
	// Unique Coyote resource id
	int coyote_resource_id;
	// Counter to keep a track of resource IDs, we have already allocated.
	// We won't be using the same resource ID again, even if the previous
	// resource is deleted.
	static int total_resource_count;
	// Is it a conditional variable?
	bool is_cond_var;
	// Vector of operations waiting for this conditional variable.
	std::vector<size_t>* waitingOps;
	// Who is holding this lock?
	size_t user_op_id;

	// reserved_resource_id_min is used to tell CoyoteLock that there are already
	// existing Coyote resources with IDs less than or equal to reserved_resource_id_min.
	// Use it when your application is moduler and you want to reserve some resource_ids for
	// one module.
	CoyoteLock(int reserved_resource_id_min = -1, int reserved_resource_id_max = INT_MAX, bool is_conditional_var = false){
		assert(scheduler != NULL && "CoyoteLock: please initialize the Coyote scheduler first!\n");

		assert(total_resource_count < reserved_resource_id_max && "CoyoteLock: Can not allocate more resources!");

		// Should only be true once per module
		if(total_resource_count <= reserved_resource_id_min){
			total_resource_count = reserved_resource_id_min + 1;
		}

		coyote_resource_id = total_resource_count;
		total_resource_count ++;

		ErrorCode e = scheduler->create_resource(coyote_resource_id);
		assert(e == ErrorCode::Success && "CoyoteLock: failed to create resource! perhaps it already exists\n");

		is_locked = false;

		// if it is a conditional variable
		if(is_conditional_var){

			is_cond_var = true;
			waitingOps  = new std::vector<size_t>();
			assert(waitingOps != NULL && "CoyoteLock: Unable to allocate on heap!");
		} else {

			// If it is a mutex
			is_cond_var = false;
			waitingOps = NULL;
		}
		user_op_id = 0; // Held by main thread
	}

	~CoyoteLock(){
		assert(scheduler != NULL && "~CoyoteLock: please initialize the Coyote scheduler first!\n");

		if(is_cond_var && (waitingOps != NULL) ){

			assert(waitingOps->empty() &&
				"Some operations are still waiting to be signaled!" &&
				 "Is it valid to destroy a cond_var when operations are waiting on it? No!");

			// If there is no one waiting on this conditional variable, then it is okay to delete it.
			assert( (is_locked == false || waitingOps->empty()) && "Can not delete the resource as it is locked!");

			delete waitingOps;
		} else {

			assert( (is_locked == false) && "Can not delete the resource as it is locked!");
		}

		ErrorCode e = scheduler->delete_resource(coyote_resource_id);
		assert(e == ErrorCode::Success && "~CoyoteLock: failed to delete resource!\n");
	}

	// Call with caution!! Currently, it is being called only during Coyote_scheduler->detach()
	static void reset_resource_count(){
		total_resource_count = 0;
	}
};

int CoyoteLock::total_resource_count = 0;

// Global hash map to store which pointer corresponds to which CoyoteLock object
std::unordered_map<llu, CoyoteLock*>* hash_map = NULL;

// Temporary data structure used for passing parameteres to pthread_create
typedef struct pthread_create_params{
	void *(*start_routine) (void *);
	pthread_barrier_t barrier;
	void* arg;
} pthread_c_params;

// This function will be called in pthread_create
void *coyote_new_thread_wrapper(void *p){

	pthread_c_params* param = (pthread_c_params*)p;
	scheduler->create_operation((long unsigned)pthread_self());
	pthread_barrier_wait(&(param->barrier));

	scheduler->start_operation((long unsigned)pthread_self());

	((param->start_routine))(param->arg);

	scheduler->complete_operation((long unsigned)pthread_self());

	return NULL;
}

/******************************************** CoyoteLock End ******************************************/

/***** Modelling of pthread APIs using Coyote scheduler APIs *****
* Our goal is to provide a drop-in replacement of default pthread APIs.
******************************************************************/

	void FFI_attach_scheduler(){

		assert(scheduler != NULL && "Wrong sequence of API calls. Create Coyote Scheduler first.");

		// Lazy initialization of hash map
		if(hash_map == NULL){
			hash_map = new std::unordered_map<llu, CoyoteLock*>();
		}

		ErrorCode e = scheduler->attach();
		assert(e == coyote::ErrorCode::Success && "FFI_attach_scheduler: attach failed");
	}

	void FFI_detach_scheduler(){

		assert(scheduler != NULL && "Wrong sequence of API calls. Create Coyote Scheduler first.");

		// If hash_map is non-null, clear and destroy it!
		if(hash_map != NULL){

			// Delete all the resources present in the hash map
			for(auto it = hash_map->begin(); it != hash_map->end(); it ++){

				CoyoteLock* obj = (*it).second;
				delete obj;
				obj = NULL;
			}

			hash_map->clear();

			delete hash_map;
			hash_map = NULL;

			CoyoteLock::reset_resource_count();
		}

		ErrorCode e = scheduler->detach();
		assert(e == coyote::ErrorCode::Success && "FFI_detach_scheduler: detach failed");
	}

	// Call our wrapper function instead of original parameters to pthread_create
	int FFI_pthread_create(void *tid, void *attr, void *(*start_routine) (void *), void* arguments){

		pthread_c_params *p = (pthread_c_params *)malloc(sizeof(pthread_c_params));
		p->start_routine = start_routine;
		pthread_barrier_init(&(p->barrier), NULL, 2);
		p->arg = arguments;

		int retval = pthread_create((pthread_t*)tid, (const pthread_attr_t*)attr, coyote_new_thread_wrapper, (void*)p);
		pthread_barrier_wait(&(p->barrier));

		return retval;
	}

	int FFI_pthread_join(pthread_t tid, void** arg){

		scheduler->join_operation((long unsigned) tid);
		return pthread_join(tid, arg);
	}

	int FFI_pthread_mutex_init(void *ptr, void *mutex_attr){

		scheduler->schedule_next();
		assert(mutex_attr == NULL && "Currently, We don't support custom mutex attributes");

	#ifdef DEBUG_PTHREAD_API
		printf("In FFI_pthread_mutex_init: recieved: %p \n", ptr);
	#endif

		llu key = (llu)ptr;
		assert(hash_map->find(key) == hash_map->end() && "FFI_pthread_mutex_init: Key is already in the map\n");

		// Create a new resource object and insert it into the hash map
		CoyoteLock* new_obj = new CoyoteLock();
		bool rv = (hash_map->insert({key, new_obj})).second;
		assert(rv == true && "FFI_pthread_mutex_init: Inserting in the map failed!\n");

	#ifdef DEBUG_PTHREAD_API
		printf("In FFI_pthread_mutex_init: Mapped: %p to Coyote resource id: %d \n", ptr, new_obj->coyote_resource_id);
	#endif

		return 0;
	}

	int FFI_pthread_mutex_lock(void *ptr){

		scheduler->schedule_next();
		assert(hash_map != NULL && "FFI_pthread_mutex_lock: Initialize the hash map first\n");

		llu key = (llu)ptr;
		std::unordered_map<llu, CoyoteLock*>::iterator it = hash_map->find(key);

		// If it is not in the hash map, initialize it. It can be becoz this mutex ptr is globally initialized
		if(it == hash_map->end()){

			FFI_pthread_mutex_init(ptr, NULL);
			it = hash_map->find(key);
		}

		assert(it != hash_map->end() && "FFI_pthread_mutex_lock: key not in map\n");

		CoyoteLock* obj = it->second;

	#ifdef DEBUG_PTHREAD_API
		printf("In FFI_pthread_mutex_lock: Locking on: %p as Coyote resource id: %d \n", ptr, obj->coyote_resource_id);
	#endif

		assert( ( !(obj->is_locked) || scheduler->scheduled_operation_id() != obj->user_op_id ) &&
			"This thread is already holding this lock, why is it trying to lock it again?");

		// If the resource is already locked, then spinlock!
		while(obj->is_locked){
			scheduler->wait_resource(obj->coyote_resource_id);
		}

		// If the resource is free for use, lock it!
		obj->is_locked = true;
		// Who is holding this lock?
		obj->user_op_id = scheduler->scheduled_operation_id();

		return 0;
	}

	int FFI_pthread_mutex_trylock(void *ptr){

		scheduler->schedule_next();
		assert(hash_map != NULL && "FFI_pthread_mutex_trylock: Initialize the hash map first\n");

		llu key = (llu)ptr;
		std::unordered_map<llu, CoyoteLock*>::iterator it = hash_map->find(key);

		// If it is not in the hash map, initialize it
		if(it == hash_map->end()){

			FFI_pthread_mutex_init(ptr, NULL);
			it = hash_map->find(key);
		}

		assert(it != hash_map->end() && "FFI_pthread_mutex_trylock: key not in map\n");

		CoyoteLock* obj = it->second;

	#ifdef DEBUG_PTHREAD_API
		printf("In FFI_pthread_mutex_trylock: Locking on: %p as Coyote resource id: %d \n", ptr, obj->coyote_resource_id);
	#endif

		// If the resource is already locked, then return -1;
		if(obj->is_locked){
			return EBUSY; // 16 is the code for EBUSY in <errno.h>.
		}

		// Otherwise, return 0 and gain the lock
		obj->is_locked = true;
		// How is holding this lock?
		obj->user_op_id = scheduler->scheduled_operation_id();

		return 0;
	}

	int FFI_pthread_mutex_is_lock(void *ptr){

		scheduler->schedule_next();
		assert(hash_map != NULL && "FFI_pthread_mutex_is_lock: Initialize the hash map first\n");

		llu key = (llu)ptr;
		std::unordered_map<llu, CoyoteLock*>::iterator it = hash_map->find(key);

		assert(it != hash_map->end() && "FFI_pthread_mutex_is_lock: key not in map\n");

		CoyoteLock* obj = it->second;

	#ifdef DEBUG_PTHREAD_API
		printf("In FFI_pthread_mutex_is_lock: Locking on: %p as Coyote resource id: %d \n", ptr, obj->coyote_resource_id);
	#endif

		// If the resource is already locked, then return -1;
		if(obj->is_locked){
			return 0;
		} else {
			return -1;
		}
	}

	int FFI_pthread_mutex_unlock(void *ptr){

		scheduler->schedule_next();
		assert(hash_map != NULL && "FFI_pthread_mutex_unlock: Initialize the hash map first\n");

	#ifdef DEBUG_PTHREAD_API
		printf("In FFI_pthread_mutex_unlock: Unlocking on: %p \n", ptr);
	#endif

		llu key = (llu)ptr;
		std::unordered_map<llu, CoyoteLock*>::iterator it = hash_map->find(key);
		assert(it != hash_map->end() && "FFI_pthread_mutex_unlock: key not in map\n");

		CoyoteLock* obj = it->second;

		assert(obj->is_locked == true &&
			 "FFI_pthread_mutex_unlock: Resource wasn't locked before calling this function");

		obj->is_locked = false;

	#ifdef DEBUG_PTHREAD_API
		printf("In FFI_pthread_mutex_unlock: Unlocking on: %p as Coyote resource id: %d \n", ptr, obj->coyote_resource_id);
	#endif

		scheduler->signal_resource(obj->coyote_resource_id);

		return 0;
	}

	int FFI_pthread_mutex_destroy(void *ptr){

		scheduler->schedule_next();
		assert(hash_map != NULL && "FFI_pthread_mutex_destroy: Initialize the hash map first\n");

		llu key = (llu)ptr;
		std::unordered_map<llu, CoyoteLock*>::iterator it = hash_map->find(key);

		assert(it != hash_map->end() && "FFI_pthread_mutex_destroy: key not in map\n");

		CoyoteLock* obj = it->second;
		assert(obj->is_locked == false && "FFI_pthread_mutex_destroy: Don't destroy a locked mutex!");

	#ifdef DEBUG_PTHREAD_API
		printf("In FFI_pthread_mutex_destroy: Destroying: %p and Coyote resource id: %d \n", ptr, obj->coyote_resource_id);
	#endif

		hash_map->erase(it); // Remove the object from hash_map
		delete obj; // Remove the object from heap

		return 0;
	}

	int FFI_pthread_cond_init(void* ptr, void* attr){

		scheduler->schedule_next();
		llu key = (llu)ptr;

		if(hash_map == NULL){
			hash_map = new std::unordered_map<llu, CoyoteLock*>();
		}

		assert(hash_map->find(key) == hash_map->end() && "FFI_pthread_cond_init: Key is already in the map\n");

		CoyoteLock* new_obj = new CoyoteLock(-1, INT_MAX, true /*it is a condition variable*/);
		bool rv = (hash_map->insert({key, new_obj})).second;
		assert(rv == true && "FFI_pthread_cond_init: Inserting in the map failed!\n");

	#ifdef DEBUG_PTHREAD_API
		printf("In FFI_pthread_cond_init: Initializing: %p as Coyote resource id: %d \n", ptr, new_obj->coyote_resource_id);
	#endif

		return 0;
	}


	int FFI_pthread_cond_wait(void* cond_var_ptr, void* mtx){

	#ifdef DEBUG_PTHREAD_API
		printf("In FFI_pthread_cond_wait: with cond_var: %p and mutex is: %p \n", cond_var_ptr, mtx);
	#endif

		assert(hash_map != NULL && "FFI_pthread_cond_wait: Initialize the hash map first\n");

		llu cond_var_key = (llu)cond_var_ptr;
		llu mutex_key = (llu)mtx;

		// First check whether the conditional variable and mutex are in the map or not
		std::unordered_map<llu, CoyoteLock*>::iterator it_cond = hash_map->find(cond_var_key);
		std::unordered_map<llu, CoyoteLock*>::iterator it_mtx = hash_map->find(mutex_key);

		assert(it_cond != hash_map->end() && "FFI_pthread_cond_wait: conditional variable not in map\n");
		assert(it_mtx != hash_map->end() && "FFI_pthread_cond_wait: mutex not in map\n");

		CoyoteLock* cond_var = (*it_cond).second;
		assert(cond_var->is_cond_var && "It is not a conditional variable!");
		assert(cond_var->waitingOps != NULL && "FFI_pthread_cond_wait: Vector of WaitingOps is NULL");

		// If they are in the map:
		// Register this operation in the list of all operations waiting on this
		// conditional variable.
		size_t current_op_id = scheduler->scheduled_operation_id();

		cond_var->waitingOps->push_back(current_op_id);
		cond_var->is_locked = true;

		// Now, unlock the mutex. Unlocking the mutex can cause a context switch becoz of scheduler->signal_resource()
		// Even in that case, we should be safe.
		FFI_pthread_mutex_unlock(mtx);

	#ifdef DEBUG_PTHREAD_API
		printf("In FFI_pthread_cond_wait: Going to sleep on cond_var: %p and Coyote res_id: %d \n", cond_var_ptr, cond_var->coyote_resource_id);
	#endif

		// Wait for cond_signal or cond_broadcast
		while(cond_var->is_locked && (find(cond_var->waitingOps->begin(), cond_var->waitingOps->end(), current_op_id)
										  != cond_var->waitingOps->end())){
			scheduler->wait_resource(cond_var->coyote_resource_id);
		}

		// Lock that conditional variable again, so that other operations can wait on this conditional variable
		cond_var->is_locked = true;

		// Lock the mutex again before exiting this function
		FFI_pthread_mutex_lock(mtx);

		return 0;
	}

	int FFI_pthread_cond_signal(void* ptr){

		scheduler->schedule_next();
		assert(hash_map != NULL && "FFI_pthread_cond_signal: Initialize the hash map first\n");

		llu cond_key = (llu)ptr;

		// First check whether the conditional variable are in the map or not
		std::unordered_map<llu, CoyoteLock*>::iterator it_cond = hash_map->find(cond_key);
		assert(it_cond != hash_map->end() && "FFI_pthread_cond_signal: conditional variable not in map\n");

		CoyoteLock* cond_obj = (*it_cond).second;
		assert(cond_obj->is_cond_var && "FFI_pthread_cond_signal: this is not a conditional variable");

		// Check whether there is someone waiting on this cond_var or not
		if(!cond_obj->waitingOps->empty()){

			// Get the last element and signal it
			size_t op_id = cond_obj->waitingOps->back();
			cond_obj->waitingOps->pop_back(); // remove the last element from the list

			cond_obj->is_locked = false; // Unlock it and signal the operation

			// It is the responsibility of this operation to lock the conditional variable again!
			scheduler->signal_resource(cond_obj->coyote_resource_id, op_id);
		} else{

			// If there is no one waiting, then just unlock it
			cond_obj->is_locked = false;
		}

	#ifdef DEBUG_PTHREAD_API
		printf("In FFI_pthread_cond_signal: Signalling on cond_var: %p and Coyote res_id: %d \n", ptr, cond_obj->coyote_resource_id);
	#endif

		return 0;
	}

	int FFI_pthread_cond_broadcast(void* ptr){

		scheduler->schedule_next();
		assert(hash_map != NULL && "FFI_pthread_cond_broadcast: Initialize the hash map first\n");

		llu cond_key = (llu)ptr;

		// First check whether the conditional variable are in the map or not
		std::unordered_map<llu, CoyoteLock*>::iterator it_cond = hash_map->find(cond_key);
		assert(it_cond != hash_map->end() && "FFI_pthread_cond_broadcast: conditional variable not in map\n");

		CoyoteLock* cond_obj = (*it_cond).second;
		assert(cond_obj->is_cond_var && "FFI_pthread_cond_broadcast: this is not a conditional variable");

		if(cond_obj->waitingOps->empty()){

			// If there is no one waiting, then just unlock it
			cond_obj->is_locked = false;
		}

		// Check whether there is someone waiting on this cond_var or not
		while(!cond_obj->waitingOps->empty()){

			// Get the last element and signal it
			size_t op_id = cond_obj->waitingOps->back();
			cond_obj->waitingOps->pop_back(); // remove the last element from the list

			cond_obj->is_locked = false; // Unlock it and signal the operation

	#ifdef DEBUG_PTHREAD_API
		printf("In FFI_pthread_cond_broadcast: Signalling on cond_var: %p and Coyote res_id: %ld \n", ptr, op_id);
	#endif

			// It is the responsibility of this operation to lock the conditional variable again!
			// Not sure if instead of this,  I should do a single FFI_signal_resource() out side this loop
			scheduler->signal_resource(cond_obj->coyote_resource_id, op_id);
		};

		return 0;
	}

	int FFI_pthread_cond_destroy(void* ptr){

		scheduler->schedule_next();
		assert(hash_map != NULL && "FFI_pthread_cond_destroy: Initialize the hash map first\n");

		llu cond_key = (llu)ptr;

		// First check whether the conditional variable is in the map or not
		std::unordered_map<llu, CoyoteLock*>::iterator it_cond = hash_map->find(cond_key);
		assert(it_cond != hash_map->end() && "FFI_pthread_cond_destroy: conditional variable not in map\n");

		CoyoteLock* cond_obj = (*it_cond).second;
		assert(cond_obj->is_cond_var && "FFI_pthread_cond_destroy: this is not a conditional variable");

		hash_map->erase(it_cond);
		delete cond_obj;

		return 0;
	}