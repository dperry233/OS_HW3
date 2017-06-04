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


//******************************************************************************
//----------------------------------<STRUCT>------------------------------------
//******************************************************************************

typedef linked_list_node_t* linked_list_node;
/**
 * linked_list_node_t : defination of a single node in the linked list.
 */
struct linked_list_node_t{
	int key_;
	void* data_;
	// add lock for data
	linked_list_node_t* prev_;
	// add lock for prev pointer
	linked_list_node_t* next_;
	// add lock for next pointer
}


typedef linked_list_t* linked_list;
/**
 * linked_list_t : defination of a single linked list.
 */
struct linked_list_t{
	linked_list_node container_;
	//add lock for container
	int size_;
};


//******************************************************************************
//---------------------------------<FUNCTION>-----------------------------------
//******************************************************************************

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
	list->container_=NULL;
	list->size_=0;
	// create a overall lock here
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
	// lock here
	if(list->size_!=0){
		linked_list_node node=list->container_;
		list->container_=NULL;
		for(;list->size_>0;list->size_--){
			linked_list_node fold=node;
			// acquire a key for the data
			free(node->data_);
			node=node->next_;
			free(fold);
		}

	}
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
int list_split(linked_list_t* list, int n, linked_list_t** arr);

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
int list_insert(linked_list_t* list, int key, void* data);


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
int list_remove(linked_list_t* list, int key);

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
int list_find(linked_list_t* list, int key);

/**
 * list_size : Returns the number of nodes in the given list.
 *
 * input		: list 	- the given list.
 *
 * output		: N/A
 *
 * return value	: The number of nodes in the given list.
 */
int list_size(linked_list_t* list);

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
int list_update(linked_list_t* list, int key, void* data);

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
int list_compute(linked_list_t* list, int key, int (*compute_func) (void *), int* result);

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
void list_batch(linked_list_t* list, int num_ops, op_t* ops);

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
