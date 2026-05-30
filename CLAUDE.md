# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

SWIM (Simulator of Web Infrastructure and Management) is a discrete-event simulator built on **OMNeT++** for studying self-adaptive web systems. It simulates a web application with load balancing, multi-threaded servers, and brownout-based service degradation. Adaptation managers can be built as simulation modules or connected externally via a TCP interface (port 4242).

## Build Commands

### Native Build (OMNeT++ 6.1+)

SWIM has been ported to OMNeT++ 6.1. The modified queueinglib is bundled in `queueinglib/`. Requires Boost (serialization, filesystem).

```bash
# Set up OMNeT++ environment
source /path/to/omnetpp-6.1/setenv

# Build queueinglib (one-time)
cd queueinglib && make && cd ..

# Build SWIM
cd src && make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu)

# Run simulation
cd simulations/swim_sa/
./run.sh Reactive 1
```

### OMNeT++ 6.x Porting Notes

Key API changes from 5.4.1 to 6.x:
- Parameters modified at runtime must be marked `@mutable` in NED files
- Default values for `@unit(s)` parameters must include the unit (e.g., `default(0.0s)` not `default(0.0)`)
- `cObject::info()` renamed to `str()`
- `IServer` now has pure virtual `allocate()` method
- `PassiveQueue::selectionStrategy` changed from protected to private
- `boost_system` is now header-only in Boost 1.90+ (remove `-lboost_system` from link flags)

### Regenerate Makefile (if needed)

`src/Makefile` is committed, so `cd src && make` works out of the box. Only
regenerate if you change source layout or link flags. Note: the **root**
`Makefile`'s `make makefiles` target still references the pre-port sibling
layout (`-I../../queueinglib`), where `queueinglib` lived *next to* the repo.
Since queueinglib is now bundled at `queueinglib/` inside the repo, use the
command below (note `-I../queueinglib`) instead — adjust the `-I`/`-L` Boost
paths for your platform (the example uses Homebrew on macOS):

```bash
cd src && opp_makemake -f --deep -o swim -I. -Imodel/pladaptMock \
  -I../queueinglib -I/opt/homebrew/opt/boost/include \
  -L../libs -L../queueinglib/ -L/opt/homebrew/opt/boost/lib \
  -lqueueinglib -lboost_serialization -lboost_filesystem -lpthread
```

## Running Simulations

```bash
# Run with embedded adaptation manager (fast, non-realtime)
cd simulations/swim_sa/
./run.sh Reactive 1          # config=Reactive, run=1 (WorldCup trace, 60s boot delay)
./run.sh Reactive2 1         # Alternative adaptation without spare capacity constraint
# Usage: ./run.sh config [run-number(s)|all [ini-file]]

# Run with external adaptation manager (real-time, ~1h45m)
cd simulations/swim/
./run-sa.sh sim 1            # Launches SWIM + included simple_am
./run.sh sim 1               # Launches SWIM only (connect your own AM)

# Plot results (requires R with ggplot2, reshape, RSQLite)
cd results/
../swim/tools/plotResults.sh SWIM_SA Reactive 1 plot.pdf
```

## Architecture

### Core Layers (all in `src/`)

**Managers** (`src/managers/`) — implements the MAPE-K adaptation loop:
- `adaptation/` — `BaseAdaptationManager` (abstract) → `ReactiveAdaptationManager`, `ReactiveAdaptationManager2`. These decide when to add/remove servers or adjust the brownout dimmer; `UtilityScorer` computes the utility used to compare adaptation decisions.
- `execution/` — `ExecutionManager` (abstract) → `ExecutionManagerModBase` → `ExecutionManagerMod` (simulated infra) and `ExecutionManagerHAProxy` (real HAProxy). Adaptation decisions are realized as **tactics** (`Tactic` subclasses: `AddServerTactic`, `RemoveServerTactic`, `SetDimmerTactic`/`SetBrownoutTactic`) bundled into a `MacroTactic` and applied by the execution manager. `BootComplete.msg` models the asynchronous server boot delay.
- `monitor/` — `SimpleMonitor` collects periodic observations via signal-based probes (`IProbe` → `SimProbe`, `HAProxyProbe`).

The `.ned` interfaces (`IAdaptationManager`, `IExecutionManager`, `IProbe`) let the INI files swap implementations without code changes. `ModulePriorities.h` orders module initialization within an evaluation period.

**Modules** (`src/modules/`) — simulation building blocks:
- `MTServer`/`MTBrownoutServer` — multi-threaded servers, the latter supporting graceful degradation
- `PredictableSource` (and `PredictableRateSource`/`PredictableRandomSource`) — workload generators driven by trace files
- `ArrivalMonitor` — request arrival statistics

**Model** (`src/model/`) — system state representation (`Model`, `Configuration`, `Environment`, `Observations`) tracking servers, configurations, and environment. Interfaces with the pladapt library (mock version in `model/pladaptMock/`).

**External Control** (`src/externalControl/`) — `AdaptInterface` runs a TCP server (port **4242**) exposing probes (dimmer, utilization, response time, throughput, arrival rate) and effectors (add/remove server, set dimmer) for external adaptation managers. Protocol documented in `docs/ExternalControl.pdf`. A reference external AM client lives in `examples/simple_am/` (`SwimClient` connects to `4242`).

### Simulation Configurations

- `simulations/swim_sa/swim_sa.ini` — embedded adaptation manager, fast simulation
- `simulations/swim/swim.ini` — external adaptation manager, real-time simulation

### Network Topology

Defined in NED files (`src/package.ned`, `src/**/*.ned`, `simulations/*/*.ned`). Modules are wired together declaratively — the NED files define the simulation network structure while INI files configure parameters.

### Workload Traces & Results

- Trace files (`.delta` inter-arrival deltas) live in `simulations/swim_sa/traces/` and `simulations/swim/traces/` — WorldCup (`wc_day53-r0-105m-l70.delta`) and Clarknet (`clarknet-http-105m-l70.delta`). Each run number selects a trace (and boot delay) via the parametric study in the INI.
- Results are written under `results/` as OMNeT++ scalar/vector files (or a SQLite DB). Plotting helpers are in `tools/`: `plotResults.sh` (wraps `plotResults.R`, needs R with `ggplot2`, `reshape`, `RSQLite`) and `plotDeltaTrace.R` for visualizing traces.

## Docker

```bash
# Build Docker image
cd docker && ./build-images.sh

# Run container (VNC on 5901, HTTP on 6901, password: vncpassword)
docker run -d -p 5901:5901 -p 6901:6901 --name swim gabrielmoreno/swim
```

## Key Parameters (from INI files)

- Initial/max servers: 3
- Evaluation period: 60s
- Response time threshold: 0.75s
- Brownout levels: 5 (0.1 factor per level)
- Boot delay: 0-240s (parametric across runs)
