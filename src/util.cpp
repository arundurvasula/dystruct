/*
Copyright (C) 2017-2018 Tyler Joseph <tjoseph@cs.columbia.edu>

This file is part of Dystruct.

Dystruct is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Dystruct is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Dystruct.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cmath>
#include <getopt.h>
#include <iostream>
#include <iterator>
#include <sstream>
#include <set>
#include <iomanip>
#include <map>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

using std::cerr;
using std::cout;
using std::count;
using std::exit;
using std::endl;
using std::find;
using std::ifstream;
using std::is_sorted;
using std::istringstream;
using std::insert_iterator;
using std::map;
using std::pair;
using std::set;
using std::setprecision;
using std::setw;
using std::skipws;
using std::sort;
using std::string;
using std::unique_copy;
using std::vector;

#include "snp_data.h"
#include "util.h"
#include "vector_types.h"


vector<int> read_generations(string fname, vector<int>& gen_sampled)
{
    ifstream input(fname);
    if (!input.is_open()) {
        cerr << "cannot open " << fname << endl;
        exit(1);
    }

    string line;
    vector<int> generations;
    int c;
    int g;
    istringstream iss(line);
    while (getline(input, line)) {
        iss = istringstream(line);
        c = 0;
        while (iss >> skipws >> g) {
            c++;
            generations.push_back(g);
            if (c > 1) {
                cerr << "Input Error (" << fname << "): more than one generation time per line" << endl;
                exit(1);
            }
        }
    }

    // remove duplicate sample times so we can aggregate samples by generation sampled
    vector<int> generations_sorted(generations);
    sort(generations_sorted.begin(), generations_sorted.end());
    insert_iterator<vector<int> > insert_it(gen_sampled, gen_sampled.begin());
    unique_copy(generations_sorted.begin(), generations_sorted.end(), insert_it);

    input.close();

    return generations;
}


int check_input_file(string fname, int nloci, int n_columns)
{   
    ifstream input(fname);
    if (!input.is_open()) {
        cerr << "cannot open " << fname << endl;
        exit(1);
    }

    string line;
    istringstream iss(line);
    int locus_count = 0;
    int col_count = 0;
    int g = 0;
    char c[] = "X";
    while (getline(input, line)) {
        locus_count++;
        iss = istringstream(line);
        col_count = 0;

        int nonmissing = 0;
        while (iss >> c[0]) {
            g = atoi(c);
            col_count++;

            if (!(g == 0 || g == 1 || g == 2 || g == 9)) {
                cerr << "Input Error (" << fname << "): line " << locus_count << " column "
                     << col_count + 1 << " has an invalid entry." << endl;
                cerr << "Genotypes must be 0, 1, or 2 if known, 9 if missing or unknown." << endl;
                exit(1);
            }

            if (g == 0 || g == 1 || g == 2) {
                nonmissing += 1;
            }
        }

        if (col_count != n_columns) {
            cerr << "Input Error (" << fname << "): line " << locus_count << " has "
                 << col_count << " samples, but generation file has " << n_columns << "." << endl;
            exit(1);
        }

        if (nonmissing == 0) {
            cerr << "Input Warning (" << fname << "): line " << locus_count 
                 << " has no nonmissing entries" << endl;
        }

        if (nonmissing == 1) {
            cerr << "Input Warning (" << fname << "): line " << locus_count 
                 << " only has 1 nonmissing entry" << endl;
        }
    }

    if (nloci != locus_count) {
        cerr << "Input Warning (" << fname << "): " << nloci << " were specified, but "
             << locus_count << " were found." << endl;
    }

    return locus_count;
}


map<int, pair<int, int> > read_snp_matrix(string fname, string gen_fname, std_vector3<short> *snps, vector<int>& gen_sampled, int& nloci)
{
    cout << "loading genotype matrix (this should not take more than a few minutes)..." << endl;
    cout << "\tchecking input file..." << endl;
    vector<int> generations = read_generations(gen_fname, gen_sampled);
    int found_loci = check_input_file(fname, nloci, generations.size());
    nloci = found_loci;

    cout << "\tfound " << generations.size() << " samples at " << gen_sampled.size() << " time points..." << endl;
    cout << "\tusing " << nloci << " loci..." << endl; 

    ifstream input(fname);
    if (!input.is_open()) {
        cerr << "cannot open " << fname << endl;
        exit(1);
    }

    string line;
    int ct;
    for (size_t t = 0; t < gen_sampled.size(); ++t) {
        ct = count(generations.begin(), generations.end(), gen_sampled[t]);
        (*snps).push_back(vector2<short>(boost::extents[ct][nloci]));
    }

    short genotype = 0;
    int i;
    int l = 0;
    int gen;
    int index;
    istringstream iss(line);
    char c[] = "X";
    while (getline(input, line)) {
        iss = istringstream(line);
        i = 0;
        map<int, int> gen_to_nsamples; // generation to number of samples
        while (iss >> c[0]) {
            genotype = atoi(c);
            gen = generations[i];
            index = find(gen_sampled.begin(), gen_sampled.end(), gen) - gen_sampled.begin();
            (*snps)[index][gen_to_nsamples[gen]][l] = genotype;
            gen_to_nsamples[gen]++;
            i++;
        }
        l++;
        if (l == nloci)
            break;
    }
    input.close();

    // need a map from original sample index to row index in genotype matrix
    map<int, pair<int, int> > sample_map;
    map<int, int> gen_to_nsamples;
    for (size_t i = 0; i < generations.size(); ++i) {
        gen = generations[i];
        index = find(gen_sampled.begin(), gen_sampled.end(), gen) - gen_sampled.begin();
        sample_map[(int)i] = pair<int, int>(index, gen_to_nsamples[gen]);
        gen_to_nsamples[gen]++;
    }
    return sample_map;
}


// This is no longer correct
vector2<int> read_pop_labels(string fname, SNPData& snp_data)
{
    vector2<int> labels(boost::extents[snp_data.total_time_steps()][snp_data.max_individuals()]);

    ifstream input(fname);
    if (!input.is_open()) {
        cerr << "cannot open label file " << fname << endl;
        exit(1);
    }

    string line;
    istringstream iss;
    int label;
    for (size_t t = 0; t < snp_data.total_time_steps(); ++t) {
        for (size_t d = 0; d < snp_data.total_individuals(t); ++d) {
            getline(input, line);
            iss = istringstream(line);
            iss >> skipws >> label;
            labels[t][d] = label;
        }
    }
    input.close();
    return labels;
}