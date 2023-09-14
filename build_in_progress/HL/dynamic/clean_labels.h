#pragma once
#include <build_in_progress/HL/dynamic/two_hop_labels_base.h>




/*clean L*/

pair<weightTYPE, int> clean_L_extract_distance(vector<two_hop_label_v1>& L_s, vector<two_hop_label_v1>& L_t) {

	/*return std::numeric_limits<double>::max() is not connected*/

	weightTYPE distance = std::numeric_limits<weightTYPE>::max(); // if disconnected, return this large value
	int common_hub;

	auto vector1_check_pointer = L_s.begin(), vector2_check_pointer = L_t.begin();
	auto pointer_L_s_end = L_s.end(), pointer_L_t_end = L_t.end();
	while (vector1_check_pointer != pointer_L_s_end && vector2_check_pointer != pointer_L_t_end) {
		if (vector1_check_pointer->vertex == vector2_check_pointer->vertex) {
			weightTYPE dis = vector1_check_pointer->distance + vector2_check_pointer->distance;
			if (distance > dis) {
				distance = dis;
				common_hub = vector1_check_pointer->vertex;
			}
			vector1_check_pointer++;
		}
		else if (vector1_check_pointer->vertex > vector2_check_pointer->vertex) {
			vector2_check_pointer++;
		}
		else {
			vector1_check_pointer++;
		}
	}

	return { distance , common_hub };
}

void clean_L_element(int v, vector<vector<two_hop_label_v1>>& L, PPR_type& PPR) {

	vector<two_hop_label_v1> Lv_final;
	mtx_595[v].lock();
	vector<two_hop_label_v1> Lv = L[v];
	mtx_595[v].unlock();

	int size = Lv.size();
	for (int i = 0; i < size; i++) {
		int u = Lv[i].vertex;
		if (v == u) {
			Lv_final.push_back(Lv[i]);
			continue;
		}
		mtx_595[u].lock();
		vector<two_hop_label_v1> Lu = L[u];
		mtx_595[u].unlock();
		pair<weightTYPE, int> query = clean_L_extract_distance(Lv_final, Lu);
		if (query.first > Lv[i].distance + 1e-5) {
			Lv_final.push_back(Lv[i]);
		}
		else {
			if (query.second != u) {
				mtx_5952[v].lock();
				PPR_insert(PPR, v, query.second, u);
				mtx_5952[v].unlock();
			}
			if (query.second != v) {
				mtx_5952[u].lock();
				PPR_insert(PPR, u, query.second, v);
				mtx_5952[u].unlock();
			}
		}
	}
	mtx_595[v].lock();
	vector<two_hop_label_v1>(Lv_final).swap(L[v]);
	mtx_595[v].unlock();
}

void clean_L_dynamic(vector<vector<two_hop_label_v1>>& L, PPR_type& PPR, ThreadPool& pool_dynamic, std::vector<std::future<int>>& results_dynamic) {

	int N = L.size();

	for (int target_v = 0; target_v < N; target_v++) {
		results_dynamic.emplace_back(
			pool_dynamic.enqueue([target_v, &L, &PPR] { // pass const type value j to thread; [] can be empty
				clean_L_element(target_v, L, PPR);
				return 1; // return to results; the return type must be the same with results
				}));
	}
	for (auto&& result : results_dynamic)
		result.get(); // all threads finish here
	results_dynamic.clear();
}

