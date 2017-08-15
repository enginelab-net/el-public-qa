Feature: SystemState

Output patches: "EngineState [d]"

Required inputs:
"EngineSpeed [RPM]"
"EngineSpeed_starting_threshold [RPM]"
"EngineSpeed_steady_threshold [RPM]"


Sub-features:
- None.

The system state is defined as follows:
State                                          Reserved name
0 - Rest state                                 SS_EngineState_rest
1 - Starting state                             SS_EngineState_starting
2 - Approaching steady state                   SS_EngineState_approaching
3 - Steady state                               SS_EngineState_steady
4 - Low load state                             SS_EngineState_lowload
5 - High load state                            SS_EngineState_highload
6 - Stress state                               SS_EngineState_stress

