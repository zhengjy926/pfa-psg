# PSG v3 firmware

Firmware for the pulse signal generator board (STM32G474QET6): ECSP slave,
pulse sequencing, current settlement and safety supervision.  Requirement,
protocol and parameter baselines live in [docs](docs/README.md).

## Layout

| Layer | Directory | Contents |
| --- | --- | --- |
| Application | `app/` | task assembly, ECSP receive/session/transmit, reports, `app_config.h` |
| Run control | `run/` | run configuration, pulse-engine state, status snapshots |
| Services | `svc/` | dedup FIFO, fault word, identity records |
| Board | `bsp/psg_v3/` | device instances, clocks, pins, `FreeRTOSConfig.h` |
| Drivers | `drv/` | public device interfaces; `drv/stm32g4/` implementations |
| Shared | `shared/` | board-to-task event contract |
| Third party | `third_party/` | FreeRTOS-Kernel V11.1.0, STM32G4 CMSIS device support |

## Builds

Host test build (no board required):

```powershell
cmake -S . -B build/host -G Ninja -DCMAKE_C_COMPILER=gcc
cmake --build build/host
ctest --test-dir build/host --output-on-failure
```

Target build with arm-none-eabi-gcc:

```powershell
cmake -S . -B build/fw -G Ninja -DPSG_BUILD_FIRMWARE=ON `
  -DCMAKE_TOOLCHAIN_FILE="$PWD/cmake/toolchains/arm-none-eabi.cmake"
cmake --build build/fw
```

The released IDE build is `keil/psg_v3.uvprojx` (MDK-ARM, Arm Compiler 6,
scatter file `bsp/psg_v3/psg_v3.sct`).  Batch build:

```powershell
& 'C:\Keil_v5\UV4\UV4.exe' -b 'keil\psg_v3.uvprojx' -j0 -o 'keil\build.log'
```

## Verification status

- Host suite covers frame/CRC, receive resynchronisation and half-packet
  timing, session/handshake/heartbeat, adjudication order, dedup, run state,
  upload gating and 0x3C/0x11 reporting.
- Both target builds link with 0 errors and 0 warnings.  The static RAM budget
  in the map file is 21.4 KB (limit 32 KB, PRM-602).
- Bench-only items remain pending: UART receive-timestamp error band (HW-05),
  PB9 blink and six-task scheduling on hardware, HRTIM/ADC/DAC chains (later
  tickets), and the IWDG supervision policy (E-008, later ticket).
- The `timebase`/`uart` host doubles live in `tests/host/fakes/`; the production
  drivers are selected by the board static instances, not by a compile switch.

## Known documentation conflicts

- I-007 says a dedup entry leaves the queue when its response completes, while
  the same clause requires up to three original-response replays.  This build
  keeps entries for the most-recent-transactions window (ECSP 4.3) and dequeues
  the *replayed frame* on completion; the conflict is recorded for review.
- E-002 requires an idle latch transition to report 0x11 while not setting the
  fault word, but 0x11 must carry a non-zero word.  This build reports 0x11 only
  for a 0->1 fault-bit transition; the over-current ticket owns the resolution.
