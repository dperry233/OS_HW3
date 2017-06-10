/******************************************************************************/
/*                                                                            */
/* File Name : my_list.c                                                      */
/*                                                                            */
/******************************************************************************/

//*****************************************************************************/
//----------------------------------<INCLUDE>---------------------------------*/
//*****************************************************************************/

#include <stdlib.h>
#include <limits.h>
#include <pthread.h>
#include "my_list.h"
#include <stdio.h>


//*****************************************************************************/
//----------------------------------<DEFINE>----------------------------------*/
//*****************************************************************************/

#define SUCCES			0
#define PARAM_ERROR 	-1
#define ALLOC_ERROR 	-2
#define INSERT_ERROR	-3
#define REMOVE_ERROR	-4
#define NOT_EX_ERROR	-5
#define LIST_FREE_ERROR -6
#define FAILURE_ERROR   -10

#define VALUE_FOUND 	1
#define VALUE_NOT_FOUND	0

//*****************************************************************************/
//----------------------------------<MACROS>----------------------------------*/
//*****************************************************************************/

#define link_node(prev,node,curr)	(node)->next_ = (curr);\
									(node)->prev_ = (prev);\
									(prev)->next_ = (node);\
									(curr)->prev_ = (node)


#define unlink_node(node)			(node)->prev_->next_ = (node)->next_;\
									(node)->next_->prev_ = (node)->prev_

#define get_first_node(list) 	((list)->first_anchor_->next_)
#define get_last_node(list)		((list)->last_anchor_->prev_)
#define get_first_anchor(list) 	((list)->first_anchor_)
#define get_last_anchor(list)	((list)->last_anchor_)

#define init_data_lock(node)	(pthread_mutex_init(&((node)->data_lock_), NULL))
#define lock_data(node)			(pthread_mutex_lock(&((node)->data_lock_)))
#define unlock_data(node)		(pthread_mutex_unlock(&((node)->data_lock_)))
#define destroy_data_lock(node)	(pthread_mutex_destroy(&((node)->data_lock_)))

#define init_node_lock(node)	(pthread_mutex_init(&((node)->node_lock_), NULL))
#define lock_node(node)			(pthread_mutex_lock(&((node)->node_lock_)))
#define unlock_node(node)		(pthread_mutex_unlock(&((node)->node_lock_)))
#define destroy_node_lock(node)	(pthread_mutex_destroy(&((node)->node_lock_)))

#define init_cont_lock(list)	(pthread_mutex_init(&((list)->main_lock_), NULL))
#define lock_container(list)	(pthread_mutex_lock(&((list)->main_lock_)))
#define unlock_container(list)	(pthread_mutex_unlock(&((list)->main_lock_)))
#define destroy_cont_lock(list) (pthread_mutex_destroy(&((list)->main_lock_)))

#define init_node_locks(node)	 init_data_lock(node);\
								 init_node_lock(node)

#define destroy_node_locks(node) destroy_data_lock(node);\
								 destroy_node_lock(node)

#define size_inc(list)			(((list)->size_)++)
#define size_dec(list)			(((list)->size_)--)

#define unlock_and_destroy(node)	unlock_node(node);\
									destroy_node(node)

//*****************************************************************************/
//----------------------------------<STRUCT>----------------------------------*/
//*****************************************************************************/

typedef struct linked_list_node_t* linked_list_node;

typedef struct linked_list_t* linked_list;

/**
 * linked_list_node_t : defination of a single node in the linked list.
 */
struct linked_list_node_t{
	int 				key_;
	void* 				data_;
	pthread_mutex_t 	data_lock_;
	linked_list_node 	prev_;
	linked_list_node 	next_;
	linked_list			list_;
	pthread_mutex_t 	node_lock_;
};

/**
 * linked_list_t : defination of a single linked list.
 */
struct linked_list_t{
	linked_list_node first_anchor_;
	linked_list_node last_anchor_;
	pthread_mutex_t  main_lock_;
};

//*****************************************************************************/
//-----------------------------<STATIC FUNCTIONS>-----------------------------*/
//*****************************************************************************/

/**
 * init_first_anchor : initializes the anchor at the beginning of the link list.
 */
static inline void init_first_anchor(linked_list list){
	list->first_anchor_->prev_ 	= NULL;
	list->first_anchor_->next_ 	= get_last_anchor(list);
	list->first_anchor_->list_	= list;
	list->first_anchor_->key_	= INT_MIN;
	init_node_locks(list->first_anchor_);
}

/**
 * init_last_anchor : initializes the anchor at the end of the link list.
 */
static inline void init_last_anchor(linked_list list){
	list->last_anchor_->prev_ 	= get_first_anchor(list);
	list->last_anchor_->next_ 	= NULL;
	list->last_anchor_->list_	= list;
	list->last_anchor_->key_	= INT_MAX;
	init_node_locks(list->last_anchor_);
}

/**
 * destroy_node : destroys the given node.
 */
static inline void destroy_node(linked_list_node node){
	destroy_node_locks(node);
	free(node);
}

//*****************************************************************************/
//--------------------------------<FUNCTIONS>---------------------------------*/
//*****************************************************************************/

/**
 * list_alloc : Creates a new linked list.
 *
 * input		: N/A
 *
 * output		: N/A
 *
 * return value	: A new linked list.
 */
linked_list_t* list_alloc(){
	linked_list list = (linked_list) malloc(sizeof(*list));
	if(!list)
		return NULL;
	list->first_anchor_	= (linked_list_node) malloc(sizeof(*(list->first_anchor_)));
	list->last_anchor_	= (linked_list_node) malloc(sizeof(*(list->last_anchor_)));
	if(!list->last_anchor_ || !list->first_anchor_){
		free(list);
		return NULL;
	}
	init_first_anchor(list);
	init_last_anchor(list);
	init_cont_lock(list);
	return list;
}

/**
 * list_free : Free the given list.
 *
 * input		: list - the given list to free.
 *
 * output		: N/A
 *
 * return value	: N/A
 */
void list_free(linked_list_t* list){
	if (!list) return;
	linked_list_node anchor, prev, curr;
	lock_container(list);
	anchor = get_first_anchor(list);
	if(!anchor){	// if the lock was acquired after the list was freed
		unlock_container(list);
		return;
	}
	get_first_anchor(list) = NULL;	// unlink anchor from list
	lock_node(anchor);
	unlock_container(list);
	prev = anchor;
	curr = anchor->next_;
	lock_node(curr);	// so if the node currently in use the process will wait.
	while(curr != get_last_anchor(list)){
		prev = curr;
		curr = curr->next_;
		lock_node(curr);
		lock_data(prev);
		unlock_data(prev);
		unlock_and_destroy(prev);
	}
	unlock_and_destroy(anchor);
	unlock_and_destroy(curr);
	destroy_cont_lock(list);
	free(list);
}

/**
 * list_split : Splits the given array into n new lists alternately.
 * the new lists will be stored in the given array and the original array will
 * be free. If there are fewer than n elements in the original list, an empty
 * lists will be generated up to a total of n lists.
 *
 * input		: list 	- the given list.
 * 				: n 	- the number of lists to split to.
 *
 * output		: arr 	- an array to add the new lists to.
 *
 * return value	: 0 in case of success or anything else in case of failure.
 */
int list_split(linked_list_t* list, int n, linked_list_t** arr){
	if (!list || !arr || n <=0)	return PARAM_ERROR;
	int i;
	linked_list_node anchor, curr;
	lock_container(list);
	anchor = get_first_anchor(list);
	if(!anchor){	// if the lock was acquired after the list was freed
		unlock_container(list);
		return LIST_FREE_ERROR;
	}
	get_first_anchor(list) = NULL;	// unlink anchor from list
	lock_node(anchor);
	unlock_container(list);
	for(i=0;i<n;i++){
		arr[i] = list_alloc();
		if (!(arr[i])){
			for(;i>=0;i--)
				list_free(arr[i]);
			return ALLOC_ERROR;
		}
	}
	i=0;
	curr = anchor->next_;
	lock_node(curr);	// so if the node currently in use the process will wait.
	while(curr != get_last_anchor(list)){
		lock_node(curr->next_);
		unlink_node(curr);
		link_node(get_last_node(arr[i]),curr,get_last_anchor(arr[i]));
		curr->list_=arr[i];
		i++;
		if(i == n)
			i=0;
		unlock_node(curr);
		curr = anchor->next_;
	}
	unlock_and_destroy(anchor);
	unlock_and_destroy(curr);
	destroy_cont_lock(list);
	free(list);
	return SUCCES;
}

/**
 * list_insert : Inserts new node with the given data and key into the given
 * list. If a node with the given key already exist the function will fail.
 *
 * input		: list 	- the given list.
 * 				: key 	- the given key for the new node.
 * 				: data 	- the given data for the new node.
 *
 * output		: N/A
 *
 * return value	: 0 in case of success or anything else in case of failure.
 */
int list_insert(linked_list_t* list, int key, void* data){
	if (!list)	return PARAM_ERROR;
	linked_list_node prev, curr, new_node;
	new_node = (linked_list_node) malloc(sizeof(*new_node));
	if(!new_node)
		return ALLOC_ERROR;
	new_node->data_ = data;
	new_node->key_ 	= key;
	new_node->list_ = list;
	init_node_locks(new_node);
	lock_container(list);
	prev = get_first_anchor(list);
	if(!prev){	// if the lock was acquired after the list was freed
		unlock_container(list);
		destroy_node(new_node);
		return LIST_FREE_ERROR;
	}
	lock_node(prev);
	unlock_container(list);
	curr = get_first_node(list);
	lock_node(curr);
	while(curr){
		if(key == curr->key_){
			unlock_node(curr);
			unlock_node(prev);
			destroy_node_locks(new_node);
			free (new_node);
			return INSERT_ERROR;	// key already in use
		}
		if(key < curr->key_){
			link_node(prev,new_node,curr);
			unlock_node(prev);
			unlock_node(curr);
			return SUCCES;
		}
		curr = curr->next_;
		lock_node(curr);
		unlock_node(prev);
		prev = curr->prev_;
	}
	// never gets here
	return FAILURE_ERROR;
}

/**
 * list_remove : Removes a new node with the given key from the given list.
 * If a node with the given key does not exist the function will fail.
 *
 * input		: list 	- the given list.
 * 				: key 	- the given key.
 *
 * output		: N/A
 *
 * return value	: 0 in case of success or anything else in case of failure.
 */
int list_remove(linked_list_t* list, int key){
	if (!list)	return PARAM_ERROR;
	linked_list_node prev, curr;
	lock_container(list);
	prev = get_first_anchor(list);
	if(!prev){	// if the lock was acquired after the list was freed
		unlock_container(list);
		return LIST_FREE_ERROR;
	}
	lock_node(prev);
	unlock_container(list);
	curr = get_first_node(list);
	lock_node(curr);
	while(curr != get_last_anchor(list)){
		if(key == curr->key_){
			unlink_node(curr);
			unlock_node(prev);
			lock_data(curr);
			unlock_data(curr);
			unlock_and_destroy(curr);
			return SUCCES;
		}
		curr = curr->next_;
		lock_node(curr);
		unlock_node(prev);
		prev = curr->prev_;
	}
	unlock_node(curr);
	unlock_node(prev);
	return REMOVE_ERROR;
}

/**
 * list_find : Checks if a node with the given key exist in the given list.
 *
 * input		: list 	- the given list.
 * 				: key 	- the given key.
 *
 * output		: N/A
 *
 * return value	: 1 in case a node with the given key found or 0 otherwise.
 */
int list_find(linked_list_t* list, int key){
	if (!list)
		return PARAM_ERROR;
	linked_list_node prev, curr;
	lock_container(list);
	prev = get_first_anchor(list);
	if(!prev){	// if the lock was acquired after the list was freed
		unlock_container(list);
		return LIST_FREE_ERROR;
	}
	lock_node(prev);
	unlock_container(list);
	curr = get_first_node(list);
	lock_node(curr);
	while(curr != get_last_anchor(list)){
		if(key == curr->key_){
			unlock_node(curr);
			unlock_node(prev);
			return VALUE_FOUND;
		}
		curr = curr->next_;
		lock_node(curr);
		unlock_node(prev);
		prev = curr->prev_;
	}
	unlock_node(curr);
	unlock_node(prev);
	return VALUE_NOT_FOUND;
}

/**
 * list_size : Returns the number of nodes in the given list.
 *
 * input		: list 	- the given list.
 *
 * output		: N/A
 *
 * return value	: The number of nodes in the given list.
 */
int list_size(linked_list_t* list){
	if (!list)	return PARAM_ERROR;
	linked_list_node prev, curr;
	int size=0;
	lock_container(list);
	prev = get_first_anchor(list);
	if(!prev){	// if the lock was acquired after the list was freed
		unlock_container(list);
		return LIST_FREE_ERROR;
	}
	lock_node(prev);
	unlock_container(list);
	curr = get_first_node(list);
	lock_node(curr);
	while(curr != get_last_anchor(list)){
		size++;
		curr = curr->next_;
		lock_node(curr);
		unlock_node(prev);
		prev = curr->prev_;
	}
	unlock_node(curr);
	unlock_node(prev);
	return size;
}

/**
 * list_update : Sets the given data as the new data of the node with the given
 * key.
 *
 * input		: list 	- the given list.
 * 				: key 	- the given key.
 * 				: data 	- the given data to set.
 *
 * output		: N/A
 *
 * return value	: 0 in case of success or anything else in case of failure.
 */
int list_update(linked_list_t* list, int key, void* data){
	if (!list || !data)	return PARAM_ERROR;
	linked_list_node prev, curr;
	lock_container(list);
	prev = get_first_anchor(list);
	if(!prev){	// if the lock was acquired after the list was freed
		unlock_container(list);
		return LIST_FREE_ERROR;
	}
	lock_node(prev);
	unlock_container(list);
	curr = get_first_node(list);
	lock_node(curr);
	while(curr != get_last_anchor(list)){
		if(key == curr->key_){
			curr->data_ = data;
			unlock_node(curr);
			unlock_node(prev);
			return SUCCES;
		}
		curr = curr->next_;
		lock_node(curr);
		unlock_node(prev);
		prev = curr->prev_;
	}
	unlock_node(curr);
	unlock_node(prev);
	return NOT_EX_ERROR;
}

/**
 * list_compute : Computes the data of the node with the given key, using the
 * user provided function.
 *
 * input		: list 			- the given list.
 * 				: key 			- the given key.
 * 				: compute_func 	- the computetion function.
 * 				:
 *
 * output		: result		- the result of the computetion on the data.
 *
 * return value	: 0 in case of success or anything else in case of failure.
 */
int list_compute(linked_list_t* list, int key, int (*compute_func) (void *), int* result){
	if (!list || !compute_func || !result)	return PARAM_ERROR;
	linked_list_node prev, curr;
	lock_container(list);
	prev = get_first_anchor(list);
	if(!prev){	// if the lock was acquired after the list was freed
		unlock_container(list);
		return LIST_FREE_ERROR;
	}
	lock_node(prev);
	unlock_container(list);
	curr = get_first_node(list);
	lock_node(curr);
	while(curr != get_last_anchor(list)){
		if(key == curr->key_){
			lock_data(curr);
			void* data = curr->data_;
			unlock_data(curr);
			unlock_node(curr);
			unlock_node(prev);
			lock_data(curr);
			*result = compute_func(data);
			unlock_data(curr);
			return SUCCES;
		}
		curr = curr->next_;
		lock_node(curr);
		unlock_node(prev);
		prev = curr->prev_;
	}
	unlock_node(curr);
	unlock_node(prev);
	return NOT_EX_ERROR;
}

typedef struct op_wrapper_t
{
	linked_list_t* list;
	op_t* op;
} op_wrapper;

/**
 * wrapper function to be used for pthread_create in list_batch
 */
void* batch_wrapper(void* param){
	linked_list_t* list=(((op_wrapper*)param)->list);
	op_t* curr_op=(((op_wrapper*)param)->op);
	int current_key = curr_op->key;
	int res;
		switch(curr_op->op){
		case INSERT:
			curr_op->result =  list_insert(list, current_key, (curr_op->data));
			break;
		case REMOVE:
			curr_op->result =  list_remove(list, current_key);
			break;
		case CONTAINS:
			curr_op->result = list_find(list, current_key);
			break;
		case UPDATE:
			curr_op->result = list_update(list, current_key, curr_op->data);
			break;
		case COMPUTE:
			curr_op->result = list_compute(list,current_key,curr_op->compute_func, &res);
			curr_op->data = (void*)(long long)res;
			break;
		}
		return NULL;
}

/**
 * list_batch : Performs a several different operations on the list.
 *
 * input		: list 		- the given list.
 * 				: num_ops 	- the number of operations in the batch.
 * 				: ops 		- the array of op_t structure, witch defined as
 * 							  follows.
 *
 * 			typedef struct op_t {
 *				int key;
 *				void* data;
 *				enum {INSERT, REMOVE, CONTAINS, UPDATE, COMPUTE} op;
 * 				int (*compute_func) (void *);
 * 				int result;
 * 			} op_t;
 *
 * output		: N/A.
 *
 * return value	: 0 in case of success or anything else in case of failure.
 */
void list_batch(linked_list_t* list, int num_ops, op_t* ops){
	if (!list || num_ops<=0 || !ops)
		return;
	int i;
	pthread_t threads[num_ops];
	op_wrapper* wrappers[num_ops];
	for(i=0;i<num_ops;i++){
		wrappers[i]=(op_wrapper*)malloc(sizeof(op_wrapper));
		if (!wrappers[i])
			return;
		wrappers[i]->list= list;
		wrappers[i]->op=&(ops[i]);
		pthread_create(&(threads[i]), NULL, batch_wrapper, (void*)(wrappers[i]));
	}
	for(i=0;i<num_ops;i++){
		pthread_join(threads[i], NULL);
		free(wrappers[i]);
	}
}

