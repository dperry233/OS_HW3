/******************************************************************************/
/*                                                                            */
/* File Name : my_list.c                                                      */
/*                                                                            */
/******************************************************************************/

//******************************************************************************
//----------------------------------<INCLUDE>-----------------------------------
//******************************************************************************
#include <stdlib.h>
#include "list.h"
#include <limits.h>

//******************************************************************************
//----------------------------------<DEFINE>------------------------------------
//******************************************************************************

#define SUCCES			0
#define PARAM_ERROR 	1
#define ALLOC_ERROR 	2
#define INSERT_ERROR	3
#define REMOVE_ERROR	4
#define NOT_EX_ERROR	5
#define FAILURE_ERROR	-1

#define VALUE_FOUND 	1
#define VALUE_NOT_FOUND	0

//******************************************************************************
//----------------------------------<MACROS>------------------------------------
//******************************************************************************

#define link_node(prev,node,curr)	((node)->next = (curr);\
									 (node)->prev = (prev);\
									 (curr)->prev = (node);\
									 (prev)->next = (node))

#define unlink_node(prev,curr)		((prev)->next = (curr)->next;\
									 (curr)->next->prev = (prev))

#define get_first_node(list) 	((list)->first_anchor_->next)
#define get_last_node(list)		((list)->last_anchor_->prev)
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

#define init_node_locks(node)	 (init_data_lock(node);\
								  init_node_lock(node))
#define destroy_node_locks(node) (destroy_data_lock(node);\
								  destroy_node_lock(node))

#define size_inc(list)			((list)->size++)
#define size_dec(list)			((list)->size--)

//******************************************************************************
//----------------------------------<STRUCT>------------------------------------
//******************************************************************************

typedef linked_list_node_t* linked_list_node;

typedef linked_list_t* linked_list;

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
	int size_;
};

//******************************************************************************
//---------------------------------<FUNCTION>-----------------------------------
//******************************************************************************

static inline void init_first_anchor(linked_list_t* list){
	list->first_anchor_->prev 	= NULL;
	list->first_anchor_->next 	= get_last_anchor;
	list->first_anchor_->list	= list;
	list->first_anchor_->key	= INT_MIN;
	init_node_locks(list->first_anchor_);
}

static inline void init_last_anchor(linked_list_t* list){
	list->last_anchor_->prev 	= get_first_anchor;
	list->last_anchor_->next 	= NULL;
	list->last_anchor_->list	= list;
	list->last_anchor_->key		= INT_MAX;
	init_node_locks(list->last_anchor_);
}

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
	linked_list list = (linked_list) malloc(sizeof(list));
	if(!list){
		return NULL;
	}
	list->first_anchor_	= (linked_list_node) malloc(sizeof(*list->first_anchor_));
	list->last_anchor_	= (linked_list_node) malloc(sizeof(*list->last_anchor_));
	if(!list->last_anchor_ || !list->first_anchor_){
		free(list);
		return NULL;
	}
	init_first_anchor(list);
	init_last_anchor(list);
	init_cont_lock(list);
	list->size_=0;
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
	if (!list){
		return;
	}
	lock_container(list);	//so no new actions can be done on the list from this point.
	if(list->size_!=0){
		linked_list_node curr = get_first_node(list);
		linked_list_node prev = get_first_anchor(list);
		while(curr != get_last_anchor){
			lock_node(curr); 	// so if the node currently in use the process will wait.
			prev = curr;
			curr = curr->next_;
			destroy_node_locks(prev);
			free(prev);
		}
	}
	destroy_node_locks(get_first_anchor(list));
	free(get_first_anchor(list));
	list->first_anchor_= NULL;
	destroy_node_locks(get_last_anchor(list));
	free(get_last_anchor(list));
	list->last_anchor_= NULL;
	unlock_container(list); 		//so all waiting process can at least wake up.
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
	if (!list || !arr){
		return PARAM_ERROR;
	}
	if (n <=0){
		return PARAM_ERROR;
	}
	int i;
	for(i=0;i<n;i++){
		arr[i] = list_alloc();
		if (!(arr[i]){
			return ALLOC_ERROR;
		}
	}
	i=0;
	linked_list_node prev = get_first_anchor(list);
	lock_node(prev);
	linked_list_node curr = get_first_node(list);
	lock_node(curr);
	while(curr != get_last_anchor(list)){
		unlink_node(prev,curr);
		link_node((get_last_anchor(arr[i])->prev),(curr),(get_last_anchor(arr[i])));
		curr->list_=arr[i++];
		i%=n;
		curr = curr->next;
		lock_node(curr);
		unlock_node(prev);
		prev = curr->prev;
	}
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
	if (!list || !data)
		return PARAM_ERROR;
	linked_list_node new_node = (linked_list_node) malloc(sizeof(*new_node));
	if(!new_node)
		return ALLOC_ERROR;
	new_node->data_ = data;
	new_node->key_ 	= key;
	new_node->list_ = list;
	init_node_locks(new_node);
	//lock_container(list);		// need to check if list_free was not called
	linked_list_node prev = get_first_anchor(list);
	lock_node(prev);
	linked_list_node curr = get_first_node(list);
	lock_node(curr);
	while(curr){
		if(key == curr->key_){
			unlock_node(curr);
			unlock_node(prev);
			free (new_node);
			return INSERT_ERROR;	// key already in use
		}
		if(key < curr->key_){
			link_node(prev,new_node,curr);
			unlock_node(curr);
			unlock_node(prev);
			lock_container(list);
			size_inc(list);
			unlock_container(list);
			return SUCCES;
		}
		curr = curr->next;
		lock_node(curr);
		unlock_node(prev);
		prev = curr->prev;
	}
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
	if (!list){
		return PARAM_ERROR;
	}
	//lock_container(list);		// need to check if list_free was not called
	linked_list_node prev = get_first_anchor(list);
	lock_node(prev);
	linked_list_node curr = get_first_node(list);
	lock_node(curr);
	while(curr != get_last_anchor(list)){
		if(key == curr->key_){
			unlink_node(prev,curr);
			unlock_node(curr);
			unlock_node(prev);
			destroy_node_locks(curr);
			free (curr);
			lock_container(list);
			size_dec(list);
			unlock_container(list);
			return SUCCES;
		}
		curr = curr->next;
		lock_node(curr);
		unlock_node(prev);
		prev = curr->prev;
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
	if (!list){
		return PARAM_ERROR;
	}
	//lock_container(list);		// need to check if list_free was not called
	linked_list_node prev = get_first_anchor(list);
	lock_node(prev);
	linked_list_node curr = get_first_node(list);
	lock_node(curr);
	while(curr != get_last_anchor(list)){
		if(key == curr->key_){
			unlock_node(curr);
			unlock_node(prev);
			return VALUE_FOUND;
		}
		curr = curr->next;
		lock_node(curr);
		unlock_node(prev);
		prev = curr->prev;
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
	if (!list){
		return PARAM_ERROR;
	}
	//lock_container(list);		// need to check if list_free was not called
	lock_container(list);
	size = list->size_;
	unlock_container(list);
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
	if (!list || !data){
		return PARAM_ERROR;
	}
	//lock_container(list);		// need to check if list_free was not called
	linked_list_node prev = get_first_anchor(list);
	lock_node(prev);
	linked_list_node curr = get_first_node(list);
	lock_node(curr);
	while(curr != get_last_anchor(list)){
		if(key == curr->key_){
			curr->data = data;
			unlock_node(curr);
			unlock_node(prev);
			return SUCCES;
		}
		curr = curr->next;
		lock_node(curr);
		unlock_node(prev);
		prev = curr->prev;
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
	if (!list || !compute_func || !result){
		return PARAM_ERROR;
	}
	//lock_container(list);		// need to check if list_free was not called
	linked_list_node prev = get_first_anchor(list);
	lock_node(prev);
	linked_list_node curr = get_first_node(list);
	lock_node(curr);
	while(curr != get_last_anchor(list)){
		if(key == curr->key_){
			void* data = curr->data;
			lock_data(curr);		// gonna be a problem here!!!
			unlock_node(curr);
			unlock_node(prev);
			*result = compute_func(data);
			unlock_data(curr);
			return SUCCES;
		}
		curr = curr->next;
		lock_node(curr);
		unlock_node(prev);
		prev = curr->prev;
	}
	unlock_node(curr);
	unlock_node(prev);
	return NOT_EX_ERROR;
}


typedef struct op_wrapper_t
{
	linked_list_t* list;
	op_t* op;
} op_wrapper_t;


/**
 * wrapper function to be used for pthread_create in list_batch
 */
void* batch_wrapper(void* param){
	linked_list_t* list= (linked_list_t*)(param->op);
	op_t* curr_op= (op_t*)(param->op);
	int current_key = ((curr_op)->key

		if(curr_op->op==INSERT){
			curr_op->result =  list_insert(list, current_key, (curr_op->data));
		}

		if((curr_op)->op==REMOVE){
			curr_op->result =  list_remove(list, current_key);
		}

		if(curr_op->op==CONTAINS){
			curr_op->result = list_find(list, current_key);
		}

		if(curr_op->op==UPDATE){
			curr_op->result = list_update(list, current_key, curr_op->data);
			
		}
		int compute_result;
		if(curr_op->op==COMPUTE){
			curr_op->result = list_compute(list,current_key,curr_op->compute_func, &compute_result);
			curr_op->data = (void*)(long long)compute_result;
		}

		free(param);
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
	if (!list||num_ops<0||!ops){
		return PARAM_ERROR;
	}
	int i;
	pthread_t threads[num_ops];
	for(i=0;i<num_ops;i++){
		

		op_wrapper_t* wrapper=(int *)malloc(sizeof(*op_wrapper_t));
		wrapper->list= list;
		wrapper->op=ops+i;
		pthread_create(&threads[i], NULL,batch_wrapper, (void*)wrapper);
	}
	for(i=0;i<num_ops;i++){
		pthread_join(threads[i], NULL);
	}


	return 0; 

}




//******************************************************************************
//---------------------------------<PREVIOUS>-----------------------------------
//******************************************************************************
//below is a list implementation not to be actually used..just for study purposes

/* Naive linked list implementation */

list *
list_create()
{
  list *l = (list *) malloc(sizeof(list));
  l->count = 0;
  l->head = NULL;
  l->tail = NULL;
  pthread_mutex_init(&(l->mutex), NULL);
  return l;
}

void
list_free(l)
  list *l;
{
  list_item *li, *tmp;

  pthread_mutex_lock(&(l->mutex));

  if (l != NULL) {
    li = l->head;
    while (li != NULL) {
      tmp = li->next;
      free(li);
      li = tmp;
    }
  }

  pthread_mutex_unlock(&(l->mutex));
  pthread_mutex_destroy(&(l->mutex));
  free(l);
}

list_item *
list_add_element(l, ptr)
  list *l;
  void *ptr;
{
  list_item *li;

  pthread_mutex_lock(&(l->mutex));

  li = (list_item *) malloc(sizeof(list_item));
  li->value = ptr;
  li->next = NULL;
  li->prev = l->tail;

  if (l->tail == NULL) {
    l->head = l->tail = li;
  }
  else {
    l->tail = li;
  }
  l->count++;

  pthread_mutex_unlock(&(l->mutex));

  return li;
}

int
list_remove_element(l, ptr)
  list *l;
  void *ptr;
{
  int result = 0;
  list_item *li = l->head;

  pthread_mutex_lock(&(l->mutex));

  while (li != NULL) {
    if (li->value == ptr) {
      if (li->prev == NULL) {
        l->head = li->next;
      }
      else {
        li->prev->next = li->next;
      }

      if (li->next == NULL) {
        l->tail = li->prev;
      }
      else {
        li->next->prev = li->prev;
      }
      l->count--;
      free(li);
      result = 1;
      break;
    }
    li = li->next;
  }

  pthread_mutex_unlock(&(l->mutex));

  return result;
}

void
list_each_element(l, func)
  list *l;
  int (*func)(list_item *);
{
  list_item *li;

  pthread_mutex_lock(&(l->mutex));

  li = l->head;
  while (li != NULL) {
    if (func(li) == 1) {
      break;
    }
    li = li->next;
  }

  pthread_mutex_unlock(&(l->mutex));
}

//below is a different list implementation
node1_t *delete(int value)
{
    node1_t *prev, *current;

    prev = &ListHead;
    pthread_mutex_lock(&prev->lock);
    while ((current = prev->link) != NULL) {
        pthread_mutex_lock(&current->lock);
        if (current->value == value) {
            prev->link = current->link;
            pthread_mutex_unlock(&current->lock);
            pthread_mutex_unlock(&prev->lock);
            current->link = NULL;
            return(current);
        }
        pthread_mutex_unlock(&prev->lock);
        prev = current;
    }
    pthread_mutex_unlock(&prev->lock);
    return(NULL);
}
