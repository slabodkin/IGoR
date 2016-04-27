/*
 * Hypermutationglobalerrorrate.cpp
 *
 *  Created on: Mar 8, 2016
 *      Author: quentin
 */

#include "Hypermutationglobalerrorrate.h"

using namespace std;

Hypermutation_global_errorrate::Hypermutation_global_errorrate(size_t nmer_width , Gene_class learn , Gene_class apply , double starting_flat_value): Error_rate() , mutation_Nmer_size(nmer_width) , learn_on(learn) , apply_to(apply) , ei_nucleotide_contributions((new double [4*nmer_width])) , R(starting_flat_value) , n_v_real(0) , n_j_real(0) , n_d_real(0) {

	if(fmod(nmer_width,2)==0){
		throw runtime_error("Cannot instanciate hypermutation globale error rate with an even size Nmer(need to be symmetric)");
	}

	size_t array_size = pow(4,mutation_Nmer_size);

	Nmer_mutation_proba = new double [array_size];
	Nmer_P_SHM = new double [array_size];
	Nmer_P_BG = new double [array_size];

	//Instantiate flat nucleotide contributions
	for(int ii=0 ; ii!=4*mutation_Nmer_size ; ++ii){
		ei_nucleotide_contributions[ii] = 0;
	}
	for(int ii=0 ; ii!=array_size ; ++ii){
		Nmer_mutation_proba[ii] = 0;
		Nmer_P_SHM[ii] = 0;
		Nmer_P_BG[ii] = 0;
	}

	//Initialize addressing vector

	//Initialize booleans
	if(apply_to == V_gene | apply_to == VJ_genes | apply_to == VD_genes | apply_to == VDJ_genes){
		apply_to_v = true;
	}
	else apply_to_v = false;

	if(apply_to == D_gene | apply_to == DJ_genes | apply_to == VD_genes | apply_to == VDJ_genes){
		apply_to_d = true;
	}
	else apply_to_d = false;

	if(apply_to == J_gene | apply_to == VJ_genes | apply_to == DJ_genes | apply_to == VDJ_genes){
		apply_to_j = true;
	}
	else apply_to_j = false;

	if(learn_on == V_gene | learn_on == VJ_genes | learn_on == VD_genes | learn_on == VDJ_genes){
		learn_on_v = true;
	}
	else learn_on_v = false;

	if(learn_on == D_gene | learn_on == DJ_genes | learn_on == VD_genes | learn_on == VDJ_genes){
		learn_on_d = true;
	}
	else learn_on_d = false;

	if(learn_on == J_gene | learn_on == VJ_genes | learn_on == DJ_genes | learn_on == VDJ_genes){
		learn_on_j = true;
	}
	else learn_on_j = false;

	//Initialize adressing vector
	for(int ii = (mutation_Nmer_size-1) ; ii != -1 ; --ii){
		adressing_vector.push_back(pow(4,ii));
	}
}

Hypermutation_global_errorrate::Hypermutation_global_errorrate(size_t nmer_width , Gene_class learn , Gene_class apply , double starting_flat_value , vector<double> ei_contributions): Hypermutation_global_errorrate(nmer_width , learn , apply , starting_flat_value){
	if(ei_contributions.size()==(4*mutation_Nmer_size)){
		for(i=0 ; i != ei_contributions.size() ; ++i){
			ei_nucleotide_contributions[i] = ei_contributions[i];
		}
	}
	else{
		throw runtime_error("Size of ei contribution vector does not match the expected size in Hypermutation_global_errorrate(size_t,Gene_class,Gene_class,double,std::vector<double>)");
	}
}

Hypermutation_global_errorrate::~Hypermutation_global_errorrate() {
	// TODO Auto-generated destructor stub
	//Make a clean destructor and delete all the double* contained in maps
	delete [] ei_nucleotide_contributions;
	delete [] Nmer_mutation_proba;
	delete [] Nmer_P_SHM;
	delete [] Nmer_P_BG;

	//Clean
	if(learn_on_v){
		for(i = 0 ; i != n_v_real ; ++i){
			delete [] v_gene_nucleotide_coverage_p[i].second;
			delete [] v_gene_nucleotide_coverage_seq_p[i].second;
			delete [] v_gene_per_nucleotide_error_p[i].second;
			delete [] v_gene_per_nucleotide_error_seq_p[i].second;
		}
		//delete [] v_gene_nucleotide_coverage_p; //FIXME find out why free() exception
		//delete [] v_gene_nucleotide_coverage_seq_p;
		//delete [] v_gene_per_nucleotide_error_p;
		//delete [] v_gene_per_nucleotide_error_seq_p;
	}

	if(learn_on_d){

	}

	if(learn_on_j){

	}



}

Error_rate* Hypermutation_global_errorrate::copy()const{

	Hypermutation_global_errorrate* copy_err_r = new Hypermutation_global_errorrate(this->mutation_Nmer_size , this->learn_on , this->apply_to , this->R);
	copy_err_r->updated = this->updated;
	//copy_err_r->R = this->R;
	for(int ii = 0 ; ii != mutation_Nmer_size*4 ; ++ii){
		copy_err_r->ei_nucleotide_contributions[ii] = this->ei_nucleotide_contributions[ii];
	}
	//Make sure the values for the Nmer probas are correct
	copy_err_r->update_Nmers_proba(0,0,1);

	return copy_err_r;

}

Hypermutation_global_errorrate& Hypermutation_global_errorrate::operator +=(Hypermutation_global_errorrate err_r){

	//FIXME sequential ifs throwing more meaningful exception
	if( (this->learn_on == err_r.learn_on)
		& (this->apply_to == err_r.apply_to)
		& (this->mutation_Nmer_size == err_r.mutation_Nmer_size)
		& (this->R == err_r.R)){
		this->number_seq+=err_r.number_seq;
		this->model_log_likelihood+=err_r.model_log_likelihood;

		//Copy V gene error and coverage
		for( i=0 ; i!= n_v_real ; ++i){
			//Get the length of the gene and a pointer to the right array to write on
			tmp_corr_len = v_gene_nucleotide_coverage_p[i].first;
			tmp_cov_p = v_gene_nucleotide_coverage_p[i].second;
			double * tmp_cov_p_other = err_r.v_gene_nucleotide_coverage_p[i].second;
			tmp_err_p = v_gene_per_nucleotide_error_p[i].second;
			double * tmp_err_p_other = err_r.v_gene_per_nucleotide_error_p[i].second;
			for(j=0 ; j!= tmp_corr_len ; ++j){
				tmp_cov_p[j]+=tmp_cov_p_other[j];
				tmp_err_p[j]+=tmp_err_p_other[j];
			}
		}

		//Copy J gene error and coverage

		//Copy D gene error and coverage

		return *this;
	}
	else{
		throw runtime_error("Hypermutation models cannot be added in Hypermutation_global_errorrate::operator +=()");
	}

}

Error_rate* Hypermutation_global_errorrate::add_checked(Error_rate* err_r){
	return &(this->operator +=( *(dynamic_cast<Hypermutation_global_errorrate*>(err_r) ) ));
}

double Hypermutation_global_errorrate::get_err_rate_upper_bound() const{
	double max_proba = 0;
	for(i=0 ; i!=pow(4,mutation_Nmer_size);i++){
		if(Nmer_mutation_proba[i]>max_proba){
			max_proba = Nmer_mutation_proba[i];
		}
	}
	cout<<"max_proba: "<<max_proba<<endl;
	return max_proba/3;
}

double Hypermutation_global_errorrate::compare_sequences_error_prob (double scenario_probability , const string& original_sequence ,  Seq_type_str_p_map& constructed_sequences , const Seq_offsets_map& seq_offsets , const unordered_map<tuple<Event_type,Gene_class,Seq_side>, Rec_Event*>& events_map , Mismatch_vectors_map& mismatches_lists , double& seq_max_prob_scenario , double& proba_threshold_factor){
	//TODO Take into account the order of mutations

	scenario_resulting_sequence.clear();
	if(v_gene){
		scenario_resulting_sequence += (*constructed_sequences[V_gene_seq]);
	}
	if(d_gene){
		if(vd_ins){
			scenario_resulting_sequence+=(*constructed_sequences[VD_ins_seq]);
		}
		scenario_resulting_sequence+=(*constructed_sequences[D_gene_seq]);
		if(dj_ins){
			scenario_resulting_sequence+=(*constructed_sequences[DJ_ins_seq]);
		}
	}
	else{
		if(vj_ins){
			scenario_resulting_sequence+=(*constructed_sequences[VJ_ins_seq]);
		}
	}
	if(j_gene){
		scenario_resulting_sequence+=(*constructed_sequences[J_gene_seq]);
	}
	/*string& v_gene_seq = (*constructed_sequences[V_gene_seq]);
	string& d_gene_seq = (*constructed_sequences[D_gene_seq]);
	string& j_gene_seq = (*constructed_sequences[J_gene_seq]);
	string& vd_ins_seq = (*constructed_sequences[VD_ins_seq]);
	string& vj_ins_seq = (*constructed_sequences[VJ_ins_seq]);
	string& dj_ins_seq = (*constructed_sequences[DJ_ins_seq]);

	scenario_resulting_sequence = v_gene_seq + vd_ins_seq + d_gene_seq + dj_ins_seq + vj_ins_seq + j_gene_seq; //Will this work?*/


	vector<int>& v_mismatch_list = *mismatches_lists[V_gene_seq];
	if(mismatches_lists.exist(D_gene_seq)){ //Remove check? ensured by initialization
		vector<int>& d_mismatch_list = *mismatches_lists[D_gene_seq];
	}

	vector<int>& j_mismatch_list = *mismatches_lists[J_gene_seq];

	scenario_new_proba = scenario_probability;

	//First compute the contribution of the errors to the sequence likelihood

	//Check that the sequence is at least the Nmer size
	tmp_len_util = scenario_resulting_sequence.size();
	if(tmp_len_util>=mutation_Nmer_size){
		current_mismatch = v_mismatch_list.begin();

		//TODO Need to get the previous V nucleotides and last J ones

		//Get the adress of the first Nmer(disregarding the error penalty on teh first nucleotides)
		Nmer_index = 0;
		current_Nmer = queue<size_t>();
		for(i=0 ; i!=mutation_Nmer_size ; ++i){
			tmp_int_nt = stoi(scenario_resulting_sequence.substr(i,1));
			current_Nmer.push(tmp_int_nt);
			Nmer_index+=adressing_vector[i]*tmp_int_nt;
		}
		//FIXME maybe should iterate the other way around, what happens for errors/context of first nucleotides?

		//Check if there's an error and apply the cost accordingly
		if((*current_mismatch)==(mutation_Nmer_size+1)/2){
			scenario_new_proba*=Nmer_mutation_proba[Nmer_index];
			current_mismatch++;
		}
		else{
			scenario_new_proba*=(1-Nmer_mutation_proba[Nmer_index]);
		}


		//Look at all Nmers in the scenario_resulting_sequence by sliding window
		//Removing the contribution of the first and adding the contribution of the new last

		for( i = (mutation_Nmer_size+1)/2 ; i!=scenario_resulting_sequence.size()-(mutation_Nmer_size-1)/2 ; ++i){
			//Remove the previous first nucleotide of the Nmer and it's contribution to the index
			Nmer_index-=current_Nmer.front()*adressing_vector[0];
			current_Nmer.pop();
			//Shift the index
			Nmer_index*=4;
			//Add the contribution of the new nucleotide
			tmp_int_nt = stoi(scenario_resulting_sequence.substr(i+(mutation_Nmer_size-1)/2,1));//Assume a symetric Nmer
			Nmer_index+=tmp_int_nt;
			current_Nmer.push(tmp_int_nt);

			//Apply the error cost
			if( (*current_mismatch)==(mutation_Nmer_size+1)/2){
				scenario_new_proba*=Nmer_mutation_proba[Nmer_index];
				current_mismatch++;
			}
			else{
				scenario_new_proba*=(1-Nmer_mutation_proba[Nmer_index]);
			}

		}

	}


/*	In order to be self consistent the error rate should be applied everywhere
 * //V gene
	if(apply_to_v){


	}


	//D gene
	if(apply_to_d){

	}


	//J gene
	if(apply_to_j){

	}*/

	//Record genomic nucleotides coverage and errors

	if(learn_on_v){

		//Get the coverage
		//Get the length of the gene and a pointer to the right array to write on
		cout<<"vgene real index: "<<**vgene_real_index_p<<endl;
		tmp_corr_len = v_gene_nucleotide_coverage_seq_p[**vgene_real_index_p].first;
		tmp_cov_p = v_gene_nucleotide_coverage_seq_p[**vgene_real_index_p].second;
		tmp_err_p = v_gene_per_nucleotide_error_seq_p[**vgene_real_index_p].second;

		//Get the corrected number of deletions(no negative deletion)
		tmp_corr_len -= max(0,*v_3_del_value_p);

		// Compute the coverage
		for( i = max(0,-(**vgene_offset_p)) ; i != tmp_corr_len ; ++i ){
			tmp_cov_p[i]+=scenario_new_proba;
		}

		//Compute the error per nucleotide on the gene
		tmp_len_util = v_mismatch_list.size();
		for( i = 0 ; i != tmp_len_util ; ++i){
			tmp_err_p[v_mismatch_list[i]-(**vgene_offset_p)]+=scenario_new_proba;
		}

	}

	if(learn_on_d){

	}

	if(learn_on_j){

	}

	this->seq_likelihood += scenario_new_proba;
	this->seq_probability+=scenario_probability;
	++debug_number_scenarios;

	return scenario_new_proba;

}

queue<int> Hypermutation_global_errorrate::generate_errors(string& generated_seq , default_random_engine& generator) const{
	uniform_real_distribution<double> distribution(0.0,1.0);
	double rand_err ;// distribution(generator);
	queue<int> errors_indices;

	double error_proba;

	string int_generated_seq = nt2int(generated_seq);

	//FIXME take into account hidden nucleotides on the right and left sides

	//Get the adress of the first Nmer(disregarding the error penalty on the first nucleotides)
	Nmer_index = 0;
	current_Nmer = queue<size_t>();
	for(i=0 ; i!=mutation_Nmer_size ; ++i){
		tmp_int_nt = stoi(int_generated_seq.substr(i,1));
		current_Nmer.push(tmp_int_nt);
		Nmer_index+=adressing_vector[i]*tmp_int_nt;
	}

	error_proba = Nmer_mutation_proba[Nmer_index];
	rand_err = distribution(generator);

	if(rand_err<error_proba){
		//Introduce an error
		errors_indices.push((mutation_Nmer_size-1)/2);

		introduce_uniform_transversion(generated_seq[(mutation_Nmer_size-1)/2], generator , distribution);
	}

	for( i = (mutation_Nmer_size+1)/2 ; i!=int_generated_seq.size()-(mutation_Nmer_size-1)/2 ; ++i){
		//Remove the previous first nucleotide of the Nmer and it's contribution to the index
		Nmer_index-=current_Nmer.front()*adressing_vector[0];
		current_Nmer.pop();
		//Shift the index
		Nmer_index*=4;
		//Add the contribution of the new nucleotide
		tmp_int_nt = stoi(int_generated_seq.substr(i+(mutation_Nmer_size-1)/2,1));//Assume a symmetrically sized Nmer
		Nmer_index+=tmp_int_nt;
		current_Nmer.push(tmp_int_nt);


		error_proba = Nmer_mutation_proba[Nmer_index];
		rand_err = distribution(generator);

		if(rand_err<error_proba){
			//Introduce an error
			errors_indices.push(i);

			introduce_uniform_transversion(generated_seq[i], generator , distribution);
		}
	}
	return errors_indices;
}

unsigned Hypermutation_global_errorrate::generate_random_contributions(double ei_contribution_range){
	//Create seed for random generator
	//create a seed from timer
	typedef std::chrono::high_resolution_clock myclock;
	myclock::time_point time = myclock::now();
	myclock::duration dur = myclock::time_point::max() - time;

	unsigned time_seed = dur.count();
	//Instantiate random number generator
	default_random_engine generator =  default_random_engine(time_seed);
	uniform_real_distribution<double> distribution(-ei_contribution_range,ei_contribution_range);

	for(i = 0 ; i != mutation_Nmer_size ; ++i){
		double contribution_sum = 0;
		double rand_contribution;
		for(j = 0 ; j != 4 ; ++j){
			rand_contribution = distribution(generator);
			ei_nucleotide_contributions[i*4+j] = rand_contribution;
			contribution_sum += rand_contribution;
		}
		//Ensure that the sum of the contributions at one position is 0
		for(j=0 ; j != 4 ; ++j){
			cout<<ei_nucleotide_contributions[i*4+j]<<";";
			ei_nucleotide_contributions[i*4+j] -= contribution_sum/4;
			cout<<ei_nucleotide_contributions[i*4+j]<<endl;
		}
	}
	this->update_Nmers_proba(0,0,1);

	return time_seed;
}


void Hypermutation_global_errorrate::update(){

	//Compute P_SHM and P_bg for each possible Nmer
	this->compute_P_SHM_and_BG();

	//Update the error rate by maximizing the likelihood of the error model

	/*
	 * Find the maximum likelihood parameters by using newton's method on the derivative
	 * The constraint on the sum of e_j at one position being zero is absorbed by setting
	 * e_j(\pi_k) = -sum(e_j(\pi_i)) for i!=k
	 * At each step we solve H\deltaX=-J (where H is the hessian and J the jacobian of the likelihood function)
	 */

	double j_norm = INFINITY;

	while(j_norm!=0){

		double error_model_likelihood = 0;

		//Construct the 3N+1 sized Jacobian vector
		double J_data[3*mutation_Nmer_size+1];
		for(i=0 ; i!= 3*mutation_Nmer_size+1 ; ++i){
			J_data[i] = 0;
		}
		gsl_vector_view J = gsl_vector_view_array (J_data, 3*mutation_Nmer_size+1);

		//Construct the 3N+1 square Hessian matrix
		double H_data[(3*mutation_Nmer_size+1)*(3*mutation_Nmer_size+1)]; //Are gsl matrices row or column first?
		for(i=0 ; i!= (3*mutation_Nmer_size+1)*(3*mutation_Nmer_size+1) ; ++i){
			H_data[i] = 0;
		}
		gsl_matrix_view H = gsl_matrix_view_array (H_data, 3*mutation_Nmer_size+1,3*mutation_Nmer_size+1);

		for(int yyy = 0 ; yyy !=(3*mutation_Nmer_size)+1 ; ++yyy){
			for(int zzz = 0 ; zzz !=(3*mutation_Nmer_size)+1 ; ++zzz){
				cout<<gsl_matrix_get(&H.matrix , zzz , yyy)<<";";
			}
			cout<<endl;
		}
		cout<<endl;

		//Compute the values for the Jacobian and Hessian entries
		int base_4_address[mutation_Nmer_size];
		int max_address = 0;
		for (i=0;i!=mutation_Nmer_size;++i){
			base_4_address[i]=0;
			max_address += 3*adressing_vector[i];
		}
		j=0;
		while(j!=max_address){
			double current_Nmer_P_SHM = Nmer_P_SHM[j];
			double current_Nmer_P_bg = Nmer_P_BG[j];
			double current_Nmer_unorm_score = compute_Nmer_unorm_score(base_4_address);
			for(i=0;i!=mutation_Nmer_size;++i){
				if(base_4_address[i]==3){
					//Add contribution to the 3 constraining nucleotides
					for(size_t jj=0 ; jj!=3 ; ++jj){

						//Add contribution to dQ/dei
						J_data[i*3 + jj] -= current_Nmer_P_SHM - current_Nmer_P_bg*(R*current_Nmer_unorm_score)/(1+R*current_Nmer_unorm_score) ;

						//Add contribution to d²Q/dei²
						*(gsl_matrix_ptr( &H.matrix, i*3 + jj,i*3 + jj)) += -current_Nmer_P_bg *(R*current_Nmer_unorm_score)/pow(1+R*current_Nmer_unorm_score,2);

						//Add contributions to the two other constrained nucleotides
						for(size_t jjj=(jj+1) ; jjj != 3 ; ++jjj){
							*(gsl_matrix_ptr( &H.matrix, i*3 + jj,i*3 + jjj)) += -current_Nmer_P_bg *(R*current_Nmer_unorm_score)/pow(1+R*current_Nmer_unorm_score,2);
						}

						//Add contribution to d²Q/dejdei
						for(size_t ii=(i+1) ; ii!=mutation_Nmer_size ; ++ii){
							//Since the Hessian is symmetric no need to go over everything twice(only fill the lower triangular matrix)
							if(base_4_address[ii]==3){
								for(size_t jjj=0 ; jjj!=3 ; ++jjj){
									*(gsl_matrix_ptr(&H.matrix,i*3 + jj ,ii*3 + jjj)) += -current_Nmer_P_bg *(R*current_Nmer_unorm_score)/pow(1+R*current_Nmer_unorm_score,2);
								}
							}
							else{
								*(gsl_matrix_ptr(&H.matrix , i*3 + jj  , ii*3 + base_4_address[ii])) -= -current_Nmer_P_bg *(R*current_Nmer_unorm_score)/pow(1+R*current_Nmer_unorm_score,2);
							}
						}

						//Add contribution to d²Q/dRdei
						*(gsl_matrix_ptr( &H.matrix , i*3 + jj , 3*mutation_Nmer_size)) -= -current_Nmer_P_bg *(current_Nmer_unorm_score)/pow(1+R*current_Nmer_unorm_score,2);

					}
				}
				else{
					//Add contribution to dQ/dei
					J_data[i*3 + base_4_address[i]] += current_Nmer_P_SHM - current_Nmer_P_bg*(R*current_Nmer_unorm_score)/(1+R*current_Nmer_unorm_score) ;

					//Add contribution to d²Q/dei²
					*(gsl_matrix_ptr( &H.matrix ,i*3 + base_4_address[i] , i*3 + base_4_address[i])) += -current_Nmer_P_bg *(R*current_Nmer_unorm_score)/pow(1+R*current_Nmer_unorm_score,2);


					//Add contribution to d²Q/dejdei
					for(size_t ii=(i+1) ; ii!=mutation_Nmer_size ; ++ii){
						//Since the Hessian is symmetric no need to go over everything twice(only fill the lower triangular matrix)
						if(base_4_address[ii]==3){
							for(size_t jj=0 ; jj!=3 ; ++jj){
								*(gsl_matrix_ptr( &H.matrix , i*3 + base_4_address[i],ii*3 + jj )) -= -current_Nmer_P_bg *(R*current_Nmer_unorm_score)/pow(1+R*current_Nmer_unorm_score,2);
							}
						}
						else{
							*(gsl_matrix_ptr( &H.matrix , i*3 + base_4_address[i],ii*3 + base_4_address[ii] )) += -current_Nmer_P_bg *(R*current_Nmer_unorm_score)/pow(1+R*current_Nmer_unorm_score,2);
						}

						//if same site(ii==i) the only remaining term is the one from the constrained contribution(4th nucleotide), taken care of in the upper part
					}
					//Add contribution to d²Q/dRdei
					*(gsl_matrix_ptr( &H.matrix , i*3 + base_4_address[i] , 3*mutation_Nmer_size)) += -current_Nmer_P_bg *(current_Nmer_unorm_score)/pow(1+R*current_Nmer_unorm_score,2);
				}



			}
			//Add contribution to dQ/dR and d²Q/dR²
			J_data[(3*mutation_Nmer_size)] += current_Nmer_P_SHM/R -(current_Nmer_P_bg*current_Nmer_unorm_score/(1+R*current_Nmer_unorm_score));
			H_data[(3*mutation_Nmer_size+1)*(3*mutation_Nmer_size+1)-1] += current_Nmer_P_bg*pow(current_Nmer_unorm_score,2)/(1+R*current_Nmer_unorm_score) -current_Nmer_P_SHM/pow(R,2);

			//Copy the symmetric part of the Hessian matrix
			for(int ii = 0 ; ii!=3*mutation_Nmer_size+1 ; ++ii){
				for(int jj = ii+1 ; jj!=3*mutation_Nmer_size+1 ; ++jj){
					*(gsl_matrix_ptr( &H.matrix , jj , ii)) = *(gsl_matrix_ptr( &H.matrix , ii , jj));
				}
			}

			//Update the base 10 and 4 addresses
			++j;//base 10
			bool bool_mod_4N = fmod(j,4)==0;
			if(bool_mod_4N){
				int position = mutation_Nmer_size-1;
				while(bool_mod_4N){
					base_4_address[position]=0;
					--position;
					base_4_address[position]+=1;
					bool_mod_4N = fmod(j,adressing_vector[position-1])==0;
					//Careful to out of range exception although should not happen

				}
			}
			else{
				base_4_address[mutation_Nmer_size-1]+=1;
			}
			error_model_likelihood+=current_Nmer_P_SHM*(log(R)+log(current_Nmer_unorm_score) - log(3)) - current_Nmer_P_bg*log(1+R*current_Nmer_unorm_score);
		}
		cout<<"current hypermutation model likelihood: "<<error_model_likelihood<<endl;

		j_norm = 0;
		for(int kk=0 ; kk != (3*mutation_Nmer_size+1) ; ++kk){
			j_norm += pow(J_data[kk],2);
		}
		j_norm = sqrt(j_norm);
		if(j_norm==0){
			cout<<"Newton's method converged"<<endl;
			break;
		}
		else if(std::isnan(j_norm)){
			throw runtime_error("Optimization of the hypermutation model failed");
		}
		else{
			cout<<"jacobian norm: "<<j_norm<<endl;
		}

		//Set J to -J and then solve H\deltax = J
		for(i=0 ; i!= 3*mutation_Nmer_size+1 ; ++i){
			J_data[i] = -J_data[i];
		}

		cout<<"Jacobian"<<endl;
		for(int zzz = 0 ; zzz !=(3*mutation_Nmer_size)+1 ; ++zzz){
			cout<<gsl_vector_get(&J.vector , zzz )<<";";
		}
		cout<<endl;

		cout<<"Hessian"<<endl;
		for(int yyy = 0 ; yyy !=(3*mutation_Nmer_size)+1 ; ++yyy){
			for(int zzz = 0 ; zzz !=(3*mutation_Nmer_size)+1 ; ++zzz){
				cout<<gsl_matrix_get(&H.matrix , zzz , yyy)<<";";
			}
			cout<<endl;
		}
		cout<<endl;

		//Solve the system
		gsl_vector *x = gsl_vector_alloc (3*mutation_Nmer_size+1);
		gsl_permutation * p = gsl_permutation_alloc (3*mutation_Nmer_size+1);
		int signum;
		gsl_linalg_LU_decomp(&H.matrix, p , &signum);
		gsl_linalg_LU_solve (&H.matrix, p, &J.vector, x);

		//solve the system using a pseudo inverse
/*		gsl_matrix *V = gsl_matrix_alloc(3*mutation_Nmer_size+1,3*mutation_Nmer_size+1);
		gsl_vector *S = gsl_vector_alloc(3*mutation_Nmer_size+1);
		gsl_vector *work_vect = gsl_vector_alloc(3*mutation_Nmer_size+1);
		gsl_linalg_SV_decomp(&H.matrix , V , S , work_vect);

		gsl_linalg_SV_solve (&H.matrix, V , S , &J.vector, x );*/

		cout<<"\\deltaX vector"<<endl;
		for(int ii= 0 ; ii != mutation_Nmer_size*3+1 ; ++ii){
			cout<<gsl_vector_get(x,ii)<<";";
		}
		cout<<endl;

		//Update the parameters values
		for(i=0 ; i != mutation_Nmer_size ; ++i){
			//ei_nucleotide_contributions[i*mutation_Nmer_size+3] = 0;
			for(j=0 ; j!=3 ; ++j){
				//Update the contribution of the nucleotide
				ei_nucleotide_contributions[i*4+j] += gsl_vector_get(x,(i*3 + j));

				//Compute the contribution of the constrained nucleotide
				ei_nucleotide_contributions[i*4+3] -= gsl_vector_get(x,(i*3 + j));
			}
		}
		//Update the normalization factor
		R += gsl_vector_get(x,(3*mutation_Nmer_size));

		cout<<"new model parms"<<endl;
		cout<<R<<endl;
		for(int zzz=0 ; zzz!=4*mutation_Nmer_size ; ++zzz){
			cout<<ei_nucleotide_contributions[zzz]<<";";
		}
		cout<<endl;
		cout<<endl;

	}

	//Compute the new mutation probabilities for the full Nmers
	this->update_Nmers_proba(0,0,1);

	//Clean coverage counters
	if(learn_on_v){
		for(i = 0 ; i!=n_v_real ; ++i){
			//Get the length of the gene and a pointer to the right array to write on
			tmp_corr_len = v_gene_nucleotide_coverage_p[i].first;
			tmp_cov_p = v_gene_nucleotide_coverage_p[i].second;
			tmp_err_p = v_gene_per_nucleotide_error_p[i].second;

			for(j = 0 ; j!= tmp_corr_len ; j++){
				//reset coverage
				tmp_cov_p[j] = 0;

				//Same for errors
				tmp_err_p[j] = 0;
			}
		}
	}

	if(learn_on_d){

	}

	if(learn_on_j){

	}


}

void Hypermutation_global_errorrate::initialize(const unordered_map<tuple<Event_type,Gene_class,Seq_side>, Rec_Event*>& events_map){
	//FIXME look for previous initialization to avoid memory leak

	//Get the right pointers for the V gene
	if(learn_on == V_gene | learn_on == VJ_genes | learn_on == VD_genes | learn_on == VDJ_genes){
		try{
			Gene_choice* v_gene_event_p = dynamic_cast<Gene_choice*>(events_map.at(tuple<Event_type,Gene_class,Seq_side>(GeneChoice_t,V_gene,Undefined_side)));
			vgene_offset_p = &v_gene_event_p->alignment_offset_p;
			vgene_real_index_p = &v_gene_event_p->current_realization_index;

			//Initialize gene counters
			v_realizations = v_gene_event_p->get_realizations_map();
			//Get the number of realizations
			n_v_real = v_realizations.size();
			//Create arrays
			v_gene_nucleotide_coverage_p = new pair<size_t,double*>[n_v_real];
			v_gene_per_nucleotide_error_p = new pair<size_t,double*>[n_v_real];
			v_gene_nucleotide_coverage_seq_p = new pair<size_t,double*>[n_v_real];
			v_gene_per_nucleotide_error_seq_p = new pair<size_t,double*>[n_v_real];

			for(unordered_map<string , Event_realization>::const_iterator iter = v_realizations.begin() ; iter != v_realizations.end() ; iter++){

				//Initialize normalized counters
				v_gene_nucleotide_coverage_p[(*iter).second.index] = pair<size_t,double*>((*iter).second.value_str_int.size(),new double [(*iter).second.value_str_int.size()]);
				v_gene_per_nucleotide_error_p[(*iter).second.index] = pair<size_t,double*>((*iter).second.value_str_int.size(),new double [(*iter).second.value_str_int.size()]);

				//Initialize sequence counters
				v_gene_nucleotide_coverage_seq_p[(*iter).second.index] = pair<size_t,double*>((*iter).second.value_str_int.size(),new double [(*iter).second.value_str_int.size()]);
				v_gene_per_nucleotide_error_seq_p[(*iter).second.index] = pair<size_t,double*>((*iter).second.value_str_int.size(),new double [(*iter).second.value_str_int.size()]);
			}

		}
		catch(exception& except){
			cout<<"Exception caught during initialization of Hypermutation global error rate"<<endl;
			cout<<"Exception caught trying to initialize V gene pointers"<<endl;
			cout<<endl<<"throwing exception now..."<<endl;
			throw except;
		}

		//Get deletion value pointer for V 3' deletions if it exists
		if(events_map.count(tuple<Event_type,Gene_class,Seq_side>(Deletion_t,V_gene,Three_prime)) != 0){
			const Deletion* v_3_del_event_p = dynamic_cast<Deletion*>(events_map.at(tuple<Event_type,Gene_class,Seq_side>(Deletion_t,V_gene,Three_prime)));
			v_3_del_value_p = &(v_3_del_event_p->deletion_value);
		}
		else{v_3_del_value_p = &no_del_buffer;}

	}

	//Get the right pointers for the D gene
	if(learn_on == D_gene | learn_on == DJ_genes | learn_on == VD_genes | learn_on == VDJ_genes){
		try{
			Gene_choice* d_gene_event_p = dynamic_cast<Gene_choice*>(events_map.at(tuple<Event_type,Gene_class,Seq_side>(GeneChoice_t,D_gene,Undefined_side)));
			dgene_offset_p = &d_gene_event_p->alignment_offset_p;
			dgene_real_index_p = &d_gene_event_p->current_realization_index;
			//Initialize gene counters

		}
		catch(exception& except){
			cout<<"Exception caught during initialization of Hypermutation global error rate"<<endl;
			cout<<"Exception caught trying to initialize D gene pointers"<<endl;
			cout<<endl<<"throwing exception now..."<<endl;
			throw except;
		}

		//Get deletion value pointer for D 5' deletions if it exists
		if(events_map.count(tuple<Event_type,Gene_class,Seq_side>(Deletion_t,D_gene,Five_prime)) != 0){
			const Deletion* d_5_del_event_p = dynamic_cast<Deletion*>(events_map.at(tuple<Event_type,Gene_class,Seq_side>(Deletion_t,D_gene,Five_prime)));
			d_5_del_value_p = &(d_5_del_event_p->deletion_value);
		}
		else{d_5_del_value_p = &no_del_buffer;}

		//Get deletion value pointer for D 3' deletions if it exists
		if(events_map.count(tuple<Event_type,Gene_class,Seq_side>(Deletion_t,D_gene,Three_prime)) != 0){
			const Deletion* d_3_del_event_p = dynamic_cast<Deletion*>(events_map.at(tuple<Event_type,Gene_class,Seq_side>(Deletion_t,D_gene,Three_prime)));
			d_3_del_value_p = &(d_3_del_event_p->deletion_value);
		}
		else{d_3_del_value_p = &no_del_buffer;}

	}

	//Get the right pointers for the J gene
	if(learn_on == J_gene | learn_on == DJ_genes | learn_on == VJ_genes | learn_on == VDJ_genes){
		try{
			Gene_choice* j_gene_event_p = dynamic_cast<Gene_choice*>(events_map.at(tuple<Event_type,Gene_class,Seq_side>(GeneChoice_t,J_gene,Undefined_side)));
			jgene_offset_p = &j_gene_event_p->alignment_offset_p;
			jgene_real_index_p = &j_gene_event_p->current_realization_index;
			//Initialize gene counters

		}
		catch(exception& except){
			cout<<"Exception caught during initialization of Hypermutation global error rate"<<endl;
			cout<<"Exception caught trying to initialize J gene pointers"<<endl;
			cout<<endl<<"throwing exception now..."<<endl;
			throw except;
		}
	}

	//Get deletion value pointer for J 5' deletions if it exists
	if(events_map.count(tuple<Event_type,Gene_class,Seq_side>(Deletion_t,J_gene,Five_prime)) != 0){
		const Deletion* j_5_del_event_p = dynamic_cast<Deletion*>(events_map.at(tuple<Event_type,Gene_class,Seq_side>(Deletion_t,J_gene,Five_prime)));
		j_5_del_value_p = &(j_5_del_event_p->deletion_value);
	}
	else{j_5_del_value_p = &no_del_buffer;}

	//Initialize booleans for constructed sequences
	if(events_map.count(tuple<Event_type,Gene_class,Seq_side>(GeneChoice_t,V_gene,Undefined_side))>0){
		v_gene=true;
	}
	else{v_gene=false;}
	if(events_map.count(tuple<Event_type,Gene_class,Seq_side>(GeneChoice_t,D_gene,Undefined_side))>0){
		d_gene=true;
	}
	else{d_gene=false;}
	if(events_map.count(tuple<Event_type,Gene_class,Seq_side>(GeneChoice_t,J_gene,Undefined_side))>0){
		j_gene=true;
	}
	else{j_gene=false;}
	if(events_map.count(tuple<Event_type,Gene_class,Seq_side>(Insertion_t,VJ_genes,Undefined_side))>0){
		vj_ins=true;
	}
	else{vj_ins=false;}
	if(events_map.count(tuple<Event_type,Gene_class,Seq_side>(Insertion_t,VD_genes,Undefined_side))>0){
		vd_ins=true;
	}
	else{vd_ins=false;}
	if(events_map.count(tuple<Event_type,Gene_class,Seq_side>(Insertion_t,DJ_genes,Undefined_side))>0){
		dj_ins=true;
	}
	else{dj_ins=false;}

	this->clean_all_counters();

}

void Hypermutation_global_errorrate::add_to_norm_counter(){
	if(seq_likelihood!=0){

		if(learn_on_v){
			double test_sum=0;
			for(i = 0 ; i!=n_v_real ; ++i){
				//Get the length of the gene and a pointer to the right array to write on
				tmp_corr_len = v_gene_nucleotide_coverage_seq_p[i].first;
				tmp_cov_p = v_gene_nucleotide_coverage_seq_p[i].second;
				tmp_err_p = v_gene_per_nucleotide_error_seq_p[i].second;

				double* tmp_cov_mod_p = v_gene_nucleotide_coverage_p[i].second;
				double* tmp_err_mod_p = v_gene_per_nucleotide_error_p[i].second;



				for(j = 0 ; j!= tmp_corr_len ; j++){
					test_sum+=tmp_err_p[j]/seq_likelihood;

					//Add to normalize counter and reset coverage
					tmp_cov_mod_p[j] += tmp_cov_p[j]/seq_likelihood;
					tmp_cov_p[j] = 0;


					//Same for errors
					tmp_err_mod_p[j] += tmp_err_p[j]/seq_likelihood;
					tmp_err_p[j] = 0;
				}
			}
		}

		if(learn_on_d){

		}

		if(learn_on_j){

		}

		model_log_likelihood+=log10(seq_likelihood);
		number_seq+=1;
	}

	seq_mean_error_number = 0;
	seq_likelihood = 0;
	seq_probability = 0;
	debug_number_scenarios=0;
}

void Hypermutation_global_errorrate::clean_seq_counters(){
	//if(seq_likelihood!=0){

			if(learn_on_v){
				for(i = 0 ; i!=n_v_real ; ++i){
					//Get the length of the gene and a pointer to the right array to write on
					tmp_corr_len = v_gene_nucleotide_coverage_seq_p[i].first;
					tmp_cov_p = v_gene_nucleotide_coverage_seq_p[i].second;
					tmp_err_p = v_gene_per_nucleotide_error_seq_p[i].second;

					for(j = 0 ; j!= tmp_corr_len ; j++){
						//reset coverage
						tmp_cov_p[j] = 0;

						//Same for errors
						tmp_err_p[j] = 0;
					}
				}
			}

			if(learn_on_d){

			}

			if(learn_on_j){

			}
//	}


	seq_mean_error_number = 0;
	seq_likelihood = 0;
	seq_probability = 0;
	debug_number_scenarios=0;
}


void Hypermutation_global_errorrate::clean_all_counters(){
	if(learn_on_v){
		for(i = 0 ; i!=n_v_real ; ++i){
			//Get the length of the gene and a pointer to the right array to write on
			tmp_corr_len = v_gene_nucleotide_coverage_p[i].first;
			tmp_cov_p = v_gene_nucleotide_coverage_p[i].second;
			tmp_err_p = v_gene_per_nucleotide_error_p[i].second;

			for(j = 0 ; j!= tmp_corr_len ; j++){
				//reset coverage
				tmp_cov_p[j] = 0;

				//Same for errors
				tmp_err_p[j] = 0;
			}
		}
	}

	if(learn_on_d){

	}

	if(learn_on_j){

	}

this->clean_seq_counters();

}


void Hypermutation_global_errorrate::write2txt(ofstream& outfile){
	outfile<<"#Hypermutationglobalerrorrate;"<<this->mutation_Nmer_size<<";"<<this->learn_on<<";"<<this->apply_to<<endl;
	outfile<<R<<endl;
	outfile<<ei_nucleotide_contributions[0];
	for(i=1 ; i!=mutation_Nmer_size*4 ; ++i){
		outfile<<";"<<ei_nucleotide_contributions[i];
	}
	outfile<<endl;
}

void Hypermutation_global_errorrate::update_Nmers_proba(int current_pos , int current_index,double current_score){
	//Iterate through possible nucleotides at this position
	for(int ii=0;ii!=4;++ii){
		int new_index = current_index + ii*adressing_vector[current_pos];
		double new_score = current_score*exp(ei_nucleotide_contributions[current_pos*4 +ii]);
		if(current_pos!=mutation_Nmer_size-1){
			update_Nmers_proba(current_pos+1,new_index,new_score);
		}
		else{
			this->Nmer_mutation_proba[new_index]= new_score*R/(1+new_score*R);
			cout<<new_index<<";"<<new_score*R/(1+new_score*R)<<endl;
		}
	}
}

void Hypermutation_global_errorrate::compute_P_SHM_and_BG(){
	//Initialize P_SHM and P_BG to 0


	if(learn_on_v){
		for(unordered_map<string,Event_realization>::const_iterator real_iter = v_realizations.begin() ; real_iter!=v_realizations.end() ; real_iter++){
			pair<size_t,double*> nucleotide_coverage = v_gene_nucleotide_coverage_p[(*real_iter).second.index];
			pair<size_t,double*> nucleotide_error = v_gene_per_nucleotide_error_p[(*real_iter).second.index];

			//Get the first Nmer on the gene
			Nmer_index = 0;
			current_Nmer = queue<size_t>();
			//and get min coverage for the first Nmer
			double min_coverage = INT16_MAX;
			for(j=0 ; j!=mutation_Nmer_size ; j++){
				tmp_int_nt = stoi((*real_iter).second.value_str_int.substr(j,1));
				current_Nmer.push(tmp_int_nt);
				Nmer_index+=adressing_vector[j]*tmp_int_nt;

				if(nucleotide_coverage.second[j]<min_coverage){
					min_coverage = nucleotide_coverage.second[j];
				}
			}

			Nmer_P_BG[Nmer_index] += min_coverage; //The coverage of the Nmer is only as high as the lowest covered nt
			Nmer_P_SHM[Nmer_index] += nucleotide_error.second[(mutation_Nmer_size-1)/2];

			for(i=(mutation_Nmer_size-1)/2 +1 ; i!=(*real_iter).second.value_str_int.size() - (mutation_Nmer_size-1)/2; ++i){

				//Remove the previous first nucleotide of the Nmer and it's contribution to the index
				Nmer_index-=current_Nmer.front()*adressing_vector[0];
				current_Nmer.pop();
				//Shift the index
				Nmer_index*=4;
				//Add the contribution of the new nucleotide

				tmp_int_nt = stoi((*real_iter).second.value_str_int.substr(i+(mutation_Nmer_size-1)/2,1));//Assume a symetric Nmer

				Nmer_index+=tmp_int_nt;
				current_Nmer.push(tmp_int_nt);

				double min_coverage = INT16_MAX;
				for(j=-(mutation_Nmer_size-1)/2 ; j!= (mutation_Nmer_size-1)/2 +1 ; j++){
					if(nucleotide_coverage.second[j+i]<min_coverage){
						min_coverage = nucleotide_coverage.second[j+i];
					}
				}

				if(min_coverage!=0){
					cout<<"kinda work more"<<endl;
				}
				if(nucleotide_coverage.second[i]!=0){
					cout<<"is this reassuring?"<<endl;
				}
				bool test = std::isnan(nucleotide_error.second[i]);
				if(!test){
					//cout<<"no problem"<<endl;
				}else{
					cout<<"problem"<<endl;
					cout<<i<<endl;
					cout<<(*real_iter).first<<endl;
				}
				Nmer_P_BG[Nmer_index] += min_coverage; //The coverage of the Nmer is only as high as the lowest covered nt
				Nmer_P_SHM[Nmer_index] += nucleotide_error.second[i];

			}
		}
	}

	if(learn_on_d){

	}

	if(learn_on_j){

	}
}

double Hypermutation_global_errorrate::compute_Nmer_unorm_score(int* base_4_address){
	double unorm_score = 1;
	for(int ii = 0 ; ii != mutation_Nmer_size ; ++ii){
		unorm_score*=exp(ei_nucleotide_contributions[4*ii+base_4_address[ii]]);
	}
	return unorm_score;
}

void Hypermutation_global_errorrate::introduce_uniform_transversion(char& nt , std::default_random_engine& generator , std::uniform_real_distribution<double>& distribution) const{
	double rand_trans = distribution(generator);

	if(nt == 'A'){
		if(rand_trans<= 1.0/3.0){
			nt = 'C';
		}
		else if (rand_trans >= 2.0/3.0){
			nt = 'G';
		}
		else{
			nt = 'T';
		}
	}
	else if(nt == 'C'){
		if(rand_trans<= 1.0/3.0){
			nt = 'A';
		}
		else if (rand_trans >= 2.0/3.0){
			nt = 'G';
		}
		else{
			nt = 'T';
		}
	}
	else if(nt == 'G'){
		if(rand_trans<= 1.0/3.0){
			nt = 'A';
		}
		else if (rand_trans >= 2.0/3.0){
			nt = 'C';
		}
		else{
			nt = 'T';
		}

	}
	else if (nt == 'T'){
		if(rand_trans<= 1.0/3.0){
			nt = 'A';
		}
		else if (rand_trans >= 2.0/3.0){
			nt = 'C';
		}
		else{
			nt = 'G';
		}
	}
	else{
		throw runtime_error("unknown nucleotide in Hypermutationglobalerrorrate::generate_errors()");
	}
}