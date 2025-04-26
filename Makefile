all: sim 

sim: 
	g++ -std=c++14 -O2 run_simulation.cc cheby.cc simulate_system.cc common.cc ev.cc -o bin/sim

load_curve: 
	g++ -std=c++14 -O2 run_simulation_load_curve.cc cheby.cc simulate_system_load_curve.cc params_load_curve.cc params_common.cc ev.cc -o bin/sim

debug: debug_sim 

debug_sim: 
	g++ -std=c++14 -O0 -ggdb -D DEBUG run_simulation.cc cheby.cc simulate_system.cc common.cc ev.cc -o bin/debug/sim

clean: 
	rm bin/sim bin/snc_lolp bin/snc_eue bin/debug/sim bin/debug/snc_lolp bin/debug/snc_eue
