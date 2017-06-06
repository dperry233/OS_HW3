#include "my_list.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define LIST_FOR_EACH(list) for(int i = 0; i < list_size((list)) ; ++i)



#define ASSERT_NON_ZERO(b) do { \
if ((b) == 0) { \
	fprintf(stdout, "\nAssertion failed at %s:%d %s ",__FILE__,__LINE__,#b); \
	return false; \
} \
} while (0)

#define ASSERT_ZERO(b) do { \
if ((b) != 0) { \
	fprintf(stdout, "\nAssertion failed at %s:%d %s ",__FILE__,__LINE__,#b); \
	return false; \
} \
} while (0)

#define ASSERT_TEST(b) do { \
if (!(b)) { \
	fprintf(stdout, "\nAssertion failed at %s:%d %s ",__FILE__,__LINE__,#b); \
	return false; \
} \
} while (0)

#define RUN_TEST(test) do { \
fprintf(stdout, "Running "#test"... "); \
if (test()) { \
	fprintf(stdout, "[OK]\n");\
} else { \
	fprintf(stdout, "[Failed]\n"); \
} \
} while(0)

static bool isAEIOU(char ch){
	return 	((ch =='a') || (ch == 'A')
			||(ch =='e') || (ch == 'E')
			||(ch =='i') || (ch == 'I')
			||(ch =='o') || (ch == 'O')
			||(ch =='u') || (ch == 'U'));
}
static int youComputeNothing(void* data){
	char* name = (char*)data;
	int count = 0;
	for(int i=0;i<strlen(name);++i)
		count+= isAEIOU(name[i]);
	return count;
}

static int randRange(int end){
	return rand() % end;
}

bool testFreeErrors(){
	list_free(NULL);
	linked_list_t* list = list_alloc();
	list_free(list);

	return true;
}

bool testSplitErrors(){
	linked_list_t* list = list_alloc();
	ASSERT_NON_ZERO(list_split(NULL,1984,NULL));
	ASSERT_NON_ZERO(list_split(list,1984,NULL));
	ASSERT_NON_ZERO(list_split(list,0,NULL));
	ASSERT_NON_ZERO(list_split(list,-1984,NULL));

	linked_list_t* arr[5];
	ASSERT_NON_ZERO(list_split(list,-1984,arr));

	ASSERT_NON_ZERO(list_split(NULL,1984,arr));
	ASSERT_NON_ZERO(list_split(NULL,0,arr));
	ASSERT_NON_ZERO(list_split(NULL,-1984,arr));

	list_free(list);
	return true;
}

bool testInsertErrors(){
	linked_list_t* list = list_alloc();
	int data = 42;
	ASSERT_NON_ZERO(list_insert(NULL,1984,NULL));
	ASSERT_NON_ZERO(list_insert(NULL,1984,&data));

	ASSERT_ZERO(list_insert(list,1984,&data));
	ASSERT_NON_ZERO(list_insert(list,1984,&data));
	ASSERT_NON_ZERO(list_insert(list,1984,&data));
	list_free(list);
	return true;
}

bool testRemoveErrors(){
	linked_list_t* list = list_alloc();
	int data = 42;
	ASSERT_NON_ZERO(list_remove(NULL,1984));

	ASSERT_NON_ZERO(list_remove(list,1984));

	list_free(list);
	return true;
}

bool testFindErrors(){
	linked_list_t* list = list_alloc();
	int data = 42;
	ASSERT_TEST(list_find(NULL,1984) == 0);

	ASSERT_TEST(list_find(list,1984) == 0);
	list_free(list);
	return true;
}

bool testSizeErrors(){
	//TODO: PIAZZA - check about list_size(NULL)
	linked_list_t* list = list_alloc();

	list_free(list);
	return true;
}

bool testUpdateErrors(){
	linked_list_t* list = list_alloc();
	ASSERT_NON_ZERO(list_update(NULL,1984,NULL));
	ASSERT_NON_ZERO(list_update(NULL,0,NULL));
	ASSERT_NON_ZERO(list_update(NULL,-1984,NULL));

	ASSERT_NON_ZERO(list_update(list,-1984,NULL));

	list_free(list);
	return true;
}

bool testComputeErrors(){
	linked_list_t* list = list_alloc();
	int result;
	ASSERT_NON_ZERO(list_compute(NULL,1984,NULL,NULL));
	ASSERT_NON_ZERO(list_compute(NULL,1984,youComputeNothing,NULL));
	ASSERT_NON_ZERO(list_compute(NULL,1984,youComputeNothing,&result));
	ASSERT_NON_ZERO(list_compute(NULL,1984,NULL,&result));
	ASSERT_NON_ZERO(list_compute(list,1984,NULL,NULL));
	ASSERT_NON_ZERO(list_compute(list,1984,youComputeNothing,NULL));
	ASSERT_NON_ZERO(list_compute(list,1984,NULL,&result));

	list_free(list);
	return true;
}

bool testBatchErrors(){
	linked_list_t* list = list_alloc();
	op_t ops;
	list_batch(NULL,1984,NULL);
	list_batch(NULL,1984,&ops);

	list_batch(NULL,-1984,NULL);
	list_batch(NULL,-1984,&ops);
	list_batch(list,-1984,NULL);
	list_batch(list,-1984,&ops);

	list_free(list);
	return true;
}

bool testSequential1(){
	linked_list_t* list1 = list_alloc();
	linked_list_t* list2 = list_alloc();
	linked_list_t* list3 = list_alloc();
	int numOfStarks = 6 , numOfLannisters = 4;
	int allStarks[6] = {66,22,55,11,44,33};  
	int n1 = 1 ,n2 = 3, n3 = numOfLannisters;
	linked_list_t* arr1[n1];
	linked_list_t* arr2[n2];
	linked_list_t* arr3[numOfLannisters];
	ASSERT_TEST(list_size(list1) == 0);
	ASSERT_ZERO(list_insert(list1,66,"Jon"));
	ASSERT_ZERO(list_insert(list1,44,"Sansa"));
	ASSERT_ZERO(list_insert(list1,55,"Arya"));
	ASSERT_ZERO(list_insert(list1,22,"Bran"));
	ASSERT_ZERO(list_insert(list1,11,"Rickon"));
	ASSERT_ZERO(list_insert(list1,33,"Robb"));
	ASSERT_TEST(list_size(list1) == numOfStarks);

	ASSERT_ZERO(list_insert(list2,444,"Joffrey"));
	ASSERT_ZERO(list_insert(list2,333,"Tommen"));
	ASSERT_ZERO(list_insert(list2,111,"Myrcella"));
	ASSERT_ZERO(list_insert(list2,222,"Tywin"));
	ASSERT_TEST(list_size(list2) == numOfLannisters);

	ASSERT_ZERO(list_insert(list3,444,"Joffrey"));
	ASSERT_ZERO(list_insert(list3,111,"Myrcella"));
	ASSERT_TEST(list_size(list3) == (numOfLannisters-2));

	LIST_FOR_EACH(list1){
		ASSERT_TEST(list_find(list1,allStarks[i]) == 1);
	}

	ASSERT_ZERO(list_remove(list1,33));
	ASSERT_ZERO(list_remove(list1,11));
	ASSERT_TEST(list_size(list1) == (numOfStarks-2));
	ASSERT_TEST(list_find(list1,33) == 0);
	ASSERT_TEST(list_find(list1,11) == 0);

	ASSERT_ZERO(list_insert(list1,11,"Rickon"));
	ASSERT_ZERO(list_insert(list1,33,"Robb"));
	ASSERT_TEST(list_find(list1,33) == 1);
	ASSERT_TEST(list_find(list1,11) == 1);

	ASSERT_ZERO(list_split(list1,n1,arr1)); // list1 is freed
	ASSERT_TEST(list_size(arr1[0]) == numOfStarks); // arr1[0] is identical to list1 before list_split

	ASSERT_ZERO(list_split(list2,n2,arr2)); // list2 is freed
	ASSERT_TEST(list_find(arr2[0],111) == 1);
	ASSERT_TEST(list_find(arr2[0],444) == 1);
	ASSERT_TEST(list_size(arr2[0]) == 2);
	ASSERT_TEST(list_find(arr2[1],222) == 1);
	ASSERT_TEST(list_size(arr2[1]) == 1);
	ASSERT_TEST(list_find(arr2[2],333) == 1);
	ASSERT_TEST(list_size(arr2[2]) == 1);

	ASSERT_ZERO(list_split(list3,n3,arr3)); // list3 is freed
	ASSERT_TEST(list_find(arr3[0],111) == 1);
	ASSERT_TEST(list_find(arr3[1],444) == 1);
	ASSERT_TEST(list_size(arr3[0]) == 1);
	ASSERT_TEST(list_size(arr3[1]) == 1);
	ASSERT_TEST(list_size(arr3[2]) == 0); // empty list
	ASSERT_TEST(list_size(arr3[2]) == 0); // empty list
	

	// free all allocated lists
	for(int i=0;i<n1;++i)
		list_free(arr1[i]);
	for(int i=0;i<n2;++i)
		list_free(arr2[i]);
	for(int i=0;i<n3;++i)
		list_free(arr3[i]);
	return true;
}

bool testSequential2(){
	linked_list_t* list1 = list_alloc();
	linked_list_t* list2 = list_alloc();
	linked_list_t* list3 = list_alloc();
	int result;

	ASSERT_ZERO(list_insert(list1,66,"Jon"));
	ASSERT_ZERO(list_insert(list1,44,"Sansa"));
	ASSERT_ZERO(list_insert(list1,55,"Arya"));
	ASSERT_ZERO(list_insert(list1,22,"Bran"));
	ASSERT_ZERO(list_insert(list1,11,"Rickon"));
	ASSERT_ZERO(list_insert(list1,33,"Robb"));

	ASSERT_ZERO(list_insert(list2,444,"Joffrey"));
	ASSERT_ZERO(list_insert(list2,333,"Tommen"));
	ASSERT_ZERO(list_insert(list2,111,"Myrcella"));
	ASSERT_ZERO(list_insert(list2,222,"Tywin"));

	ASSERT_ZERO(list_insert(list3,1111,"Jorah"));

	ASSERT_ZERO(list_compute(list1,11,youComputeNothing,&result));
	ASSERT_TEST(result == 2);
	ASSERT_ZERO(list_compute(list1,33,youComputeNothing,&result));
	ASSERT_TEST(result == 1);
	ASSERT_ZERO(list_compute(list1,66,youComputeNothing,&result));
	ASSERT_TEST(result == 1);
	ASSERT_ZERO(list_compute(list2,222,youComputeNothing,&result));
	ASSERT_TEST(result == 1);
	ASSERT_ZERO(list_compute(list2,444,youComputeNothing,&result));
	ASSERT_TEST(result == 2);
	ASSERT_ZERO(list_compute(list3,1111,youComputeNothing,&result));
	ASSERT_TEST(result == 2);

	ASSERT_ZERO(list_update(list1,11,"Rickon One Direction"));
	ASSERT_ZERO(list_update(list1,33,"Robb Zombie"));
	ASSERT_ZERO(list_update(list1,66,"Jon know-nothing Snow"));
	ASSERT_ZERO(list_update(list2,222,"Ty-win-ston churchill"));
	ASSERT_ZERO(list_update(list2,444,"Joffrey I'll tell mother"));
	ASSERT_ZERO(list_update(list3,1111,"Jorah in the zone"));

	ASSERT_ZERO(list_compute(list1,11,youComputeNothing,&result));
	ASSERT_TEST(result == 8);
	ASSERT_ZERO(list_compute(list1,33,youComputeNothing,&result));
	ASSERT_TEST(result == 4);
	ASSERT_ZERO(list_compute(list1,66,youComputeNothing,&result));
	ASSERT_TEST(result == 5);
	ASSERT_ZERO(list_compute(list2,222,youComputeNothing,&result));
	ASSERT_TEST(result == 4);
	ASSERT_ZERO(list_compute(list2,444,youComputeNothing,&result));
	ASSERT_TEST(result == 6);
	ASSERT_ZERO(list_compute(list3,1111,youComputeNothing,&result));
	ASSERT_TEST(result == 6);

	list_free(list1);
	list_free(list2);
	list_free(list3);
	return true;
}


int main(){
	RUN_TEST(testFreeErrors);
	RUN_TEST(testSplitErrors);
	RUN_TEST(testInsertErrors);
	RUN_TEST(testRemoveErrors);
	RUN_TEST(testFindErrors);
	RUN_TEST(testSizeErrors);
	RUN_TEST(testUpdateErrors);
	RUN_TEST(testComputeErrors);
	RUN_TEST(testBatchErrors);
	RUN_TEST(testSequential1);
	RUN_TEST(testSequential2);

	return 0;
}
