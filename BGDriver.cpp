/*
 *  The driver for braingrid.
 *  The driver performs the following steps:
 *  1) reads parameters from an xml file (specified as the first argument)
 *  2) creates the network
 *  3) launches the simulation
 *
 *  @authors Allan Ortiz and Cory Mayberry.
 */

#include <fstream>
#include "Global.h"
#include "paramcontainer/ParamContainer.h"

#include "Network.h"
#include "Model.h"


// Uncomment to use visual leak detector (Visual Studios Plugin)
// #include <vld.h>

#if defined(USE_GPU)
//    #include "GpuSim.h"
#elif defined(USE_OMP)
//    #include "MultiThreadedSim.h"
#else 
    #include "LIFSingleThreadedModel.h"
    #include "SingleThreadedSim.h"
#endif

using namespace std;

// state file name
string stateOutputFileName;
string stateInputFileName;

// memory dump file name
string memOutputFileName;
string memInputFileName;
bool fReadMemImage = false;  // True if dumped memory image is read before
                             // starting simulation
bool fWriteMemImage = false;  // True if dumped memory image is written after
                              // simulation

int poolsize[3];  // size of pool of neurons [x y z]

Model *model = NULL;

// Simulation Parameters
BGFLOAT Tsim;  // Simulation time (s) (between growth updates) rename: epochLength
int numSims;  // Number of Tsim simulation to run
int maxFiringRate;  // Maximum firing rate (only used by GPU version)
int maxSynapsesPerNeuron;  // Maximum number of synapses per neuron
                           // (only used by GPU version)
long seed;  // Seed for random generator (single-threaded)

// functions
SimulationInfo makeSimulationInfo(int cols, int rows,
    BGFLOAT growthEpochDuration, BGFLOAT maxGrowthSteps,
    int maxFiringRate, int maxSynapsesPerNeuron, BGFLOAT new_deltaT,
    long seed);
bool load_simulation_parameters(const string &sim_param_filename);
void LoadSimParms(TiXmlElement*);
//void SaveSimState(ostream &);
void printParams();
bool parseCommandLine(int argc, char* argv[]);

/**
 *  Main for Simulator. Handles command line arguments and loads parameters
 *  from parameter file. All initial loading before running simulator in Network
 *  is here.
 *  @param  argc    argument count.
 *  @param  argv    arguments.
 *  @return -1 if error, else if success.
 */
int main(int argc, char* argv[]) {


    #if defined(USE_GPU)
	// model = new LIFSingleThreadedModel();
    #elif defined(USE_OMP)
	// model = new LIFSingleThreadedModel();
    #else
	 model = new LIFSingleThreadedModel();
    #endif
    
    DEBUG(cout << "reading parameters from xml file" << endl;)

    if (!parseCommandLine(argc, argv)) {
        cerr << "! ERROR: failed during command line parse" << endl;
        return -1;
    }
    if (!load_simulation_parameters(stateInputFileName)) {
        cerr << "! ERROR: failed while parsing simulation parameters." << endl;
        return -1;
    }

    /*    verify that params were read correctly */
    DEBUG(printParams();)

// NETWORK MODEL VARIABLES NMV-BEGIN {
    // calculate the number of inhibitory, excitory, and endogenously active
    // neurons
    int numNeurons = poolsize[0] * poolsize[1];
// } NMV-END

    SimulationInfo si = makeSimulationInfo(poolsize[0], poolsize[1],Tsim, numSims,
            maxFiringRate, maxSynapsesPerNeuron, DEFAULT_dt, seed);

    // create the network
    Network network(model, si);

    time_t start_time, end_time;
    time(&start_time);

    Simulator *simulator;
	// It might be possible to do away with virtual function calls
	// by having simulator be a reference to a auto allocated derived class 
	// similar to the below code:
	//SingleThreadedSim test(&network, si);
	//simulator = &test;

	#if defined(USE_GPU)
	simulator = new SingleThreadedSim(&network, si);
	#elif defined(USE_OMP)
	simulator = new SingleThreadedSim(&network, si);
	#else
   	 simulator = new SingleThreadedSim(&network, si);
	#endif

	
	
    if (fReadMemImage) {
        ifstream memory_in;
        memory_in.open(memInputFileName.c_str(), ofstream::binary | ofstream::in);
        simulator->readMemory(memory_in);
        memory_in.close();
    }

    simulator->simulate();

    ofstream state_out(stateOutputFileName.c_str());
    simulator->saveState(state_out);
    state_out.close();

    ofstream memory_out;
    if (fWriteMemImage) {
        memory_out.open(memOutputFileName.c_str(),ofstream::binary | ofstream::trunc);
        simulator->saveMemory(memory_out);
        memory_out.close();
    }

    for(int i = 0; i < rgNormrnd.size(); ++i) {
        delete rgNormrnd[i];
    }

    rgNormrnd.clear();

    time(&end_time);
    double time_elapsed = difftime(end_time, start_time);
    double ssps = Tsim * numSims / time_elapsed;
    cout << "time simulated: " << Tsim * numSims << endl;
    cout << "time elapsed: " << time_elapsed << endl;
    cout << "ssps (simulation seconds / real time seconds): " << ssps << endl;
    
    delete model;
    model = NULL;
    
    delete simulator;
    simulator = NULL;

    return 0;
}

/*
 *  Init SimulationInfo parameters.
 *  @param  cols    number of columns for the simulation.
 *  @param  rows    number of rows for the simulation.
 *  @param  growthEpochDuration  duration in between each growth.
 *  @param  maxGrowthSteps  TODO
 *  @param  maxFiringRate   maximum firing rate for the simulation.
 *  @param  maxSynapsesPerNeuron    cap limit for the number of Synapses each Neuron can have.
 *  @param  new_deltaT  TODO (model independent)
 *  @param  seed    seeding for random numbers.
 *  @return SimulationInfo object encapsulating info given.
 */
SimulationInfo makeSimulationInfo(int cols, int rows,
        BGFLOAT growthEpochDuration, BGFLOAT maxGrowthSteps,
        int maxFiringRate, int maxSynapsesPerNeuron, BGFLOAT new_deltaT,
        long seed)
{
    SimulationInfo si;
    // Init SimulationInfo parameters
    int max_neurons = cols * rows;

    si.totalNeurons = max_neurons;
    si.epochDuration = growthEpochDuration;
    si.maxSteps = (int)maxGrowthSteps;

    // May be model-dependent
    si.maxFiringRate = maxFiringRate;
    si.maxSynapsesPerNeuron = maxSynapsesPerNeuron;

// NETWORK MODEL VARIABLES NMV-BEGIN {
    si.width = cols;
    si.height = rows;
// } NMV-END

    si.deltaT = new_deltaT;  // Model Independent
    si.seed = seed;  // Model Independent

    return si;
}

/**
 *  Prints loaded parameters out to console.
 */
void printParams() {
    cout << "\nPrinting parameters...\n";
    cout << "poolsize x:" << poolsize[0]
         << " y:" << poolsize[1]
         << " z:" << poolsize[2]
         << endl;
    cout << "Simulation Parameters:\n";
    cout << "\tTime between growth updates (in seconds): " << Tsim << endl;
    cout << "\tNumber of simulations to run: " << numSims << endl;

    cout << "Model Parameters:" << endl;
    model->printParameters(cout);
    cout << "Done printing parameters" << endl;
}

/**
 *  Load parameters from a file.
 *  @param  sim_param_filename  filename of file to read from
 *  @return true if successful, false if not
 */
bool load_simulation_parameters(const string &sim_param_filename)
{
    TiXmlDocument simDoc(sim_param_filename.c_str());
    if (!simDoc.LoadFile()) {
        cerr << "Failed loading simulation parameter file "
             << sim_param_filename << ":" << "\n\t" << simDoc.ErrorDesc()
             << endl;
        cerr << " error: " << simDoc.ErrorRow() << ", " << simDoc.ErrorCol()
             << endl;
        return false;
    }

    TiXmlElement* parms = NULL;

    if ((parms = simDoc.FirstChildElement("SimParams")) == NULL) {
        cerr << "Could not find <SimParms> in simulation parameter file "
             << sim_param_filename << endl;
        return false;
    }

    try {
        LoadSimParms(parms);
        model->readParameters(parms);
    } catch (KII_exception &e) {
        cerr << "Failure loading simulation parameters from file "
             << sim_param_filename << ":\n\t" << e.what()
             << endl;
        return false;
    }
    return true;
}

/**
 *  Handles loading of parameters using tinyxml from the parameter file.
 *  @param  parms   tinyxml element to load from.
 */
void LoadSimParms(TiXmlElement* parms)
{
    
    TiXmlElement* temp = NULL;

    // Flag indicating that the variables were correctly read
    bool fSet = true;

    // Note that we just grab the first child with the right value;
    // multiple children with the same values are ignored. This might
    // not be as quick as iterating through the children and setting the
    // parameters as each one's element is found, but the code is
    // simpler this way and the performance penalty is insignificant.

    if ((temp = parms->FirstChildElement("PoolSize")) != NULL) {
        if (temp->QueryIntAttribute("x", &poolsize[0]) != TIXML_SUCCESS) {
            fSet = false;
            cerr << "error x" << endl;
        }
        if (temp->QueryIntAttribute("y", &poolsize[1]) != TIXML_SUCCESS) {
            fSet = false;
            cerr << "error y" << endl;
        }
        if (temp->QueryIntAttribute("z", &poolsize[2]) != TIXML_SUCCESS) {
            fSet = false;
            cerr << "error z" << endl;
        }
    } else {
        fSet = false;
        cerr << "missing PoolSize" << endl;
    }

    if ((temp = parms->FirstChildElement("SimParams")) != NULL) {
        if (temp->QueryFLOATAttribute("Tsim", &Tsim) != TIXML_SUCCESS) {
            fSet = false;
            cerr << "error Tsim" << endl;
        }
        if (temp->QueryIntAttribute("numSims", &numSims) != TIXML_SUCCESS) {
            fSet = false;
            cerr << "error numSims" << endl;
        }
        if (temp->QueryIntAttribute("maxFiringRate", &maxFiringRate) != TIXML_SUCCESS) {
            fSet = false;
            cerr << "error maxFiringRate" << endl;
        }
        if (temp->QueryIntAttribute("maxSynapsesPerNeuron", &maxSynapsesPerNeuron) != TIXML_SUCCESS) {
            fSet = false;
            cerr << "error maxSynapsesPerNeuron" << endl;
        }
    } else {
        fSet = false;
        cerr << "missing SimParams" << endl;
    }

    if ((temp = parms->FirstChildElement("OutputParams")) != NULL) {
        if (temp->QueryValueAttribute("stateOutputFileName", &stateOutputFileName) != TIXML_SUCCESS) {
            fSet = false;
            cerr << "error stateOutputFileName" << endl;
        }
    } else {
        fSet = false;
        cerr << "missing OutputParams" << endl;
    }

    if ((temp = parms->FirstChildElement("Seed")) != NULL) {
        if (temp->QueryValueAttribute("value", &seed) != TIXML_SUCCESS) {
            fSet = false;
            cerr << "error value" << endl;
        }
    } else {
        fSet = false;
        cerr << "missing Seed" << endl;
    }

    // Ideally, an error message would be output for each failed Query
    // above, but that's just too much code for me right now.
    if (!fSet) throw KII_exception("Failed to initialize one or more simulation parameters; check XML");
}

/**
 *  Handles parsing of the command line
 *  @param  argc    argument count.
 *  @param  argv    arguments.
 *  @returns    true if successful, false otherwise.
 */
bool parseCommandLine(int argc, char* argv[])
{
    ParamContainer cl;
    cl.initOptions(false);  // don't allow unknown parameters
    cl.setHelpString(string("The DCT growth modeling simulator\nUsage: ") + argv[0] + " ");

#if defined(USE_GPU)
    if ((cl.addParam("stateoutfile", 'o', ParamContainer::filename, "simulation state output filename") != ParamContainer::errOk)
            || (cl.addParam("stateinfile", 't', ParamContainer::filename | ParamContainer::required, "simulation state input filename") != ParamContainer::errOk)
            || (cl.addParam("deviceid", 'd', ParamContainer::regular, "CUDA device id") != ParamContainer::errOk)
            || (cl.addParam("meminfile", 'r', ParamContainer::filename, "simulation memory image input filename") != ParamContainer::errOk)
            || (cl.addParam("memoutfile", 'w', ParamContainer::filename, "simulation memory image output filename") != ParamContainer::errOk)) {
        cerr << "Internal error creating command line parser" << endl;
        return false;
    }
#else    // !USE_GPU
    if ((cl.addParam("stateoutfile", 'o', ParamContainer::filename, "simulation state output filename") != ParamContainer::errOk)
            || (cl.addParam("stateinfile", 't', ParamContainer::filename | ParamContainer::required, "simulation state input filename") != ParamContainer::errOk)
            || (cl.addParam("meminfile", 'r', ParamContainer::filename, "simulation memory image filename") != ParamContainer::errOk)
            || (cl.addParam("memoutfile", 'w', ParamContainer::filename, "simulation memory image output filename") != ParamContainer::errOk)) {
        cerr << "Internal error creating command line parser" << endl;
        return false;
    }
#endif  // USE_GPU

    // Parse the command line
    if (cl.parseCommandLine(argc, argv) != ParamContainer::errOk) {
        cl.dumpHelp(stderr, true, 78);
        return false;
    }

    // Get the values
    stateOutputFileName = cl["stateoutfile"];
    stateInputFileName = cl["stateinfile"];
    memInputFileName = cl["meminfile"];
    memOutputFileName = cl["memoutfile"];
    if (!memInputFileName.empty())
        fReadMemImage = true;
    if (!memOutputFileName.empty())
        fWriteMemImage = true;
#if defined(USE_GPU)
    if (EOF == sscanf(cl["deviceid"].c_str(), "%d", &g_deviceId))
        g_deviceId = 0;
#endif  // USE_GPU
    return true;
}
