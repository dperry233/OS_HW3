

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

