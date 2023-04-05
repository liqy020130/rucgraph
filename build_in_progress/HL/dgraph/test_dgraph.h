#pragma once

/*the following codes are for testing

---------------------------------------------------
a cpp file (try.cpp) for running the following test code:
----------------------------------------

#include <iostream>
#include <fstream>
using namespace std;
#include <boost/random.hpp>
boost::random::mt19937 boost_random_time_seed{ static_cast<std::uint32_t>(std::time(0)) };

#include <build_in_progress/HL/dgraph/test_dgraph.h>

int main()
{
    test_dgraph_PLL_PSL();
}

------------------------------------------------------------------------------------------
Commends for running the above cpp file on Linux:

g++ -std=c++17 -I/home/boost_1_75_0 -I/home/dengs/dgraph try.cpp -lpthread -Ofast -o A
./A
rm A

(optional to put the above commends in run.sh, and then use the comment: sh run.sh)
*/

#include <unordered_map>
#include <dgraph_v_of_v/dgraph_v_of_v.h>
#include <dgraph_v_of_v/dgraph_generate_random_dgraph.h>
#include <dgraph_v_of_v/dgraph_save_dgraph.h>
#include <dgraph_v_of_v/dgraph_read_dgraph.h>
#include <build_in_progress/HL/dgraph/dgraph_change_IDs.h>
#include <dgraph_v_of_v/dgraph_shortest_paths.h>
#include <build_in_progress/HL/dgraph/dgraph_two_hop_label.h>
#include <build_in_progress/HL/dgraph/dgraph_PLL.h>
#include <build_in_progress/HL/dgraph/dgraph_PSL.h>



/*check_correctness*/

template <typename weight_type>
void dgraph_v1_check_correctness(dgraph_case_info_v1& case_info, dgraph_v_of_v<weight_type>& instance_graph, int iteration_source_times, int iteration_terminal_times) {

    /*below is for checking whether the above labels are right (by randomly computing shortest distances)*/

    boost::random::uniform_int_distribution<> dist{ static_cast<int>(0), static_cast<int>(instance_graph.INs.size() - 1) };

    for (int yy = 0; yy < iteration_source_times; yy++) {
        int source = dist(boost_random_time_seed);
        //source = 3; //cout << "source = " << source << endl;

        auto distances = dgraph_shortest_distances_source_to_all(instance_graph, source);

        for (int xx = 0; xx < iteration_terminal_times; xx++) {
            int terminal = dist(boost_random_time_seed);
            //terminal = 4; //cout << "terminal = " << terminal << endl;

            weight_type dis = dgraph_v1_extract_shortest_distance(case_info.L_in, case_info.L_out, source, terminal);

            if (abs(dis - distances[terminal]) > 1e-4) {
                cout << "source = " << source << endl;
                cout << "terminal = " << terminal << endl;
                cout << "source vector:" << endl;
                for (auto it = case_info.L_out[source].begin(); it != case_info.L_out[source].end(); it++) {
                    cout << "<" << it->vertex << "," << it->distance << ">";
                }
                cout << endl;
                cout << "terminal vector:" << endl;
                for (auto it = case_info.L_in[terminal].begin(); it != case_info.L_in[terminal].end(); it++) {
                    cout << "<" << it->vertex << "," << it->distance << ">";
                }
                cout << endl;

                cout << "dis = " << dis << endl;
                cout << "distances[terminal] = " << distances[terminal] << endl;
                cout << "abs(dis - distances[terminal]) > 1e-5!" << endl;
                getchar();
            }
        }
    }
}

void test_dgraph_PLL_PSL() {
    /*parameters*/
    int iteration_graph_times = 100, iteration_source_times = 100, iteration_terminal_times = 100;
    int V = 1000, E = 5000, precision = 1, thread_num = 10;
    two_hop_weight_type ec_min = 0.1, ec_max = 1;

    double avg_index_time = 0, avg_index_size_per_v = 0;

    bool use_PLL = 0; // 1: PLL 0: PSL

    dgraph_case_info_v1 mm;
    mm.use_canonical_repair = 1;

    /*iteration*/
    for (int i = 0; i < iteration_graph_times; i++) {
        cout << i << endl;

        /*input and output; below is for generating random new graph, or read saved graph*/
        int generate_new_graph = 1;

        dgraph_v_of_v<two_hop_weight_type> instance_graph;

        if (generate_new_graph == 1) {          
            instance_graph = dgraph_generate_random_dgraph(V, E, ec_min, ec_max, precision, boost_random_time_seed);
            dgraph_change_IDs_sum_IN_OUT_degrees(instance_graph); //id������õ�
            dgraph_save_dgraph("random_dgraph_test.txt", instance_graph);
        }
        else {
            dgraph_read_dgraph("random_dgraph_test.txt", instance_graph);
        }

        auto begin = std::chrono::high_resolution_clock::now();
        try {
            if (use_PLL) {
                dgraph_PLL(instance_graph, thread_num, mm);
            }
            else {
                dgraph_PSL(instance_graph, thread_num, mm);
            }
        }
        catch (string s) {
            cout << s << endl;
            dgraph_clear_global_values_PLL_PSL();
            continue;
        }
        auto end = std::chrono::high_resolution_clock::now();
        double runningtime = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count() / 1e9; // s
        avg_index_time = avg_index_time + runningtime / iteration_graph_times;

        dgraph_v1_check_correctness(mm, instance_graph, iteration_source_times, iteration_terminal_times);

        long long int index_size = 0;
        for (auto it = mm.L_in.begin(); it != mm.L_in.end(); it++)
        {
            index_size = index_size + (*it).size();
        }

        for (auto it = mm.L_out.begin(); it != mm.L_out.end(); it++)
        {
            index_size = index_size + (*it).size();
        }

        avg_index_size_per_v = avg_index_size_per_v + (double)index_size / V / iteration_graph_times;

        mm.clear_labels();
    }

    if (use_PLL) {
        cout << "V = " << V << " E = " << E << " thread_num = " << thread_num << " PLL avg_index_time = " << avg_index_time << "s" << endl;
    }
    else {
        cout << "V = " << V << " E = " << E << " thread_num = " << thread_num << " PSL avg_index_time = " << avg_index_time << "s" << endl;
    }
}




/*compare_different_sorting_method*/

double avg_query_time(int query_times, int N, vector<vector<two_hop_label>>& L_in, vector<vector<two_hop_label>>& L_out) {

    boost::random::uniform_int_distribution<> dist{ static_cast<int>(0), static_cast<int>(N-1) };

    double avg_query_time_ms = 0;

    for (int i = 0; i < query_times; i++) {
        int source = dist(boost_random_time_seed), terminal = dist(boost_random_time_seed);;
        auto begin = std::chrono::high_resolution_clock::now();
        dgraph_v1_extract_shortest_distance(L_in, L_out, source, terminal);
        auto end = std::chrono::high_resolution_clock::now();
        double runningtime = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count() / 1e6; // ms
        avg_query_time_ms+= runningtime / query_times;
    }

    return avg_query_time_ms;
}

void compare_different_sorting_method() {
    /*parameters*/
    int iteration_graph_times = 100, query_times_per_g = 1e4;
    int V = 1000, E = 5000, precision = 1, thread_num = 10;
    two_hop_weight_type ec_min = 0.1, ec_max = 1;

    double degree_order_avg_index_time = 0, degree_order_avg_label_bit_size = 0, degree_order_avg_query_time = 0,
        weighted_degree_order_avg_index_time = 0, weighted_degree_order_avg_label_bit_size = 0, weighted_degree_order_avg_query_time = 0;

    bool use_PLL = 1; // 1: PLL 0: PSL

    dgraph_case_info_v1 mm;
    mm.use_canonical_repair = 1;

    /*iteration*/
    for (int i = 0; i < iteration_graph_times; i++) {
        cout << i << endl;

        dgraph_v_of_v<two_hop_weight_type> instance_graph = dgraph_generate_random_dgraph(V, E, ec_min, ec_max, precision, boost_random_time_seed);

        /*degree order*/
        if (1) {
            dgraph_change_IDs_sum_IN_OUT_degrees(instance_graph); 
            auto begin = std::chrono::high_resolution_clock::now();
            try {
                if (use_PLL) {
                    dgraph_PLL(instance_graph, thread_num, mm);
                }
                else {
                    dgraph_PSL(instance_graph, thread_num, mm);
                }
            }
            catch (string s) {
                cout << s << endl;
                dgraph_clear_global_values_PLL_PSL();
                continue;
            }
            auto end = std::chrono::high_resolution_clock::now();
            double runningtime = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count() / 1e9; // s
            degree_order_avg_index_time += runningtime / iteration_graph_times;
            degree_order_avg_label_bit_size += (double)compute_label_bit_size(mm.L_in,mm.L_out) / iteration_graph_times;
            degree_order_avg_query_time += (double)avg_query_time(query_times_per_g, V, mm.L_in, mm.L_out) / iteration_graph_times;
            mm.clear_labels();
        }

        /*weighted degree order*/
        if (1) {
            dgraph_change_IDs_weighted_degrees(instance_graph);
            auto begin = std::chrono::high_resolution_clock::now();
            try {
                if (use_PLL) {
                    dgraph_PLL(instance_graph, thread_num, mm);
                }
                else {
                    dgraph_PSL(instance_graph, thread_num, mm);
                }
            }
            catch (string s) {
                cout << s << endl;
                dgraph_clear_global_values_PLL_PSL();
                continue;
            }
            auto end = std::chrono::high_resolution_clock::now();
            double runningtime = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count() / 1e9; // s
            weighted_degree_order_avg_index_time += runningtime / iteration_graph_times;
            weighted_degree_order_avg_label_bit_size += (double)compute_label_bit_size(mm.L_in, mm.L_out) / iteration_graph_times;
            weighted_degree_order_avg_query_time += (double)avg_query_time(query_times_per_g, V, mm.L_in, mm.L_out) / iteration_graph_times;
            mm.clear_labels();
        }

    }

    cout << "V = " << V << " E = " << E << " thread_num = " << thread_num << " use_PLL = " << use_PLL << endl;
    cout << "degree_order:  avg_index_time = " << degree_order_avg_index_time << "s" << " avg_label_bit_size = " << degree_order_avg_label_bit_size << "bit" 
        << " avg_query_time = " << degree_order_avg_query_time << "ms" << endl;
    cout << "weighted_degree_order: avg_index_time = " << weighted_degree_order_avg_index_time << "s" << " avg_label_bit_size = " << weighted_degree_order_avg_label_bit_size << "bit" 
        << " avg_query_time = " << weighted_degree_order_avg_query_time << "ms" << endl;
}




