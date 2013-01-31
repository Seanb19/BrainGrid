/**
 *	@file Network.h
 *
 *
 *	@brief Header file for Network.
 */
//! A collection of Neurons and their connecting synapses.

/**
 ** \class Network Network.h "Network.h"
 **
 **\latexonly  \subsubsection*{Implementation} \endlatexonly
 ** \htmlonly  <h3>Implementation</h3> \endhtmlonly
 **
 ** \image html bg_data_layout.png
 **
 ** The network is composed of 3 superimposed 2-d arrays: neurons, synapses, and
 ** summation points (m_neuronList, m_rgSynapseMap, and m_summationMap).
 **
 ** Synapses in the synapse map are located at the coordinates of the neuron
 ** from which they receive output.  Each synapse stores a pointer into a
 ** m_summationMap bin.  Bins in the m_summationMap map directly to their output neurons.
 ** 
 ** If, during an advance cycle, a neuron \f$A\f$ at coordinates \f$x,y\f$ fires, every synapse
 ** \f$B\f$ at \f$x,y\f$ in the m_rgSynapseMap is notified of the spike. Those synapses then hold
 ** the spike until their delay period is completed.  At a later advance cycle, once the delay
 ** period has been completed, the synapses apply their PSRs (Post-Synaptic-Response) to their output bins \f$C\f$ in
 ** the m_summationMap.  Finally, on the next advance cycle, each neuron \f$D\f$ adds the value stored
 ** in their corresponding m_summationMap bin to their \f$V_m\f$ and resets the m_summationMap bin to
 ** zero.
 **
 ** \latexonly \subsubsection*{Credits} \endlatexonly
 ** \htmlonly <h3>Credits</h3> \endhtmlonly
 **
 ** This simulator is a rewrite of CSIM (2006) and other work (Stiber and Kawasaki (2007?))
 **
 **
 **	@authors Allan Ortiz and Cory Mayberry
 **/

#ifndef _NETWORK_H_
#define _NETWORK_H_

#include "global.h"
#include "include/Timer.h"
#include "ISimulation.h"

class Network
{
public:
	//! The constructor for Network.
	Network(FLOAT inhFrac, FLOAT excFrac, FLOAT startFrac, FLOAT Iinject[2], FLOAT Inoise[2],
			FLOAT Vthresh[2], FLOAT Vresting[2], FLOAT Vreset[2], FLOAT Vinit[2], FLOAT starter_Vthresh[2],
			FLOAT starter_Vreset[2], FLOAT new_targetRate, ostream& new_outstate, 
			ostream& new_memoutput, bool fWriteMemImage, istream& new_meminput, bool fReadMemImage, bool fFixedLayout, 
            vector<int>* pEndogenouslyActiveNeuronLayout, vector<int>* pInhibitoryNeuronLayout,
			SimulationInfo simInfo, ISimulation* sim);
	~Network();

	//! Frees dynamically allocated memory associated with the maps.
	void freeResources();

	//! Reset simulation objects
	void reset();

	//! Initialize neurons with simulation voltage and current parameters.
	void initNeurons(FLOAT Iinject[2], FLOAT Inoise[2], FLOAT Vthresh[2], FLOAT Vresting[2], FLOAT Vreset[2],
			FLOAT Vinit[2], FLOAT starter_Vthresh[2], FLOAT starter_Vreset[2]);

	//! Initialize entries in the neuron type map from random values
	void initNeuronTypeMap();

	//! Randomly initialize entries in the neuron starter map.
	void initStarterMap();

	//! Get list of neurons of random type
	vector<neuronType>* getNeuronOrder();
    
	//! Write the network state to an ostream.
	void saveSimState(ostream& os, CompleteMatrix& radiiHistory, CompleteMatrix& ratesHistory, VectorMatrix& xloc,
			VectorMatrix& yloc, VectorMatrix& neuronTypes, VectorMatrix& burstinessHist, VectorMatrix& spikesHistory,
			FLOAT Tsim, VectorMatrix& neuronThresh);

	//! Write the simulation memory image to an ostream
	void writeSimMemory(ostream& os, CompleteMatrix& radiiHistory, CompleteMatrix& ratesHistory);

	//! Read the simulation memory image from an istream
	void readSimMemory(istream& is, VectorMatrix& radii, VectorMatrix& rates);

	//! Performs the simulation.
	void simulate(FLOAT growthStepDuration, FLOAT num_growth_steps);

	//! Output the m_rgNeuronTypeMap to a VectorMatrix.
	void getNeuronTypes(VectorMatrix& neuronTypes);

	//! Output the m_pfStarterMap to a VectorMatrix.
	void getStarterNeuronMatrix(VectorMatrix& starterNeurons);

	//! The number of excitory neurons.
	int m_cExcitoryNeurons;

	//! The number of inhibitory neurons.
	int m_cInhibitoryNeurons;

	//! The number of endogenously active neurons.
	int m_cStarterNeurons;

	//! List of lists of synapses
	vector<ISynapse*>* m_rgSynapseMap;

	//! The map of summation points.
	FLOAT* m_summationMap;

	//! The neuron map.
	vector<INeuron*> m_neuronList;

	//! The neuron type map (INH, EXC).
	neuronType* m_rgNeuronTypeMap;

	//! The starter existence map (T/F).
	bool* m_rgEndogenouslyActiveNeuronMap;

	//! growth param (spikes/second) TODO: more detail here
	FLOAT m_targetRate;

	//! A file stream for xml output.
	ostream& state_out;

	//! An output file stream for memory dump
	ostream& memory_out;

	//! True if dumped memory image is written after simulation. 
	bool m_fWriteMemImage;

	//! An input file stream for memory image 	
	istream& memory_in;

	//! True if dumped memory image is read before starting simulation. 
	bool m_fReadMemImage;

	//! True if a fixed layout has been provided
	bool m_fFixedLayout;

	vector<int>* m_pEndogenouslyActiveNeuronLayout;

	vector<int>* m_pInhibitoryNeuronLayout;

private:
	// Struct that holds information about a simulation
	SimulationInfo m_si;

	// Simulator
	ISimulation* m_sim;

	//! Used to track the running simulation time.
	Timer m_timer;
	Timer m_short_timer;
	Network();
};
#endif // _NETWORK_H_
