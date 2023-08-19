#pragma once

/*the following codes are for testing

---------------------------------------------------
a cpp file (try.cpp) for running the following test code:
----------------------------------------

#include <iostream>
#include <fstream>
using namespace std;

// header files in the Boost library: https://www.boost.org/
#include <boost/random.hpp>
boost::random::mt19937 boost_random_time_seed{ static_cast<std::uint32_t>(std::time(0)) };

#include <build_in_progress/HL/dynamic/exp_generate_graphs.h>

int main()
{
	;
}

------------------------------------------------------------------------------------------
Commends for running the above cpp file on Linux:

g++ -std=c++17 -I/home/boost_1_75_0 -I/root/rucgraph try.cpp -lpthread -O3 -o A
./A
rm A

(optional to put the above commends in run.sh, and then use the comment: sh run.sh)


*/
#include <iostream>
#include <iomanip>
#include <fstream>
#include <ctime>
#include <cstdint>
#include <cstdlib>
#include <numeric>
#include <string>
#include <list>
#include <vector>
#include <tuple>
#include <algorithm>
#include <iterator>
#include <chrono>
#include <typeinfo>
#include <unordered_set>
#include <unordered_map>
#include <climits>
#include <math.h>
#include <thread>
#include <chrono>
#include <shared_mutex>
using namespace std;
#include <build_in_progress/HL/dynamic/PLL_dynamic.h>
#include <text_mining/parse_string.h>
#include <graph_hash_of_mixed_weighted/read_save/graph_hash_of_mixed_weighted_binary_save_read.h>

int max_ec = 100;
std::shared_mutex glock;

void update_Jacard_ec_element(graph_hash_of_mixed_weighted* input_graph, int i,
	vector<pair<pair<int, int>, double>>* input_new_ec, vector<vector<pair<int, double>>>* input_adj_lists) {

	auto& graph = *input_graph;
	auto& new_ec = *input_new_ec;
	auto& adj_lists = *input_adj_lists;

	std::vector<int> adj_v1 = graph.adj_v(i);
	for (int x = 0; x < adj_v1.size(); x++) {
		int j = adj_v1[x];
		if (i < j) {
			int V_i_cap_V_j = 0;
			/*merge join style compare*/
			auto vector_i_check_pointer = adj_lists[i].begin();
			auto vector_j_check_pointer = adj_lists[j].begin();
			auto pointer_vector_i_end = adj_lists[i].end();
			auto pointer_vector_j_end = adj_lists[j].end();
			while (vector_i_check_pointer != pointer_vector_i_end && vector_j_check_pointer != pointer_vector_j_end) {
				if (vector_i_check_pointer->first == vector_j_check_pointer->first) {
					V_i_cap_V_j++;
					vector_i_check_pointer++;
				}
				else if (vector_i_check_pointer->first > vector_j_check_pointer->first) {
					vector_j_check_pointer++;
				}
				else {
					vector_i_check_pointer++;
				}
			}
			int V_i_cup_V_j = adj_lists[i].size() + adj_lists[j].size() - V_i_cap_V_j;
			double ec = 1 - (double)V_i_cap_V_j / V_i_cup_V_j;

			glock.lock();
			new_ec.push_back({ {i, j} ,ec });
			glock.unlock();
		}
	}

}

graph_hash_of_mixed_weighted update_Jacard_ec(graph_hash_of_mixed_weighted graph) {

	ThreadPool pool(20);
	std::vector< std::future<int> > results; // return typename: xxx

	vector<pair<pair<int, int>, double>> new_ec;

	vector<vector<pair<int, double>>> adj_lists(graph.hash_of_vectors.size() + 1); // sorted lists
	for (auto it1 = graph.hash_of_vectors.begin(); it1 != graph.hash_of_vectors.end(); it1++) {
		adj_lists[it1->first] = graph.adj_v_and_ec(it1->first);
		/*if graph.hash_of_vectors.size() < number of vertices (some vertices are not added) then bugs here
		, this is due to the fact that some vertices are isolated, and could not be added by adding edges,*/
	}

	graph_hash_of_mixed_weighted* input_graph = &graph;
	vector<pair<pair<int, int>, double>>* input_new_ec = &new_ec;
	vector<vector<pair<int, double>>>* input_adj_lists = &adj_lists;
	for (auto it1 = graph.hash_of_vectors.begin(); it1 != graph.hash_of_vectors.end(); it1++) {
		int i = it1->first;
		results.emplace_back(
			pool.enqueue([input_graph, i, input_new_ec, input_adj_lists] { // pass const type value j to thread; [] can be empty
				update_Jacard_ec_element(input_graph, i, input_new_ec, input_adj_lists);
				return 1; // return to results; the return type must be the same with results
				})
		);
	}
	for (auto&& result : results)
		result.get(); //all threads finish here

	int size = new_ec.size();
	for (int i = 0; i < size; i++) {
		graph_hash_of_mixed_weighted_add_edge(graph, new_ec[i].first.first, new_ec[i].first.second, new_ec[i].second);
	}

	return graph;
}


void save_data(graph_hash_of_mixed_weighted& graph_random, graph_hash_of_mixed_weighted& graph_Jacard, string save_name) {

	string save_file_name;
	ofstream outputFile;
	outputFile.precision(6);
	outputFile.setf(ios::fixed);
	outputFile.setf(ios::showpoint);

	int V = graph_hash_of_mixed_weighted_num_vertices(graph_random);
	int E = graph_hash_of_mixed_weighted_num_edges(graph_random);
	save_file_name = save_name + "_graph.txt";
	outputFile.open(save_file_name);
	outputFile << "V=" << V << " E=" << E << endl;
	outputFile << "Vetex_1	Vertex_2	Random_weight	Jacard_weight" << endl;
	for (int i = 0; i < V; i++) {
		auto x = graph_random.adj_v_and_ec(i);
		auto y = graph_Jacard.adj_v_and_ec(i);
		for (int j = 0; j < x.size(); j++) {
			if (x[j].first != y[j].first) {
				cout << "graph_random edges are different from graph_Jacard edges!" << endl;
			}
			outputFile << i << "	" << x[j].first << "	" << x[j].second << "	" << y[j].second << endl;
		}
	}
	outputFile.close();
}


/*google*/

void generate_google() {

	string data_name = "google";

	unordered_map<int, int> old_id_to_new_id;
	vector<pair<int, int>> edges_new_id;

	string file_name = "C:\\Users\\Yahui\\Documents\\Drive2\\DynamicHL_data\\web-Google.txt\\web-Google.txt";
	ifstream myfile(file_name); // open the file
	string line_content;
	if (myfile.is_open()) // if the file is opened successfully
	{
		int count = 0;
		while (getline(myfile, line_content)) // read file line by line
		{
			if (count >= 4) {
				std::vector<string> Parsed_content = parse_string(line_content, "	"); // parse line_content
				int id1 = stoi(Parsed_content[0]);
				int id2 = stoi(Parsed_content[1]);
				int new_id1, new_id2;
				if (old_id_to_new_id.count(id1) > 0) {
					new_id1 = old_id_to_new_id[id1];
				}
				else {
					int size = old_id_to_new_id.size();
					new_id1 = size;
					old_id_to_new_id[id1] = new_id1;
				}
				if (old_id_to_new_id.count(id2) > 0) {
					new_id2 = old_id_to_new_id[id2];
				}
				else {
					int size = old_id_to_new_id.size();
					new_id2 = size;
					old_id_to_new_id[id2] = new_id2;
				}
				edges_new_id.push_back({ new_id1, new_id2 });
			}
			count++;
		}
		myfile.close(); //close the file
	}
	else
	{
		std::cout << "Unable to open file " << file_name << endl << "Please check the file location or file name." << endl; // throw an error message
		getchar(); // keep the console window
		exit(1); // end the program
	}

	graph_hash_of_mixed_weighted new_id_graph_random;
	boost::random::uniform_int_distribution<> dist{ 1, max_ec };
	for (auto p : edges_new_id) {
		graph_hash_of_mixed_weighted_add_edge(new_id_graph_random, p.first, p.second, dist(boost_random_time_seed));
	}

	cout << data_name << ":" << endl;
	cout << "num_vertices(read_graph): " << graph_hash_of_mixed_weighted_num_vertices(new_id_graph_random) << endl;
	cout << "num_edges(read_graph): " << graph_hash_of_mixed_weighted_num_edges(new_id_graph_random) << endl;

	graph_hash_of_mixed_weighted new_id_graph_Jacard = update_Jacard_ec(new_id_graph_random);

	graph_hash_of_mixed_weighted_binary_save(new_id_graph_random, data_name + "_random.bin");
	graph_hash_of_mixed_weighted_binary_save(new_id_graph_Jacard, data_name + "_Jacard.bin");
	save_data(new_id_graph_random, new_id_graph_Jacard, data_name);
}







