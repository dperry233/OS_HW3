#include <iostream> // Out stuff
#include <fstream> // File stuff
#include <unistd.h> // Unix stuff
#include <cstdlib> // Random stuff
#include <cstring>
#include <random>
#include <cstdarg> // va va va stuff

#include <vector> // STL stuff
#include <string> // another STL stuff
#include <map> // another another STL stuff
#include <unordered_set>

using namespace std;

static inline string format(const char* fmt, ...){
    int size = 512;
    char* buffer = 0;
    buffer = new char[size];
    va_list vl;
    va_start(vl, fmt);
    int nsize = vsnprintf(buffer, size, fmt, vl);
    if(size<=nsize){ //fail delete buffer and try again
        delete[] buffer;
        buffer = 0;
        buffer = new char[nsize+1]; //+1 for /0
        nsize = vsnprintf(buffer, size, fmt, vl);
    }
    string ret(buffer);
    va_end(vl);
    delete[] buffer;
    return ret;
}

enum class Operation{
  INSERT, REMOVE, CONTAINS, UPDATE, COMPUTE, SIZE, BATCH
};

void initialize_names(vector<string>& names){ // AEIOU
  names.push_back("Sansa");
  names.push_back("Tommen");
  names.push_back("Jorah");
  names.push_back("Tywin");
  names.push_back("Robb");
  names.push_back("Arya");
  names.push_back("Cersei");
  names.push_back("Jaime");

  names.push_back("Daenerys");
  names.push_back("Jon");
  names.push_back("Lyanna");
  names.push_back("Rhaegar");

  names.push_back("Oh Gilly");
}

string program_head = "#include \"stdio.h\"\n#include \"stdlib.h\"\n#include \"string.h\"\n#include \"stdbool.h\"\n#include \"my_list.h\"\n\n#define ASSERT_NON_ZERO(b) do { \\\r\nif ((b) == 0) { \\\r\n\tfprintf(stdout, \"\\nAssertion failed at %s:%d %s \",__FILE__,__LINE__,#b); \\\r\n\treturn false; \\\r\n} \\\r\n} while (0)\r\n\r\n#define ASSERT_ZERO(b) do { \\\r\nif ((b) != 0) { \\\r\n\tfprintf(stdout, \"\\nAssertion failed at %s:%d %s \",__FILE__,__LINE__,#b); \\\r\n\treturn false; \\\r\n} \\\r\n} while (0)\r\n\r\n#define ASSERT_TEST(b) do { \\\r\nif (!(b)) { \\\r\n\tfprintf(stdout, \"\\nAssertion failed at %s:%d %s \",__FILE__,__LINE__,#b); \\\r\n\treturn false; \\\r\n} \\\r\n} while (0)\n#define RUN_TEST(test) do { \\\r\nfprintf(stdout, \"Running \"#test\"... \"); \\\r\nif (test()) { \\\r\n\tfprintf(stdout, \"[OK]\\n\");\\\r\n} else { \\\r\n\tfprintf(stdout, \"[Failed]\\n\"); \\\r\n} \\\r\n} while(0)\r\n\nstatic int hash_test(void* data){\r\n  char* str = data;\r\n  int hash = 7;\r\n  for (int i = 0; i < strlen(str); i++) {\r\n      hash = hash*31 + str[i];\r\n  }\r\n  return hash;\r\n}\n\n";

string main_head = "int main(){\n";
string main_tail = "\n\treturn 0;\n}\n";

int g_num_of_list_operations = 1000; // Default values
int g_num_of_threads = 10;  // Default values
bool isValgrindOn = false;   // Default values

// Will be used as MAX_KEY aswell
#define MAX_OPS_PER_BATCH 100

int findLongest(vector<string> names){
  int max = 0;
  for(string str : names){
    if(str.length() > max) max = str.length();
  }

  return max;
}

string opToString(int op){
  switch(op){
    case 0:
      return "INSERT";
    case 1:
      return "REMOVE";
    case 2:
      return "CONTAINS";
    case 3:
      return "UPDATE";
    case 4:
      return "COMPUTE";
  }
  return "BLAH";
}

random_device rd;
mt19937 rng(rd());

void write_names(ofstream& batch_file, vector<string>& names){
  batch_file << format("\tchar names[%d][%d] = {", names.size(), findLongest(names) + 1);
  batch_file << "\"" << names[0] << "\"";
  for(int i = 1 ; i < names.size() ; i++) batch_file << ",\"" << names[i] << "\"";
  batch_file << "};\n";
}

void createAndRunNonDeterministicBatch(vector<string>& names){
  uniform_int_distribution<int> num_rand(5, MAX_OPS_PER_BATCH);
  uniform_int_distribution<int> name_rand(0, names.size() - 1);
  uniform_int_distribution<int> op_rand(0, 4);
  uniform_int_distribution<int> num_of_batches(50, 500); // Random number is random


  ofstream batch_file;
  batch_file.open("batch_test.c");
  batch_file << program_head; // Initialize program
  batch_file << main_head;
  batch_file << "\tlinked_list_t* list = list_alloc();\n";

  write_names(batch_file, names);
  batch_file << "\n\top_t oper; int i = 0;\n";
  for(int batch = 0 ; batch < num_of_batches(rng) ; batch++){
    int batch_op_amount = num_rand(rng);
    batch_file << format("\top_t ops_batch%d[%d];\n", batch, batch_op_amount);
    for(int i = 0 ; i < batch_op_amount; i++){
      int key = num_rand(rng);
      int op = op_rand(rng);
      batch_file << format("\toper.key = %d; oper.data = names[%d]; oper.op = %d; oper.compute_func = hash_test; oper.result = 0;\n", key, name_rand(rng), op);
      batch_file << format("\tops_batch%d[i++] = oper;\n", batch);
    }

    batch_file << format("\tlist_batch(list, %d, ops_batch%d); i = 0;\n", batch_op_amount, batch);
  }
  batch_file << "\n\tlist_free(list);";
  batch_file << main_tail;
  batch_file.close();

  system("gcc -std=c99 -g -o batch_test.out batch_test.c my_list.c -lpthread");
  if(isValgrindOn)
    system("valgrind --leak-check=full ./batch_test.out");
  else
    system("./batch_test.out");
}
struct Entry{
  Operation op;
  int name;
};

void injectTest(ofstream& test_file, Operation next_op, int name, int key, int res, int size, bool success){
  string line = "";
  switch(next_op){
    case Operation::INSERT:
    line = format("(list_insert(list, %d, names[%d]));", key, name);
    break;
    case Operation::REMOVE:
    line = format("(list_remove(list, %d));", key);
    break;
    case Operation::CONTAINS:
    line = format("(list_find(list, %d) == %d);", key, success);
    break;
    case Operation::SIZE:
    line = format("(list_size(list) == %d);", size);
    break;
    case Operation::UPDATE:
    line = format("(list_update(list, %d, names[%d]));", key, name);
    break;
    case Operation::COMPUTE:
    test_file << format("\tres = %d;\n", res);
    line = format("(list_compute(list, %d, hash_test, &res));\n\tASSERT_TEST(res == %d);", key, res);
    break;
  }
  string test = "";
  if(next_op != Operation::SIZE && next_op != Operation::CONTAINS)
    test = format("\tASSERT_%s", success ? "ZERO" : "NON_ZERO");
  else
    test = "\tASSERT_TEST";
  test_file << test << line << "\n";
}

struct BatchEntry{
  unordered_set<int> allowedOps;
  int key;
  bool inBatch;
};

void generateBatchTest(ostream& test_file, vector<string>& names, unordered_set<int>& key_pool, map<int, Entry>& current_state, int batch_number, int& size){
  uniform_int_distribution<int> ops_per_batch(2, g_num_of_threads);
  uniform_int_distribution<int> key_rand(-100 , 100);
  uniform_int_distribution<int> name_rand(0, names.size() - 2);
  uniform_int_distribution<int> op_rand(0, 4);

  int ops = ops_per_batch(rng);

  map<int, BatchEntry> batch_state;

  unordered_set<int> allOps = {(int)Operation::INSERT, (int)Operation::REMOVE, (int)Operation::CONTAINS, (int)Operation::UPDATE, (int)Operation::COMPUTE};

  for(int key : key_pool){
    BatchEntry be = {allOps,key,false};
    batch_state[key] = be;
  }

  test_file << format("\top_t* ops%d = (op_t*) malloc(sizeof(*ops%d) * %d);\n", batch_number, batch_number, ops);
  test_file << format("\tif(ops%d == NULL){ list_free(list); return false; }\n", batch_number);

  for(int i = 0; i < ops ; i++){
    int name = names.size() - 1;
    int key = key_rand(rng);
    if(!batch_state.count(key)){
      BatchEntry be = {allOps, key, false};
      batch_state[key] = be;
    }

    bool wasPresent = key_pool.count(key);

    Operation op;
    bool isOk = false;
    do{
      op = (Operation)op_rand(rng);
      switch((int)op){
        case 0:
        if(batch_state[key].allowedOps.count((int)Operation::INSERT)){
          batch_state[key].allowedOps.erase((int)Operation::INSERT);
          batch_state[key].allowedOps.erase((int)Operation::REMOVE);
          batch_state[key].allowedOps.erase((int)Operation::UPDATE);
          if(!wasPresent){
            ++size;
            key_pool.insert(key);
          }
          isOk = true;
        }
        break;
        case 1:
        if(batch_state[key].allowedOps.count((int)Operation::REMOVE)){
          batch_state[key].allowedOps.erase((int) Operation::UPDATE);
          batch_state[key].allowedOps.erase((int)Operation::INSERT);
          if(key_pool.count(key)){
            --size;
            current_state.erase(key);
            key_pool.erase(key);
          }
          isOk = true;
        }
        break;
        case 3:
        if(batch_state[key].allowedOps.count((int) Operation::UPDATE)){
            batch_state[key].allowedOps.erase((int) Operation::UPDATE);
            batch_state[key].allowedOps.erase((int)Operation::INSERT);
            batch_state[key].allowedOps.erase((int) Operation::REMOVE);
            isOk = true;
        }
        break;
        default:
        isOk = true;
      }
    } while(!isOk);

    if(op == Operation::INSERT || op == Operation::UPDATE){
      name = name_rand(rng);
      if((op == Operation::UPDATE && wasPresent) || (op == Operation::INSERT && !wasPresent)){
        current_state[key].name = name;
        current_state[key].op = op;
      }
    }

    test_file << format("\toper.key = %d; oper.data = names[%d]; oper.op = %s; oper.compute_func = hash_test; oper.result = 0;\n", key, name, opToString((int)op).c_str());
    test_file << format("\tops%d[%d] = oper;\n", batch_number, i, batch_number);
  }
  test_file << format("\tlist_batch(list, %d, ops%d);\n", ops, batch_number);
  test_file << format("\tfree(ops%d);\n", batch_number);
}

static int hash_test(const void* data){
  char* str = const_cast<char*>(static_cast<const char*>(data));
  int hash = 7;
  for (int i = 0; i < strlen(str) ; i++) {
      hash = hash*31 + str[i];
  }
  return hash;
}

void generateTest(ofstream& test_file,vector<string>& names){
  uniform_int_distribution<int> key_rand(-100 , 100);
  uniform_int_distribution<int> name_rand(0, names.size() - 2);
  uniform_int_distribution<int> op_rand(0, 6);

  map<int, Entry> current_state = map<int, Entry>();
  unordered_set<int> key_pool;
  int batch_number = 0;
  int size = 0;
  current_state.clear();
  for(int line = 0 ; line < g_num_of_list_operations ; line++){
    Operation next_op = (Operation)op_rand(rng);
    if(next_op != Operation::BATCH){
      int next_name = name_rand(rng);
      int next_key = key_rand(rng);
      bool success = true;
      int res = 0;

      switch(next_op){
        case Operation::INSERT:
        success = !(current_state.count(next_key));
        if(success && key_pool.find(next_key) == key_pool.end()){
          Entry e = {Operation::INSERT, next_name};
          current_state[next_key] = e;
          key_pool.insert(next_key);
          size++;
        }
        break;
        case Operation::REMOVE:
        success = (current_state.count(next_key));
        if(success && key_pool.find(next_key) != key_pool.end()){
          current_state.erase(next_key);
          key_pool.erase(next_key);
          size--;
        }
        break;
        case Operation::CONTAINS:
        success = (current_state.count(next_key));
        if(success && key_pool.find(next_key) != key_pool.end()){
          current_state[next_key].op = Operation::CONTAINS;
        }
        break;
        case Operation::UPDATE:
        success = (current_state.count(next_key));
        if(success && key_pool.find(next_key) != key_pool.end()){
          current_state[next_key].op = Operation::UPDATE;
          current_state[next_key].name = next_name;
        }
        break;
        case Operation::COMPUTE:
        success = (current_state.count(next_key));
        if(success && key_pool.find(next_key) != key_pool.end()){
          current_state[next_key].op = Operation::COMPUTE;
          res = hash_test(names[current_state[next_key].name].c_str());
        }
        else res = 0;
        break;
      }
      injectTest(test_file, next_op, next_name, next_key, res, size, success);
    }
    else{
      // Batch will test multi-threading, the real deal
      generateBatchTest(test_file, names, key_pool, current_state, ++batch_number, size);
    }
  }
}

void createAndRunNonDeterministicTest(vector<string>& names){
  ofstream test_file;
  string file_name = format("random_test_%d_lines", g_num_of_list_operations);
  test_file.open(format("%s.c",file_name.c_str()));
  test_file << program_head; // Initialize program

  test_file << "bool actualTest(){\n";
  write_names(test_file, names);
  test_file << "\tint res = 0;\n";
  test_file << "\top_t oper;\n";
  test_file << "\tlinked_list_t* list = list_alloc();\n\tif(!list) return false;\n\n\n";
  generateTest(test_file, names);
  test_file << "\n\tlist_free(list);\n\treturn true;\n}\n";
  test_file << main_head;
  test_file << "\tRUN_TEST(actualTest);";
  test_file << main_tail;

  test_file.close();

  system(format("gcc -std=c99 -g -o %s.out %s.c my_list.c -lpthread", file_name.c_str(), file_name.c_str()).c_str());
  if(isValgrindOn)
    system(format("valgrind --leak-check=full ./%s.out", file_name.c_str()).c_str());
  else
    system(format("./%s.out", file_name.c_str()).c_str());
}

#define RED_START "\033[1;31m"
#define MAGENTA_START "\033[1;35m"
#define GREEN_START "\033[1;32m"
#define COLOR_END "\033[0m"

void MyparseError(){
	cerr << RED_START << ">>>Usage: [-valgrind] [-ops={num_of_ops}] [-th={num_of_threads}]" << endl;
    cerr << ">>>Parameters: 0 < num_of_ops <= 12000 and 0 < num_of_threads <= 30" << endl;
   	cerr << ">>>Try again." << COLOR_END << endl;
   	exit(1);
}

bool is_number(const std::string& s)
{
    std::string::const_iterator it = s.begin();
    if(*it == '-') it++;
    while (it != s.end() && std::isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
}

void parseOptions(char** options, int size){
	for(int i=0;i<size;++i){
		string optionStr = string(options[i]);
		if(optionStr.compare(0,4,"-th=") == 0){
			if(!is_number(optionStr.substr(4))) MyparseError();
			g_num_of_threads = atoi(optionStr.substr(4).c_str());
			if(g_num_of_threads <=0) {
				cout << MAGENTA_START << ">>>NOTE: num_of_threads is at least 1, changing to 1" << COLOR_END << endl;
				g_num_of_threads = 1;
			}
			if(g_num_of_threads > 30){
				cout << MAGENTA_START << ">>>NOTE: num_of_threads is at most 30, changing to 30" << COLOR_END << endl;
				g_num_of_threads = 30;
			}
		}
		else if(optionStr.compare(0,5,"-ops=") == 0){
			if(!is_number(optionStr.substr(5))) MyparseError();
			g_num_of_list_operations = atoi(optionStr.substr(5).c_str());
			if(g_num_of_list_operations <=0) {
				cout << MAGENTA_START << ">>>NOTE: num_of_list_operations is at least 1, changing to 1" << COLOR_END << endl;
				g_num_of_list_operations = 1;
			}
			if(g_num_of_list_operations > 12000){
      			cout << MAGENTA_START << ">>>NOTE: num_of_list_operations at most 12000, changing to 12000" << COLOR_END << endl;
      			g_num_of_list_operations = 12000;
    		}		
		}
		else if(optionStr.compare("-valgrind")==0){
			isValgrindOn = true;
		}
		else{
			MyparseError();
		}
	}
}
int main(int argc, char **argv){
 // ./a.out -t=30 -op=12000 -valgrind
	if(argc > 1){
		parseOptions(argv+1,argc-1);
	}

	cout << GREEN_START << format(">>>RUNNING WITH: THREADS = %d, OPS = %d, VALGRIND = %s", g_num_of_threads, g_num_of_list_operations, isValgrindOn ? "ON" : "OFF") << COLOR_END << endl;

  vector<string> name_stash;
  initialize_names(name_stash);

  cout << GREEN_START << ">>>INFO: Generating and running batch test, created test - batch_test.c" << COLOR_END << endl;
  createAndRunNonDeterministicBatch(name_stash);
  cout << GREEN_START << format(">>>INFO: Generating and running regular test, created test - random_test_%d_lines.c", g_num_of_list_operations) << COLOR_END << endl;
  createAndRunNonDeterministicTest(name_stash);

  return 0;
}
