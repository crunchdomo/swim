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
- `adaptation/` — `BaseAdaptationManager` (abstract) → `ReactiveAdaptationManager`, `ReactiveAdaptationManager2`. These decide when to add/remove servers or adjust brownout dimmer.
- `execution/` — `ExecutionManager` (abstract) → `ExecutionManagerMod`, `ExecutionManagerHAProxy`. Carries out adaptation decisions (server scaling, dimmer changes).
- `monitor/` — `SimpleMonitor` collects periodic observations via signal-based probes (`IProbe` → `SimProbe`, `HAProxyProbe`).

**Modules** (`src/modules/`) — simulation building blocks:
- `MTServer`/`MTBrownoutServer` — multi-threaded servers, the latter supporting graceful degradation
- `PredictableSource` — workload generator driven by trace files (WorldCup, Clarknet in `simulations/swim/traces/`)
- `ArrivalMonitor` — request arrival statistics

**Model** (`src/model/`) — system state representation tracking servers, configurations, and environment. Interfaces with pladapt library (mock version in `model/pladaptMock/`).

**External Control** (`src/externalControl/`) — TCP server exposing probes (dimmer, utilization, response time, throughput, arrival rate) and effectors (add/remove server, set dimmer) for external adaptation managers. Protocol documented in `docs/ExternalControl.pdf`.

### Simulation Configurations

- `simulations/swim_sa/swim_sa.ini` — embedded adaptation manager, fast simulation
- `simulations/swim/swim.ini` — external adaptation manager, real-time simulation

### Network Topology

Defined in NED files (`src/*.ned`, `simulations/*/*.ned`). Modules are wired together declaratively — the NED files define the simulation network structure while INI files configure parameters.

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
