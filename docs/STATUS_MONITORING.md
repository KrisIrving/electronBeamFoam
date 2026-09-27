# ./Status 运行监控说明

本文档说明 barePlate_calibration_Zakirov2020 中 ./Status 显示的每一类信息、
数据来源、数值/物理含义、使用方法和已知限制。

Status 的定位是：

> 不持续跟随庞大的 solver log，而是快速回答“算到哪里、算得多快、
> 热源是否正常、当前物理解是什么、MPI 是否健康、为什么可能变慢”。

它不是最终结果归档工具，也不是完整故障诊断工具。计算结束后仍应使用
./SummarizeRun 和相应 comparison scripts；发生异常时应结合
./DiagnoseFailure 和原始 log.electronBeamFoam。

---

## 1. 使用方法

进入 calibration case：

~~~bash
cd tutorials/electronBeamFoam/barePlate_calibration_Zakirov2020
./Status
~~~

如需保留某次状态快照：

~~~bash
./Status | tee status-$(date +%Y%m%d-%H%M%S).txt
~~~

Status 只读取日志、CSV 和进程信息，不修改计算场，也不会推进时间步。

---

## 2. 先区分“实时信息”和“write-time 信息”

不同字段的更新时间不同，这是正确理解 Status 的第一步。

| 信息 | 数据来源 | 更新频率 |
|---|---|---|
| simulation time / ExecutionTime / ClockTime / deltaT | log.electronBeamFoam | 基本每个 CFD step |
| throughput | log.electronBeamFoam | 根据已完成 steps 计算 |
| beam diagnostics | solver log | 正常 write time |
| performance profile | solver log | 正常 write time |
| local cells / imbalance / heaviest rank | performance profile | 正常 write time |
| melt-pool | meltPool.csv | 正常 write time |
| cumulative fusion zone | fusionZone.csv | 正常 write time |
| station-wise sections | fusionZoneSections.csv | 正常 write time |
| PID / MPI rank 数 | Linux process table | 调用 Status 时 |
| log age | 文件修改时间 | 调用 Status 时 |

因此例如：

~~~text
Latest simulation time = 0.000840 s
Latest fusion-zone sections final time = 0.000800 s
~~~

不表示 section 已停止演化，只表示 solver 还没有到下一个 write time。

---

## 3. Calibration input

最上面的 Calibration input 来自 log.generateTrack。

典型字段：

~~~text
preheat
power
scan speed
x start/end
track length
scan time
line energy
absorptivity
beam radius
penetration
epsilon tol
max T corr
beam seed
write interval
mesh profile
pressure profile
PIMPLE profile
decomp profile
~~~

用途是确认“当前 log 到底对应哪一个 case”。

在 resume、参数 sweep、decomposition A/B 时，不要只靠目录名判断输入。
尤其检查：

- mesh profile，例如 gradedYSmooth；
- pressure profile，当前 baseline 为 pcg；
- PIMPLE profile，当前 baseline 为 pCorr2；
- decomp profile，例如 scotch 或 simpleXZ；
- x start/end 与 track length，确认是 smoke test、0.5 mm、1 mm 还是 3 mm。

---

## 4. Latest simulation time

例如：

~~~text
Latest simulation time: Time = 0.0008400695627
~~~

这是“模拟物理时间”，不是计算机已经运行的真实时间。

3 mm、3 m/s 时总扫描物理时间为：

~~~text
0.003 / 3 = 0.001 s = 1 ms
~~~

因此 Time = 0.00084 s 代表扫描物理时间已推进约 84%。

---

## 5. ExecutionTime 与 ClockTime

例如：

~~~text
ExecutionTime = 168158.95 s
ClockTime     = 168302 s
~~~

### ClockTime

真实经过的墙钟时间。判断：

- 算例实际耗时；
- A/B 谁更快；
- 还需运行多久；

应优先使用 ClockTime。

### ExecutionTime

OpenFOAM 的 CPU execution timing。

MPI 情况下不要把它理解为 48 ranks 的 CPU 时间总和，也不要由它计算
总 CPU-hours。Status 中 CPU/Clock 仅作为 timing 辅助量，不等价于
MPI parallel efficiency。

---

## 6. Latest deltaT

例如：

~~~text
Latest deltaT = 7.15e-09 s
~~~

表示当前一个 CFD step 只推进约 7.15 ns 物理时间。

这是解释“为什么 suddenly slow”的第一项指标。

推进 1 microsecond 时：

- deltaT = 2e-8 s，约需要 50 steps；
- deltaT = 5e-9 s，约需要 200 steps。

即使每一步的成本完全相同，后者单位物理时间成本也约为前者 4 倍。

因此性能判断必须联合查看：

~~~text
throughput
deltaT
global/local cells
cell imbalance
~~~

---

## 7. Recent simulation throughput

典型输出：

~~~text
Recent simulation throughput:
  recent 200 steps: 456.78 wall-s/sim-us
                    (7.88 sim-us/wall-h), ETA~20.3 h
  since resume log: 311.37 wall-s/sim-us
                    (11.56 sim-us/wall-h), ETA~13.8 h
~~~

### wall-s/sim-us

定义：

~~~text
真实墙钟秒数 / 推进的模拟微秒数
~~~

若两个样本为：

~~~text
simulation time: t0 -> t1
ClockTime:       C0 -> C1
~~~

则：

~~~text
wall-s/sim-us =
(C1 - C0) / ((t1 - t0) * 1e6)
~~~

越小越快。

例如 180 wall-s/sim-us 表示：

> 模拟 1 microsecond 的真实物理过程，需要电脑运行约 180 秒。

### sim-us/wall-h

为上一指标的倒数表达：

~~~text
sim-us/wall-h = 3600 / (wall-s/sim-us)
~~~

越大越快。

20 sim-us/wall-h 表示：

> 电脑真实运行 1 小时，可推进约 20 microseconds 的模拟时间。

### recent 200 steps

只看最近约 200 个完整 CFD steps。

回答：

> 现在这一小段跑多快？

它最适合发现最近是否突然变慢。

若：

~~~text
recent 200 steps = 450 wall-s/sim-us
since resume     = 300 wall-s/sim-us
~~~

说明最近明显慢于长期平均。

### since resume log

使用当前 log.electronBeamFoam 中全部可配对的 Time / ExecutionTime /
ClockTime 样本。

resume 后的新 log 中，它表示“从 resume 到现在的平均速度”；
fresh run 中，它实际表示“当前 solver log 从开始到现在的平均速度”。

### ETA

当前脚本按 t = 0.001 s 作为目标时间做线性外推，因此主要服务于
3 mm / 3 m/s full reference。

注意：

- ETA 假设后续速度与采样窗口类似；
- deltaT、cell 数、thermal correctors 都可能继续变化；
- 对 0.5 mm 或 1 mm probe，当前 ETA 没有严格意义，不应用于判断短算例剩余时间。

---

## 8. Electron-beam diagnostics

这组指标检查电子束热源是否正常工作。

### Latest power error

定义：

~~~text
power error =
integrated deposited power - effective absorbed power
~~~

900 W 且 absorptivity=0.85、全部 beamlets 命中时：

~~~text
effective absorbed power = 765 W
~~~

理想 power error 仅为 floating-point roundoff，例如 1e-11 W 量级。

### Latest tracking mode

当前 baseline：

~~~text
multiRayFirstHit
~~~

表示沉积深度从局部 instantaneous metal/vacuum first-hit surface 计算，
而不是从固定参考面计算。

### Latest beamlet hits

当前 5 radial x 12 angular = 60 beamlets。

正常 bare plate 应看到：

~~~text
beamlets hit metal = 60
~~~

少于 60 时需要检查：

- beam footprint 是否离开工件；
- 自由表面是否极端变形；
- ray tracking；
- multiRay miss handling。

### Latest hit fraction

表示命中 beamlets 所代表的 Gaussian power fraction。

~~~text
hit fraction = 1
~~~

表示采样 beam power 全部找到金属 first hit。

### first-hit mean / min / max

表示 beamlets 从 seed 到局部 metal first-hit surface 的几何距离。

它们反映：

- 表面局部起伏；
- 各 beam sectors 的表面位置；
- multiRay surface mapping 是否正常。

不要把它们当作 penetration depth。

first-hit distance 是“真空 seed 到自由表面距离”；
penetrationDepth 是“材料内部能量沉积尺度”。

---

## 9. Performance profile

Status 显示：

~~~text
Latest profile total
Latest thermal share
Latest pressure share
Latest beam share
~~~

这些来自 solver 内置 profiler，通常只在 write time 汇总。

详细 profiler 定义见 docs/PERFORMANCE_PROFILING.md。

### wall total

表示“从上一个 write 到当前 write”这一段的总 critical-path wall cost，
不是整个 case 总时间。

### thermal/phase

主要是温度方程、液相分数和相变 correction 的成本。

若 thermal share 同时伴随：

- thermal correctors 增多；
- thermal cap hits 增多；
- interface residual 增大；

则成熟熔池的相变界面收敛可能成为主要瓶颈。

### pressure

pressure correction 成本。

当前 accepted baseline 为：

~~~text
PCG + DIC
PIMPLE nCorrectors = 2
~~~

这个组合已经通过专门 A/B，不应仅因某次 pressure share 较高就随意更换。

### electron beam

包括 first-hit tracking 和 deposition construction。

当前 multiRay baseline 下通常不是最大 wall-time 项。

---

## 10. MPI cell distribution

这是 moving dynamic AMR 长轨迹性能分析的核心。

### local cells min/max

例如：

~~~text
local cells min/max = 14375 / 237982
~~~

表示所有 MPI ranks 中：

- 最轻 rank 有 14,375 cells；
- 最重 rank 有 237,982 cells。

### cell imbalance max/mean

定义：

~~~text
imbalance =
maximum local cell count / mean local cell count
~~~

其中：

~~~text
mean local cells =
global cells / MPI ranks
~~~

理想值为 1。

本项目可以用以下尺度做快速判断：

| max/mean | 项目中的解释 |
|---:|---|
| 1.00–1.10 | 很均匀 |
| 1.10–1.25 | 轻度不均匀 |
| 1.25–1.50 | 值得关注 |
| 1.50–2.00 | 明显失衡 |
| >2.0 | 严重失衡 |

这些是项目经验尺度，不是 OpenFOAM 的官方硬阈值。

第一条完整 3 mm Scotch reference 最终约为 3.94，这正是当前 decomposition
优化的动机。

### heaviest rank

表示当前 cell 数最多的 MPI rank。

它不等于“实际 CPU 最慢的 rank”。

真实 workload 还受：

- interface cells；
- phase-change cells；
- pressure matrix；
- processor boundaries；
- AMR activity；

影响。

因此 cell imbalance 是重要 proxy，但不是完整的计算负载模型。

### 为什么 imbalance 会拖慢 MPI

许多 CFD 操作需要同步。

假设：

~~~text
rank0  1 s
rank1  1 s
rank2  1 s
rank3  2 s
~~~

前三个 ranks 算完后仍要等 rank3。

因此 parallel critical path 更接近最慢 rank，而不是平均 rank。

---

## 11. Latest melt-pool

来自：

~~~text
postProcessing/meltPoolDiagnostics/meltPool.csv
~~~

这是当前 write time 的“瞬时液态熔池”。

字段：

~~~text
L       length
W       width
D       depth
V       liquid volume
Tmax    maximum temperature
Umax    maximum velocity
recoil  maximum recoil pressure
cells   cells included in the diagnostic
~~~

概念：

~~~text
melt pool = 当前仍满足液态判据的区域
~~~

它不是最终凝固后的金相 fusion boundary。

---

## 12. Latest fusion-zone

来自：

~~~text
postProcessing/meltPoolDiagnostics/fusionZone.csv
~~~

这是累计 everMelted fusion zone。

概念：

~~~text
fusion zone = 历史上曾满足熔化判据的金属区域
~~~

所以：

~~~text
melt pool != fusion zone
~~~

beam 扫过后，一个区域可以：

- 已经重新凝固，不属于瞬时 melt pool；
- 仍属于 cumulative fusion zone。

对于最终金相比较，fusion zone 比 instantaneous melt pool 更相关。

但是 global fusion-zone W/D 是整条轨迹的 bounding-box extrema，
完整长轨迹中不作为主要 metallographic observable。

---

## 13. Latest fusion-zone sections

来自：

~~~text
postProcessing/meltPoolDiagnostics/fusionZoneSections.csv
~~~

当前 calibration 默认中央 stations：

~~~text
x = -0.5 mm
x =  0.0 mm
x = +0.5 mm
~~~

输出：

~~~text
W
D
section cells
~~~

对于完整 3 mm reference，主要实验 observable 定义为：

~~~text
central-window mean W/D
+
station-to-station spread
~~~

whole-track global bounding box 只作为 secondary diagnostic。

---

## 14. Station-wise experiment comparison

Status 优先读取：

~~~text
section-experiment-comparison.txt
~~~

若不存在但 section CSV 已存在，则调用 CompareExperimentSections.py。

它输出：

- 每个 station 的 W、W error；
- 每个 station 的 D、D error；
- central-window mean；
- station spread；
- central x=0 result。

只有完整 3 mm reference 才可把这些 error 当作正式 quantitative comparison。

0.5 mm、1 mm 等短轨迹主要用于 numerical / performance / diagnostic gate。

---

## 15. Experiment comparison

这一块是 legacy/global comparison。

优先读取 calibration-comparison.txt，否则调用 CompareExperiment.py。

它比较 cumulative global fusion-zone W/D 与实验。

在完整 3 mm case 中仍有参考价值，但优先级低于 station-wise comparison。

因此 global W/D 不能覆盖 central-window 的主要结论。

---

## 16. Run health

这一部分读取 Linux process table 和 solver log。

### Allrun PID

~~~text
Allrun PID = ... (active)
~~~

表示后台 wrapper 进程存在。

但 wrapper 存在不等于 solver 一定健康。

第一条完整 3 mm run 的 MPI peer-reset 已证明：

- wrapper 可以仍然存在；
- solver 可能已经失效或不再推进。

所以必须继续看 mpirun、rank count 和 log age。

### mpirun PID

表示在 Allrun wrapper 下发现的 mpirun electronBeamFoam 进程。

若 run 已经完成，找不到 mpirun 是正常的；
若 wrapper active 且应该正在计算，则异常。

### solver ranks = N / expected

正常 48 核运行：

~~~text
solver ranks = 48 / 48
~~~

如果变成 47 / 48，应视为 degraded MPI job，而不是普通“慢”。

### rank CPU min/mean/max

当前脚本使用 ps 的 %CPU。

重要限制：

> 当前这个值更接近进程生命周期/调度统计意义上的 CPU 利用率，
> 不是严格的 1 秒瞬时 MPI workload 采样。

因此 99/99/100% 可以说明 ranks 长期很忙，但不能单独证明此刻没有 MPI wait。

### max rank RSS

RSS = resident set size。

表示单个最吃内存的 solver rank 当前 resident memory。

它不是 48 ranks 的总内存。

### system-wide solver processes

表示整个系统中找到多少 electronBeamFoam -parallel 进程。

若：

~~~text
solver ranks = 48 / 48
system-wide solver processes = 49
~~~

可能存在一个不属于当前 mpirun 的残留 solver process。

应进一步检查 PID 来源，不要直接盲目 kill。

---

## 17. FAILURE SIGNATURE

Status 会扫描 log 中：

~~~text
Connection reset by peer
Segmentation fault
Killed
Out of memory
FOAM FATAL ERROR
MPI_ABORT
BAD TERMINATION
~~~

命中后显示：

~~~text
FAILURE SIGNATURE detected in solver log
~~~

重要限制：

> 当前实现扫描的是整个当前 log，因此可能命中“历史错误”。

例如：

1. 原 MPI job peer reset；
2. checkpoint resume；
3. 最终成功到 End；

如果旧错误文本仍在当前 log 中，Status 仍可能提示 failure signature。

因此一定要结合：

- wrapper 是否 active；
- ranks 是否完整；
- Time 是否继续增加；
- log age；
- 最终是否 End；

共同解释。

---

## 18. log age

~~~text
log age = N s
~~~

表示：

~~~text
当前时间 - log.electronBeamFoam 最后修改时间
~~~

健康运行时通常较小。

如果：

~~~text
wrapper active
log age >= 300 s
~~~

Status 会警告，并打印 log 最后 8 行。

进一步判断：

### ranks 不足

~~~text
State: FAILED/DEGRADED MPI job
~~~

表示等待一般不会自行恢复。

### 最后出现 Connection reset by peer

提示 MPI peer failure。

### 最后停在 AMR 信息

例如：

~~~text
Selected ... cells for refinement
Refined from ...
Unrefined from ...
~~~

Status 会提示可疑阶段为：

~~~text
dynamic mesh topology update / field mapping
~~~

注意：这是“定位可疑阶段”，不是仅靠最后几行就证明 dynamicRefineFvMesh
一定是根因。

---

## 19. 如何判断“为什么变慢”

推荐按以下组合分析。

### A. deltaT 显著下降

~~~text
throughput 下降
deltaT 明显下降
cell imbalance 变化不大
~~~

优先考虑：

- Courant 限制；
- Umax 上升；
- 自由表面运动增强；
- 单位物理时间需要更多 steps。

### B. global cells 明显增加

~~~text
throughput 下降
deltaT 相近
global cells 上升
~~~

说明每一步的 unknowns 增多。

再看 pressure / thermal / VOF shares。

### C. cell imbalance 明显增加

~~~text
throughput 下降
imbalance: 1.2 -> 2 -> 4
~~~

优先怀疑 moving AMR 后 static decomposition 越来越不均衡。

第一条完整 3 mm Scotch run 就属于这一类。

### D. pressure share 高

说明 pressure correction 是单步主要成本之一。

但当前 PCG/DIC + pCorr2 已经专门通过 A/B，不应因单次占比高就重新打开
pressure optimization。

### E. thermal share 和 thermal cap hits 同时升高

更可能是成熟熔池阶段的 interface phase-change convergence 变贵。

此时应使用 SummarizeRun 查看：

~~~text
thermal correctors
thermal cap hits
thermal conv residual
bulk/interface residual
~~~

### F. rank 数不完整或 log 很久不更新

这已经不是普通 performance issue，应进入 failure/stall workflow。

---

## 20. 推荐阅读顺序

每次 ./Status 后按这个顺序看：

~~~text
1. Calibration input
   确认 case 身份

2. simulation time + deltaT
   看物理时间推进和 timestep

3. recent throughput
   看最近是否变慢

4. beam hits + power error
   看热源是否正常

5. performance shares
   看单步成本花在哪里

6. local cells / imbalance
   看 MPI partition 是否恶化

7. melt pool / fusion zone / sections
   看物理解发展到哪里

8. run health
   看 wrapper、MPI ranks、log 是否真正健康
~~~

---

## 21. 常见误读

### section 数值没更新 = 熔池没变化

错误。

section 只在 write time 更新。

### CPU 都接近 100% = MPI balance 很好

错误。

优先看 local cells 和 cell imbalance，并结合 wall time。

### global fusion W/D = 实验金相 W/D

完整长轨迹中错误。

主要 observable 是 station-wise central-window W/D。

### FAILURE SIGNATURE = 当前 job 一定已坏

不一定。

它可能来自历史错误，需要结合 live health 和 End 判断。

### 0.5 mm / 1 mm probe 的 ETA 可以直接相信

当前不可以。

ETA 目标固定为 1 ms，主要针对完整 3 mm reference。

---

## 22. Status、SummarizeRun、DiagnoseFailure 的分工

### ./Status

正在运行时的快速监控：

- 到哪里；
- 多快；
- 热源是否正常；
- MPI 是否健康；
- 最新 write-time 物理结果。

### ./SummarizeRun

运行完成后或中间归档时的结构化摘要：

- performance profile 历史；
- thermal convergence；
- continuity；
- final diagnostics；
- A/B comparison 的输入。

### ./DiagnoseFailure

异常发生后的现场快照：

- process tree；
- error signatures；
- memory；
- filesystem；
- processor time distribution；
- kernel OOM / segfault clues。

三者不能互相替代。

---

## 23. 当前 moving-track calibration 最值得关注的字段

~~~text
Latest simulation time
Latest deltaT

recent 200 steps throughput
since resume log throughput

power error
beamlets hit metal
hit power fraction

wall total
thermal/phase
pressure

local cells min/max
cell imbalance max/mean
heaviest rank

fusion-zone sections

solver ranks / expected
system-wide solver processes
log age
~~~

长期最值得记录趋势的量：

~~~text
deltaT(t)
throughput(t)
globalCells(t)
cellImbalance(t)
thermalShare(t)
pressureShare(t)
W_section(t)
D_section(t)
~~~

这些分别对应：

- timestep constraint；
- 实际推进效率；
- 网格规模；
- MPI load balance；
- thermal/phase cost；
- pressure cost；
- 最终 validation observable。

---

## 24. 当前已知限制

当前 Status 有以下明确限制：

1. ETA 固定以 t=0.001 s 为目标，主要适用于 3 mm / 3 m/s full reference。
2. rank CPU 来自 ps %CPU，不是严格瞬时 MPI workload 采样。
3. failure signature 扫描整个当前 log，可能包含已经恢复的历史错误。
4. beam/performance/melt/fusion/section 只在 write time 更新。
5. cell-count imbalance 不等于真正的 compute-work imbalance。
6. Status 不判断物理模型本身是否正确，只报告已有 diagnostics。
7. Status 不替代最终 A/B comparison 和 validation decision。

如果未来修改 Status 的语义或算法，必须同步更新本文档；若变化影响研究解释，
还应在 log/entries 中记录修改理由。

---

## 25. 与整个研究方法论的关系

Status 不只是“方便看进度”。

它服务于 electronBeamFoam 的核心方法论：

> 在修改模型前，先判断问题属于 physics、numerics、parallel performance
> 还是 experimental observable。

例如 W/D 不对，不能立刻推出“热源参数错误”。

还必须排除：

~~~text
mesh resolution
phase-change convergence
station definition
dynamic-AMR load imbalance
stale write-time output
run failure
~~~

因此 Status 是 gate-based validation workflow 的实时观测入口，而不是单纯的
terminal convenience script。
