# Dynamic resources allocation in non-3GPP IoT networks involving UAVs


<p align='center' style="margin-bottom: -4px">Rogério S. Silva$$^\ast^\dagger$$, William P. Junior$$^\ast$$, Sand L. Correa$$^\ast$$, Kleber V. Cardoso$$^\ast$$, Antonio Oliveira-JR$$^\ast$$</p>
<p align='center' style="margin-bottom: -4px">$$^\dagger$$NumbERS, Instituto Federal de Goiás-Inhumas-GO</p>
<p align='center' style="margin-bottom: -4px">E-mail: {rogerio.sousa}@ifg.edu.br</p>
<p align='center' style="margin-bottom: -4px">$$^\ast$$Instituto de Informática, Universidade Federal de Goiás</p>
<p align='center'>E-mail: {williamtpjunior, sand, kleber, antonio}@inf.ufg.br</p>

## Abstract

In this work, we investigate how to minimize the number of gateways deployed in Unmanned Aerial Vehicles (UAVs) needed to meet the demand for non-3GPP Internet of Things (IoT) devices, seeking to improve the Quality of Service (QoS), keeping a balance between delay and data rate. Gateways deployed in UAVs add the mobility flexibility of UAVs, which paves the way for meeting emergency demand increments. The 5th Generation Networks (5G) and Beyond 5th Generation Networks (B5G) systems incorporated access to IoT devices via non-3GPP access, opening up new integration  possibilities. Furthermore, Low Power Wide Area Network (LPWAN) networks, especially Long Range Wide Area Network (LoRaWAN), allow access over long distances with reduced energy consumption. In this scenario, our work proposes a Mixed Integer Linear Programming (MILP) optimization model to minimize the number of UAVs that meet the increment of emergency demand, comply with limits of QoS, and maintain the compromise between delay and data rate. Simulation results show that the proposed model significantly reduces the number of UAVs, maintains optimal levels of QoS, and maintains the compromise between delay and data rate compared to the baselines.

## Baselines
Two methods are defined as baselines for our proposal, the Density-oriented (DO) and the Equidistant (EQ) UAVs placement models. 
### {Equidistant UAVs placement algorithm (EQ)}
EQ was developed to distribute the UAVs across the area and maintain the equidistance between them. The input data contains the placement area $\mathcal{A}_{(MxN)}$ and the lower bound QoS  to be reached. The algorithm iterates by incrementing the number of UAVs to find how many satisfy the $QoS_{bound}$. The inter-UAV distance is computed for each UAV added, and the UAVs are distributed, ensuring equidistance. This algorithm places the UAVs in the perspective of the coverage area and does not consider the positions of the EDs.
### {Density-oriented UAVs placement algorithm (DO)}
DO was developed to place UAVs in the most populated areas, following the EDs density map. The input data contains the map with the coordinates of EDs $\beta_{(MxN)}$. Initially, the area density information is calculated, then $\beta_{(MxN)}$ is linearized, sorted in descending order, and assigned to $\rho_{(L)}$. The algorithm iterates until reaching the target bound $QoS_{bound}$. In each iteration, a new UAV is added and placed in the corresponding position $\rho{(l)}$ of matrix of $\alpha_{(m,n)}$, where $l$ represents the linear index that corresponds to a $(m,n)$. The DO outputs a UAVs placement map $\alpha_{(m,n)}$.

## Proposed placement algorithm

<img src="/images/OP_Algorithm.png" width="500">

The OP algorithm presents our approach for positioning UAVs-based gateways following the optimization model in section 2. The algorithm receives as input the area for positioning devices ($\mathcal{A}_{(MxN)}$), the UAV placement area ($\mathcal{V}_{(XxYxZ)}$) and the expected QoS limit ($\rho^{QoS}_l$). In line 1, the devices are distributed following a realistic spatial distribution (here). In lines 2 and 3, the set of communication parameters between the devices and each virtual position, in addition to the association of the devices to the respective slices. In line 4, the optimization algorithm is executed. The OP results in the UAVs positions set $\mathcal{P}$, the SF (spreading factor) and TP (transmission power) configurations set $\mathcal{C}$, and the slices associations set $\mathcal{S}$. Finally, the resulting data are modeled in the NS-3, and the simulation is performed.

## Results
The results were obtained from 50 executions for 40, 50, and 60 devices. In each execution, a group of devices and then the UAVs are deployed according to the algorithms EQ, DO, and OP, presented earlier. 

### UAVs placement

<p align='center'>
    <img src='/images/UAVs_Placements.png' width='500'>
</p>    
<p align='center'>
    <figurecaption>
        Fig. 1. UAVs placement.
    </figurecaption>
</p>

Fig. 1 shows the area for UAVs placement in a scenario with 60 EDs. Colored symbols represent UAVs for each placement method, and black circles represent devices.

### Number of uavs needed to serve the devices ensuring the QoS level. 
<p align='center'> <img src="/images/UAVsXDevices.png" width="500"> </p>
<p align='center'>
    <figurecaption>
        Fig. 2. Devices serviced and number of UAVs required.
    </figurecaption>
</p>

Figure 2 presents the number of UAVs needed to meet the QoS threshold determined as a target. For a Qbound = 0.9, OP presents the best results, e.g., to meet an average above 50 EDs, with a standard deviation near to the other algorithms, four UAVs were deployed with OP, while DO and EQ needs more. The DO and EQ have a less pronounced growth curve for serving the EDs, which impacts a higher number of deployed UAVs. Another critical point to evaluate is the number of effective runs to obtain each point. In a universe of 50 runs, for each set of EDs, we consider effective those that resulted in a Qbound ≥ 0.9; the highlights of the numerical annotations in the graph denote these effective rounds.

### QoS
<p align='center'> <img src="/images/Heatmap_QoS.png" width="700"></p>
<p align='center'>
    <figurecaption>
        Fig. 3. QoS.
    </figurecaption>
</p>

Figure 3 displays results relative to QoS. The large areas in light colors for OP demonstrate the excellent efficiency of the proposed algorithm. Furthermore, it is possible to observe that the number of unanswered EDs is more minor than DO and EQ, denoted by the black dots. Another highlight about DO and EQ is that in addition to presenting the highest rates of EDs not met, the rates of QoS reached are much worse than in OP, that is denoted by the red, and purple dots on the respective figures.

# Replicating The Experiment

## Requirements

- GNU (>=8.0.0)
- CMAKE (>=3.24)
- python (3.11.4)
- [SCIP Optimization Suite (8.0.3)](https://scipopt.org/index.php#download)

## Preparing Environment

Start by cloning this repository.

```bash
git clone https://github.com/LABORA-INF-UFG/non3GPP_IoT_simulations.git 
cd non3GPP_IoT_simulations
```

The first step is to build the version 3.36 of NS3.

```bash
git clone https://github.com/nsnam/ns-3-dev-git ns-3
cd ns-3
git checkout ns-3.36
cp -r ../contrib/* ./contrib/
ln -s ~/non3GPP_IoT_simulations/scratch/optimal-distrib-experiment.cc ~/non3GPP_IoT_simulations/ns-3/scratch/optimal-distrib-experiment.cc
ln -s ~/non3GPP_IoT_simulations/scratch/thesis-experiment.cc ~/non3GPP_IoT_simulations/ns-3/scratch/thesis-experiment.cc

./ns3 configure --enable-examples
./ns3 build
```

The following python packages are needed to execute the experiment.

```bash
pip install pyomo pandas matplotlib blob
```

We can then start the experimentation process, after every step you can check the generated files inside [data/](data/) folder.

### 1st Step - Generating Input Data

use the notebook [`equidistantPlacement.ipynb`](./equidistantPlacement.ipynb) to generate the files with the virtual positions for LoRa-GWs.

Generate the files with LoRa-ED positions using the NS3 script, you can modify the number of devices with the option `--nDevices=x` and the seed for the pseudo random distribution of the devices with the option `--seed=y`.

```bash
./ns-3/build/scratch/ns3.36-devices-density-oriented-distrib-default --nDevices=30 --seed=1
```

To finalize, generate the files with slice association of the devices.

```bash
./ns-3/build/scratch/ns3.36-thesis-experiments-default --nDevices=30 --seed=1 --nGateways=25 --nPlanes=1 --verbose
```

### 2nd Step - Optimization Model

To execute the optimization model, we need to pass in order the number of virtual positions (25), number of height levels (1), number of devices (30), device distribution seed (1) and QoS lower bound (0.9). 

```bash
python model.py 25 1 30 1 0.9
```

### 3rd Step - Simulation

We are ready to execute the simulation with the solution obtained in the last step. 

```bash
./ns-3/build/scratch/ns3.36-optimal-distrib-experiment-default --nDevices=30 --seed=1 --nGateways=25 

The simulation output can be found in the directory [./data/results/](./data/results/).
