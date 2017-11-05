#ifndef CAVI_H
#define CAVI_H

#include <boost/random/mersenne_twister.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <utility>

#include "snp_data.h"
#include "vector_types.h"

// Coordinate Ascent Variational Inference
class Cavi
{
  public:
    Cavi(int                     npops,
         std::vector<double>     mixture_prior,
         double                  pop_size,        // fixed population size
         SNPData                 snp_data,
         boost::random::mt19937& gen,
         size_t                  nloci,
         vector2<int>            labels,
         bool                    using_labels = false);

    // Stochastic variational inference
    inline bool update_auxiliary_parameters(int locus);
    inline void update_allele_frequencies(int locus);
    inline void update_mixture_proportions(int locus);
    void run_stochastic();
    void initialize_variational_parameters();

    // Compute a lower bound on the posterior 
    // predictive distribution of the hold out 
    // set given the current variational parameters
    double compute_ho_log_likelihood();

    void   write_results(std::string out_file);


  private:
    std::vector<double>                 mixture_prior;  // prior parameter for dirichlet distribution
    const size_t                        npops;
    const size_t                        nloci;
    const size_t                        nsteps;         // number of time steps with samples   
    SNPData                             snp_data;       // snp data matrix
    boost::random::mt19937              gen;
    double                              pop_size;       // if specified, fixes population size rather than performing variational EM
    vector2<double>                     initial_freq;   // parameters specifying initial allele frequencies: initial_freq[k][l] is the
                                                        // initial frequency in population k at locus l
    vector4<double>                     freqs;          // freqs[t][k][l] gives mean and variance parameters for current variational estimates of
                                                        // the allele frequencies in population k at locus l
    vector3<double>                     pseudo_outputs; // the current pseudo outputs from the variational Kalman smoother
                                                        // these are indexed differently than other parameters! pseudo_outputs[k][l][t]
    vector3<double>                     theta;          // dirichlet variational parameters; theta[t][d][k] = kth parameter for the variational 
                                                        // dirichlet distribution corresponding to individual d sampled at time t
    vector3<double>                     phi;            // phi[t][d][l][k] = log of kth auxiliary parameter phi_{tdl,k} for individual d at locus l
    vector3<double>                     zeta;           // zeta[t][d][l][k] = log of kth auxiliary parameter zeta_{tdl,k} for individual d at locus l
    vector2<int>                        nloci_indv;
    vector2<int>                        labels;
    vector2<int>                        sample_iter;    // store the number of iterations for each sample. use for step size
    bool                                using_labels = false;
    
    inline void   update_auxiliary_local(size_t t, size_t d, size_t l);
    inline void   load_auxiliary_parameters(int l);
    void   write_temp();
    std::pair<bool, double>   check_theta_convergence(const vector3<double>& prev_theta);
};

#endif