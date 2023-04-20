#pragma once
#include <algorithm>
#include <iostream>
#include <limits>
#include <vector>
#include <utility>
#include <string>
#include <algorithm>
#include <unordered_set>
#include <dgraph_v_of_v/dgraph_v_of_v.h>
#include <build_in_progress/HL/dgraph/dgraph_two_hop_label.h>
#include <build_in_progress/HL/dgraph/dgraph_PLL.h>

struct node_degree
{
    int vertex, degree;
    bool operator<(const node_degree &nd) const
    {
        return degree > nd.degree;
    }
};

class dgraph_case_info_v2
{
  public:
    /* parameters */
    int thread_num;
    bool use_PLL;
    int d;

    /* labels */
    dgraph_case_info_v1 two_hop_case_info;
    dgraph_v_of_v<two_hop_weight_type> core_graph;
    std::vector<std::vector<std::pair<int, two_hop_weight_type>>> Bags_in, Bags_out;
    std::vector<bool> isIntree;
    std::vector<int> root;

    std::vector<std::vector<int>> tree_st;   // for lca
    std::vector<std::vector<int>> tree_st_r; // for lca
    std::vector<int> first_pos;              // for lca
    std::vector<int> lg;                     // for lca
    std::vector<int> dep;                    // for lca

    /*running limits*/
    long long int max_bit_size = 1e12;
    double max_run_time_seconds = 1e12; // s

    /*compute label size*/
    // long long int compute_label_bit_size()
    // {
    //     long long int size = 0;
    //     size = size + two_hop_case_info.compute_label_bit_size();
    //     return size;
    // }

    /*clear labels*/
    void clear_labels()
    {
        two_hop_case_info.clear_labels();
        core_graph.clear();
        std::vector<std::vector<pair<int, two_hop_weight_type>>>().swap(Bags_in);
        std::vector<std::vector<pair<int, two_hop_weight_type>>>().swap(Bags_out);
        vector<bool>().swap(isIntree);
        vector<int>().swap(root);
        vector<vector<int>>().swap(tree_st);
        vector<vector<int>>().swap(tree_st_r);
        vector<int>().swap(first_pos);
        vector<int>().swap(lg);
        vector<int>().swap(dep);
    }

    /*indexing times*/
    double time1_initialization = 0;
    double time2_tree_decomposition = 0;
    double time3_tree_indexs = 0;
    double time4_lca = 0;
    double time5_core_indexs = 0;
    double time6_post = 0;
    double time_total = 0;

    /*printing*/
    void print_Bags()
    {
        cout << "print_Bags:" << endl;
        for (int i = 0; i < Bags_in.size(); i++)
        {
            cout << i << ": ";
            for (int j = 0; j < Bags_in[i].size(); j++)
            {
                cout << Bags_in[i][j].first << ", ";
            }
            cout << "||";
            for (int j = 0; j < Bags_out[i].size(); j++)
            {
                cout << Bags_out[i][j].first << ", ";
            }
            cout << endl;
        }
    }

    void print_root()
    {
        cout << "print_root:" << endl;
        for (int i = 0; i < root.size(); i++)
        {
            cout << "root[" << i << "]=" << root[i] << endl;
        }
    }

    void print_isIntree()
    {
        cout << "print_isIntree:" << endl;
        for (int i = 0; i < isIntree.size(); i++)
        {
            cout << "isIntree[" << i << "]=" << isIntree[i] << endl;
        }
    }

    /*record_all_details*/
};

/*global values*/
dgraph_v_of_v<two_hop_weight_type> global_dgraph_CT;
std::vector<std::pair<int, int>> added_max_edge;

void clear_gloval_values_CT()
{
    global_dgraph_CT.clear();
    std::vector<std::pair<int, int>>().swap(added_max_edge);
}

void substitute_parallel(int u, int w, two_hop_weight_type ec)
{
    if (global_dgraph_CT.contain_edge(u, w) == 0) {
        global_dgraph_CT.add_edge(u, w, ec);
    }
    else{
        if (global_dgraph_CT.edge_weight(u, w) > ec)
            global_dgraph_CT.add_edge(u, w, ec);
    }
}

void substitute_parallel_2(int u, int w, two_hop_weight_type ec)
{
    if (global_dgraph_CT.contain_edge(u, w) == 0 && global_dgraph_CT.contain_edge(w, u) == 0) {
        global_dgraph_CT.add_edge(u, w, std::numeric_limits<two_hop_weight_type>::max());
    }
}

void remove_parallel(int u, int w)
{
    global_dgraph_CT.remove_edge(u, w);
}

void dfs(int &total, vector<int> &first_pos, int x, vector<vector<int>> &son, vector<int> &dfn)
{
    total++;
    dfn[total] = x;
    first_pos[x] = total;
    int s_size = son[x].size();
    for (int i = 0; i < s_size; i++)
    {
        dfs(total, first_pos, son[x][i], son, dfn); // this is a recursive function
        total++;
        dfn[total] = x;
    }
}

/*indexing function*/
void CT_dgraph(dgraph_v_of_v<two_hop_weight_type> &input_graph, int max_N_ID, dgraph_case_info_v2 &case_info)
{

    //--------------------------- step 1: initialization ---------------------------
    // cout << "step 1: initialization" << endl;
    auto begin1 = std::chrono::high_resolution_clock::now();

    auto &Bags_in = case_info.Bags_in;
    auto &Bags_out = case_info.Bags_out;
    auto &isIntree = case_info.isIntree;
    auto &root = case_info.root;

    auto &tree_st = case_info.tree_st;
    auto &tree_st_r = case_info.tree_st_r;
    auto &first_pos = case_info.first_pos;
    auto &lg = case_info.lg;
    auto &dep = case_info.dep;

    int N = input_graph.INs.size();

    global_dgraph_CT = input_graph;
    isIntree.resize(max_N_ID, 0);

    /* initialize queue */
    priority_queue<node_degree> q;
    for (int i = 0; i < N; i++)
    {
        node_degree nd;
        nd.degree = global_dgraph_CT.degree(i);
        nd.vertex = i;
        q.push(nd);
    }

    auto end1 = std::chrono::high_resolution_clock::now();
    case_info.time1_initialization = std::chrono::duration_cast<std::chrono::nanoseconds>(end1 - begin1).count() / 1e9;
    //---------------------------------------------------------------------------------

    //--------------------------- step 2: MDE-based tree decomposition ---------------------------
    // cout << "step 2: MDE-based tree decomposition" << endl;
    auto begin2 = std::chrono::high_resolution_clock::now();

    /* MDE */
    int bound_lambda = N;
    Bags_in.resize(N);
    Bags_out.resize(N);
    vector<int> node_order(N + 1); // merging ID to original ID

    ThreadPool pool(case_info.thread_num);
    std::vector<std::future<int>> results;

    for (int i = 1; i <= N; i++)
    {
        /* 1. find v_i */
        node_degree nd;
        while (1)
        {
            nd = q.top();
            q.pop();
            if (!isIntree[nd.vertex] && global_dgraph_CT.degree(nd.vertex) == nd.degree)
                break;
        }
        int v_i = nd.vertex;
        std::vector<std::pair<int, two_hop_weight_type>> vi_in = global_dgraph_CT.INs[v_i];
        std::vector<std::pair<int, two_hop_weight_type>> vi_out = global_dgraph_CT.OUTs[v_i];
        /* 2. Determine whether to break */
        if (nd.degree >= case_info.d)
        {
            bound_lambda = i - 1;
            q.push(nd);
            break;
        }
        /* 3. update CT info */
        isIntree[v_i] = 1;
        node_order[i] = v_i;
        /* 4. construct Bags_in/out & add edge */
        int in_size = vi_in.size();
        int out_size = vi_out.size();
        if (in_size != 0)
        {
            for (int u = 0; u < in_size; u++)
            {
                Bags_in[v_i].push_back({vi_in[u].first, vi_in[u].second});
                int _u = vi_in[u].first;
                /* u -> vi -> w */
                for (int w = 0; w < out_size; w++)
                {
                    int _w = vi_out[w].first;
                    if (_u == _w)
                        continue;
                    two_hop_weight_type new_ec = vi_in[u].second + vi_out[w].second;
                    results.emplace_back(
                        pool.enqueue([_u, _w, new_ec] { // pass const type value j to thread; [] can be empty
                            substitute_parallel(_u, _w, new_ec);
                            return 1;
                        }));
                }
                /* u -> vi, u2 -> vi */
                for (int u2 = u + 1; u2 < in_size; u2++)
                {
                    int _u2 = vi_in[u2].first;
                    two_hop_weight_type new_ec = std::numeric_limits<two_hop_weight_type>::max();
                    results.emplace_back(pool.enqueue([_u, _u2, new_ec] {
                        substitute_parallel_2(_u, _u2, new_ec);
                        return 1;
                    }));
                }
            }
        }
        if (out_size != 0)
        {
            for (int w = 0; w < out_size; w++)
            {
                Bags_out[v_i].push_back({vi_out[w].first, vi_out[w].second});
                int _w = vi_out[w].first;
                /* vi -> w, vi -> w2 */
                for (int w2 = w + 1; w2 < out_size; w2++)
                {
                    int _w2 = vi_out[w2].first;
                    two_hop_weight_type new_ec = std::numeric_limits<two_hop_weight_type>::max();
                    results.emplace_back(pool.enqueue([_w, _w2, new_ec] {
                        substitute_parallel_2(_w, _w2, new_ec);
                        return 1;
                    }));
                }
            }
        }
        for (auto &&result : results)
        {
            result.get();
        }
        results.clear();
        /* 5. delete edge and update degree */
        for (auto u = vi_in.begin(); u != vi_in.end(); u++)
        {
            int _u = u->first;
            results.emplace_back(pool.enqueue([_u, v_i] {
                remove_parallel(_u, v_i);
                return 1;
            }));
        }
        for (auto w = vi_out.begin(); w != vi_out.end(); w++)
        {
            int _w = w->first;
            results.emplace_back(pool.enqueue([v_i, _w] {
                remove_parallel(v_i, _w);
                return 1;
            }));
        }
        for (auto &&result : results)
        {
            result.get();
        }
        results.clear();
        /* 6. update priority queue */
        for (auto u = vi_in.begin(); u != vi_in.end(); u++)
        {
            nd.vertex = u->first;
            nd.degree = global_dgraph_CT.degree(nd.vertex);
            q.push(nd);
        }
        for (auto w = vi_out.begin(); w != vi_out.end(); w++)
        {
            nd.vertex = w->first;
            nd.degree = global_dgraph_CT.degree(nd.vertex);
            q.push(nd);
        }
    }

    auto end2 = std::chrono::high_resolution_clock::now();
    case_info.time2_tree_decomposition =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end2 - begin2).count() / 1e9;
    // --------------------------------------------------------------------------------------------------------
    // cout << "case_info.time2_tree_decomposition = " << case_info.time2_tree_decomposition << endl;

    //--------------------------- step 3: generate CT-tree indexs ---------------------------
    // cout << "step 3: generate CT-tree indexs" << endl;
    auto begin3 = std::chrono::high_resolution_clock::now();

    /* some variables */
    vector<vector<two_hop_label>> L1_in(N), L1_out(N);
    vector<int> order_mapping(N + 1);
    vector<int> fa(N);
    vector<two_hop_weight_type> temp_dis_in(N);
    vector<two_hop_weight_type> temp_dis_out(N);
    vector<int> index_node_in(N);
    vector<int> index_node_out(N);
    vector<int> islabel_in(N, 0);
    vector<int> islabel_out(N, 0);
    vector<vector<int>> son(N);

    int labelnum = 0;

    root.resize(N);
    dep.resize(N);
    first_pos.resize(N);

    /* order_mapping: origin to merging */
    for (int i = 1; i <= bound_lambda; i++)
        order_mapping[node_order[i]] = i;
    vector<bool> popped_isIncore_node(N, 0);
    /* sort the core node */
    for (int i = bound_lambda + 1; i <= N; i++)
    {
        struct node_degree nd;
        while (1)
        {
            nd = q.top();
            q.pop();
            if (!isIntree[nd.vertex] && !popped_isIncore_node[nd.vertex] &&
                global_dgraph_CT.degree(nd.vertex) == nd.degree)
                break;
        }
        node_order[i] = nd.vertex;
        popped_isIncore_node[nd.vertex] = 1;
        order_mapping[nd.vertex] = i;
    }

    /* tree node index */
    for (int i = bound_lambda; i >= 1; i--)
    {
        /* determine father */
        int v_i = node_order[i]; // merging ID to original ID
        fa[v_i] = INT_MAX;       // merging ID
        int Bags_in_size = Bags_in[v_i].size();
        int Bags_out_size = Bags_out[v_i].size();
        int Bags_size = Bags_in_size + Bags_out_size;
        for (auto it = Bags_in[v_i].begin(); it != Bags_in[v_i].end(); it++)
            if (order_mapping[it->first] < fa[v_i])
                fa[v_i] = order_mapping[it->first];
        for (auto it = Bags_out[v_i].begin(); it != Bags_out[v_i].end(); it++)
            if (order_mapping[it->first] < fa[v_i])
                fa[v_i] = order_mapping[it->first];
        /* interface nodes */
        if (fa[v_i] > bound_lambda || Bags_size == 0) // a root in the forest, interface
        {
            root[v_i] = v_i; // original ID
            fa[v_i] = -1;
            dep[v_i] = 0;
            /* interface nodes only need to add neighborn as hub */
            two_hop_label xx;
            for (auto it = Bags_in[v_i].begin(); it != Bags_in[v_i].end(); it++)
            {
                xx.vertex = it->first;
                xx.distance = it->second;
                L1_in[v_i].push_back(xx);
            }
            for (auto it = Bags_out[v_i].begin(); it != Bags_out[v_i].end(); it++)
            {
                xx.vertex = it->first;
                xx.distance = it->second;
                L1_out[v_i].push_back(xx);
            }
        }
        /* non-root tree nodes */
        else
        {
            fa[v_i] = node_order[fa[v_i]];
            root[v_i] = root[fa[v_i]];
            dep[v_i] = dep[fa[v_i]] + 1;
            son[fa[v_i]].push_back(v_i);

            int index_node_num_in = 0;
            int index_node_num_out = 0;
            labelnum++;

            int r = root[v_i];

            /* add interface's adj */
            /* here, add into both Lin an Lout is very important */
            for (auto it = Bags_in[r].begin(); it != Bags_in[r].end(); it++)
            {
                islabel_in[it->first] = labelnum;
                index_node_num_in++;
                index_node_in[index_node_num_in] = it->first;
                temp_dis_in[it->first] = std::numeric_limits<two_hop_weight_type>::max();
                islabel_out[it->first] = labelnum;
                index_node_num_out++;
                index_node_out[index_node_num_out] = it->first;
                temp_dis_out[it->first] = std::numeric_limits<two_hop_weight_type>::max();
            }
            for (auto it = Bags_out[r].begin(); it != Bags_out[r].end(); it++)
            {
                islabel_in[it->first] = labelnum;
                index_node_num_in++;
                index_node_in[index_node_num_in] = it->first;
                temp_dis_in[it->first] = std::numeric_limits<two_hop_weight_type>::max();
                islabel_out[it->first] = labelnum;
                index_node_num_out++;
                index_node_out[index_node_num_out] = it->first;
                temp_dis_out[it->first] = std::numeric_limits<two_hop_weight_type>::max();
            }

            /* add ancestor */
            int tmp = fa[v_i]; // original id
            while (1)
            {
                if (islabel_in[tmp] != labelnum)
                {
                    islabel_in[tmp] = labelnum;
                    index_node_num_in++;
                    index_node_in[index_node_num_in] = tmp;
                    temp_dis_in[tmp] = std::numeric_limits<two_hop_weight_type>::max();
                }

                if (islabel_out[tmp] != labelnum)
                {
                    islabel_out[tmp] = labelnum;
                    index_node_num_out++;
                    index_node_out[index_node_num_out] = tmp;
                    temp_dis_out[tmp] = std::numeric_limits<two_hop_weight_type>::max();
                }

                if (tmp == r)
                    break;

                tmp = fa[tmp]; // fa[] is original
            }

            /* delta(u) */
            for (auto it = Bags_in[v_i].begin(); it != Bags_in[v_i].end(); it++)
            {
                if (islabel_in[it->first] != labelnum)
                { // line 33
                    islabel_in[it->first] = labelnum;
                    index_node_num_in++;
                    index_node_in[index_node_num_in] = it->first;
                    temp_dis_in[it->first] = it->second;
                }
                else
                { // line ?
                    temp_dis_in[it->first] = it->second;
                }
            }
            /* delta(w) */
            for (auto it = Bags_out[v_i].begin(); it != Bags_out[v_i].end(); it++)
            {
                if (islabel_out[it->first] != labelnum)
                { // line 34
                    islabel_out[it->first] = labelnum;
                    index_node_num_out++;
                    index_node_out[index_node_num_out] = it->first;
                    temp_dis_out[it->first] = it->second;
                }
                else
                { // line ?
                    temp_dis_out[it->first] = it->second;
                }
            }

            /* min vj in N^an-in */
            for (int j = 0; j < Bags_in_size; j++)
            { // line 33,38
                int v_j = Bags_in[v_i][j].first;
                if (isIntree[v_j])
                {
                    two_hop_weight_type delta_vj = Bags_in[v_i][j].second;
                    int Lj_in_size = L1_in[v_j].size();

                    for (int k = 0; k < Lj_in_size; k++)
                    {
                        int u = L1_in[v_j][k].vertex;

                        two_hop_weight_type delta_uvj = L1_in[v_j][k].distance;
                        if (islabel_in[u] == labelnum && delta_vj + delta_uvj < temp_dis_in[u])
                        {
                            temp_dis_in[u] = delta_vj + delta_uvj;
                        }
                    }
                }
            }
            /* min vj in N^an-out */
            for (int j = 0; j < Bags_out_size; j++)
            { // line 34,39
                int v_j = Bags_out[v_i][j].first;
                if (isIntree[v_j])
                {
                    two_hop_weight_type delta_vj = Bags_out[v_i][j].second;
                    int Lj_out_size = L1_out[v_j].size();

                    for (int k = 0; k < Lj_out_size; k++)
                    {
                        int w = L1_out[v_j][k].vertex;
                        two_hop_weight_type delta_vjw = L1_out[v_j][k].distance;
                        if (islabel_out[w] == labelnum && delta_vj + delta_vjw < temp_dis_out[w])
                        {
                            temp_dis_out[w] = delta_vj + delta_vjw;
                        }
                    }
                }
            }

            /*add correct indexes of Lines 29-30 of CT into L1, and possibly wrong distances for Lines 31-32 into L1*/
            int delete_num = 0;
            L1_in[v_i].resize(index_node_num_in);
            for (int j = 1; j <= index_node_num_in; j++)
            {
                two_hop_label xx;
                xx.vertex = index_node_in[j];
                xx.distance = temp_dis_in[index_node_in[j]];
                if (xx.distance == std::numeric_limits<two_hop_weight_type>::max())
                {
                    delete_num++;
                    continue;
                }
                L1_in[v_i][j - 1 - delete_num] = xx;
            }
            L1_in[v_i].resize(index_node_num_in - delete_num);

            delete_num = 0;
            L1_out[v_i].resize(index_node_num_out);
            for (int j = 1; j <= index_node_num_out; j++)
            {
                two_hop_label xx;
                xx.vertex = index_node_out[j];
                xx.distance = temp_dis_out[index_node_out[j]];
                if (xx.distance == std::numeric_limits<two_hop_weight_type>::max())
                {
                    delete_num++;
                    continue;
                }
                L1_out[v_i][j - 1 - delete_num] = xx;
            }
            L1_out[v_i].resize(index_node_num_out - delete_num);
        }
    }

    /*add distance-0 labels to tree nodes; this is needed in querying functions*/
    two_hop_label xx;
    for (int i = 0; i < N; i++)
    {
        if (isIntree[i])
        {
            xx.vertex = i;
            xx.distance = 0;
            L1_in[i].push_back(xx);
            L1_out[i].push_back(xx);
        }
    }

    auto end3 = std::chrono::high_resolution_clock::now();
    case_info.time3_tree_indexs =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end3 - begin3).count() / 1e9; // s
    //-------------------------------------------------------------------------------------------------------

    //--------------------------- step 4: LCA ---------------------------
    // cout << "step 4: LCA" << endl;
    auto begin4 = std::chrono::high_resolution_clock::now();

    /* LCA code; already get the root, the father and the depth, here is the preprocessing of querying LCA */
    int total = 0;

    vector<int> dfn(2 * N + 5);
    for (int i = 1; i <= bound_lambda; i++)
    {
        int v_i = node_order[i];
        if (root[v_i] == v_i)
            dfs(total, first_pos, v_i, son, dfn);
    }

    if (total > 0)
    {
        int multi_step = ceil(log(total) / log(2)) + 2;

        tree_st.resize(total + 5);
        tree_st_r.resize(total + 5);
        for (int i = 1; i <= total; i++)
        {
            tree_st[i].resize(multi_step + 2);
            tree_st_r[i].resize(multi_step + 2);
        }

        vector<int> pow_2(multi_step);

        pow_2[0] = 1;
        for (int i = 1; i < multi_step; i++)
            pow_2[i] = pow_2[i - 1] << 1;

        for (int i = 1; i <= total; i++)
        {
            tree_st[i][0] = dfn[i];
            tree_st_r[i][0] = dfn[i];
        }

        for (int j = 1; j < multi_step; j++)
            for (int i = 1; i <= total; i++)
            {
                int k = i + pow_2[j - 1];
                if (k > total || dep[tree_st[i][j - 1]] <= dep[tree_st[k][j - 1]])
                    tree_st[i][j] = tree_st[i][j - 1];
                else
                    tree_st[i][j] = tree_st[k][j - 1];
                k = i - pow_2[j - 1];
                if (k <= 0 || dep[tree_st_r[i][j - 1]] <= dep[tree_st_r[k][j - 1]])
                    tree_st_r[i][j] = tree_st_r[i][j - 1];
                else
                    tree_st_r[i][j] = tree_st_r[k][j - 1];
            }
    }

    lg.resize(total + 1);
    for (int i = 1; i <= total; i++)
        lg[i] = floor(log(i) / log(2));

    /*clear variables not used below*/
    priority_queue<node_degree>().swap(q);
    vector<int>().swap(node_order);
    vector<int>().swap(fa);
    vector<two_hop_weight_type>().swap(temp_dis_in);
    vector<two_hop_weight_type>().swap(temp_dis_out);
    vector<int>().swap(order_mapping);
    vector<int>().swap(islabel_in);
    vector<int>().swap(islabel_out);
    vector<vector<int>>().swap(son);
    vector<int>().swap(index_node_in);
    vector<int>().swap(index_node_out);
    vector<int>().swap(dfn);

    auto end4 = std::chrono::high_resolution_clock::now();
    case_info.time4_lca = std::chrono::duration_cast<std::chrono::nanoseconds>(end4 - begin4).count() / 1e9; // s
    //---------------------------------------------------------------------------------

    //--------------------------- step 5: 2-hop labeling ---------------------------
    // cout << "step 5: 2-hop labeling" << endl;
    auto begin5 = std::chrono::high_resolution_clock::now();

    /*update limits*/

    /* construct 2-hop labels on core */
    case_info.core_graph = global_dgraph_CT;
    if (case_info.use_PLL)
    {
        dgraph_PLL(global_dgraph_CT, case_info.thread_num, case_info.two_hop_case_info);
    }
    else {
    	dgraph_PSL(global_dgraph_CT, case_info.thread_num, case_info.two_hop_case_info);
    }

    auto end5 = std::chrono::high_resolution_clock::now();
    case_info.time5_core_indexs = std::chrono::duration_cast<std::chrono::nanoseconds>(end5 - begin5).count() / 1e9;

    //--------------------------------------------------------------------------------------------------------------------

    //--------------------------- step 6: ---------------------------
    // cout << "step 6: postprocessing" << endl;
    auto begin6 = std::chrono::high_resolution_clock::now();

    /* merge tree_index: L1 into case_info.two_hop_case_info.L */
    for (int v_k = 0; v_k < N; v_k++)
    {
        if (L1_in[v_k].size() > 0)
        {
            vector<two_hop_label>(L1_in[v_k]).swap(L1_in[v_k]);
            case_info.two_hop_case_info.L_in[v_k] = L1_in[v_k];
            vector<two_hop_label>().swap(L1_in[v_k]);
        }
        if (L1_out[v_k].size() > 0)
        {
            vector<two_hop_label>(L1_out[v_k]).swap(L1_out[v_k]);
            case_info.two_hop_case_info.L_out[v_k] = L1_out[v_k];
            vector<two_hop_label>().swap(L1_out[v_k]);
        }
    }

    auto end6 = std::chrono::high_resolution_clock::now();
    case_info.time6_post = std::chrono::duration_cast<std::chrono::nanoseconds>(end6 - begin6).count() / 1e9;
    //---------------------------------------------------------------------------------------------------------------------------------

    clear_gloval_values_CT();

    case_info.time_total = std::chrono::duration_cast<std::chrono::nanoseconds>(end6 - begin1).count() / 1e9;
}

/*
query function
*/

int lca(dgraph_case_info_v2 &case_info, int x, int y)
{
    auto &first_pos = case_info.first_pos;

    if (first_pos[x] > first_pos[y])
    {
        int t = x;
        x = y;
        y = t;
    }

    int len = first_pos[y] - first_pos[x] + 1;
    int j = case_info.lg[len];
    x = case_info.tree_st[first_pos[x]][j];
    y = case_info.tree_st_r[first_pos[y]][j];
    if (case_info.dep[x] < case_info.dep[y])
        return x;
    else
        return y; // return the vertex with minimum depth between x and y in the dfs sequence.
}

two_hop_weight_type CT_extract_distance(dgraph_case_info_v2 &case_info, int source, int terminal)
{
    auto &L_in = case_info.two_hop_case_info.L_in;
    auto &L_out = case_info.two_hop_case_info.L_out;
    auto &Bags_in = case_info.Bags_in;
    auto &Bags_out = case_info.Bags_out;
    auto &isIntree = case_info.isIntree;
    auto &root = case_info.root;
    auto &core_graph = case_info.core_graph;

    if (source == terminal)
        return 0.0;

    two_hop_weight_type distance = std::numeric_limits<two_hop_weight_type>::max();

    if (!isIntree[source] && !isIntree[terminal])
    { /* both in core */
        // cout << "case 1" << endl;
        return dgraph_v1_extract_shortest_distance(L_in, L_out, source, terminal);
    }
    else if (!isIntree[source] && isIntree[terminal])
    { /* source in core, terminal in tree ; s -> root(t) -> t */
        // cout << "case 2.1" << endl;
        int r_size = L_in[terminal].size();
        for (int i = 0; i < r_size; i++)
        {
            if (L_in[terminal][i].distance > distance)
                continue;
            int u = L_in[terminal][i].vertex;
            two_hop_weight_type dist_ut = L_in[terminal][i].distance;
            two_hop_weight_type dist_su = dgraph_v1_extract_shortest_distance(L_in, L_out, source, u);
            if (distance > dist_su + dist_ut)
                distance = dist_su + dist_ut;
        }

        return distance;
    }
    else if (isIntree[source] && !isIntree[terminal])
    { /* source in tree, terminal in core ; s -> root(s) -> t */
        // cout << "case 2.2" << endl;

        int r_size = L_out[source].size();
        for (int i = 0; i < r_size; i++)
        {
            if (L_out[source][i].distance > distance)
                continue;
            int u = L_out[source][i].vertex;
            two_hop_weight_type dist_su = L_out[source][i].distance;
            two_hop_weight_type dist_ut = dgraph_v1_extract_shortest_distance(L_in, L_out, u, terminal);
            if (distance > dist_su + dist_ut)
                distance = dist_su + dist_ut;
        }

        return distance;
    }
    else if (root[source] != root[terminal])
    { /* in different tree */
        int r_s_size = L_out[source].size();
        int r_t_size = L_in[terminal].size();

        for (int i = 0; i < r_s_size; i++)
        {
            int u = L_out[source][i].vertex;
            double dist_su = L_out[source][i].distance;
            for (int j = 0; j < r_t_size; j++)
            {
                int w = L_in[terminal][j].vertex;
                double dist_wt = L_in[terminal][j].distance;
                double dist_uw = dgraph_v1_extract_shortest_distance(L_in, L_out, u, w);
                double dis = dist_su + dist_wt + dist_uw;
                if (dis < distance)
                    distance = dis;
            }
        }

        return distance;
    }
    else /* in same tree */
    {
        int grand = lca(case_info, source, terminal);

        /* d2 : s -> lca -> t */
        unordered_set<int> lca_in = {grand};
        for (int i = Bags_in[grand].size() - 1; i >= 0; i--)
            lca_in.insert(Bags_in[grand][i].first);

        unordered_map<int, two_hop_weight_type> source_dis;
        for (int i = L_out[source].size() - 1; i >= 0; i--)
        {
            int v = L_out[source][i].vertex;
            if (lca_in.count(v) > 0)
            {
                source_dis[v] = L_out[source][i].distance;
            }
        }
        for (int i = L_in[terminal].size() - 1; i >= 0; i--)
        {
            int v = L_in[terminal][i].vertex;
            if (source_dis.count(v) > 0)
            {
                two_hop_weight_type d2 = source_dis[v] + L_in[terminal][i].distance;
                if (distance > d2)
                {
                    distance = d2;
                }
            }
        }

        /* d4 : s -> root -> root -> t */
        int r_in_size = L_out[source].size();
        int r_out_size = L_in[terminal].size();
        for (int i = 0; i < r_in_size; i++)
        {
            int u = L_out[source][i].vertex;
            double dist_su = L_out[source][i].distance;
            for (int j = 0; j < r_out_size; j++)
            {
                int w = L_in[terminal][j].vertex;
                double dist_wt = L_in[terminal][j].distance;
                double dist_uw = dgraph_v1_extract_shortest_distance(L_in, L_out, u, w);
                double d4 = dist_su + dist_uw + dist_wt;
                if (distance > d4)
                {
                    distance = d4;
                }
            }
        }

        return distance;
    }
}