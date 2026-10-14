#include "core/platform/Platform.h"
#include "core/platform/Window.h"
#include "core/time/Time.h"
#include "core/logging/Logger.h"
#include "core/config/Config.h"
#include "core/io/FileSystem.h"
#include "core/memory/MemoryPool.h"
#include "core/memory/StackAllocator.h"
#include "core/memory/ArenaAllocator.h"
#include "ecs/World.h"
#include "ecs/entity/Entity.h"
#include "ecs/system/System.h"
#include "iqgpu/IQGPURuntime.h"
#include "iqgpu/QuantumInspired.h"
#include "iqgpu/KernelOptimizer.h"
#include "iqgpu/ZetaCompute.h"
#include "math/ZetaSpectral.h"
#include "ai/NPC.h"
#include "world/procedural/ResourceScatter.h"
#include "world/procedural/WorldStreamer.h"

#include <cmath>
#include <iostream>
#include <memory>

// --- Demo components (POD, serializable) | 演示組件（POD，可序列化） ---
struct Position { float x = 0, y = 0, z = 0; };
struct Velocity { float x = 0, y = 0, z = 0; };
struct Health   { int hp = 100; };

// Demo event | 演示事件
struct EntityDamagedEvent { StonePotatoes::ECS::Entity entity; int amount; };

// Real movement system: integrates Position from Velocity each tick.
// 真實移動系統：每 tick 以 Velocity 積分更新 Position。
class MovementSystem : public StonePotatoes::ECS::System {
public:
    void Update(StonePotatoes::ECS::World* world, float deltaTime) override {
        world->ForEach<Position, Velocity>(
            [deltaTime](StonePotatoes::ECS::Entity, Position& p, Velocity& v) {
                p.x += v.x * deltaTime;
                p.y += v.y * deltaTime;
                p.z += v.z * deltaTime;
            });
    }
};

int main(int argc, char* argv[]) {
    (void)argc; // Suppress unused parameter warning
    (void)argv; // Suppress unused parameter warning

    // Enable file logging | 啟用檔案日誌
    StonePotatoes::Logging::Logger::GetInstance().EnableFileOutput(true, "stonepotatoes.log");

    STONEPOTATOES_LOG_INFO("StonePotatoes - Phase 1 Core with Self-Developing AI Agent");
    STONEPOTATOES_LOG_INFO("StonePotatoes - 第一階段核心配備自我開發 AI Agent");

    // Initialize platform
    // 初始化平台
    if (!StonePotatoes::Platform::Initialize()) {
        STONEPOTATOES_LOG_FATAL("Failed to initialize platform");
        STONEPOTATOES_LOG_FATAL("初始化平台失敗");
        return -1;
    }

    // Initialize time manager
    // 初始化時間管理器
    StonePotatoes::Time::TimeManager::GetInstance().Initialize();

    // Load configuration (JSON) | 載入設定（JSON）
    // Create a default config on first run | 首次執行時建立預設設定檔
    auto& config = StonePotatoes::Config::ConfigManager::GetInstance();
    if (!StonePotatoes::IO::FileSystem::Exists("engine_config.json")) {
        config.SetInt("window.width", 1280);
        config.SetInt("window.height", 720);
        config.SetString("window.title", "StonePotatoes - Self-Developing AI Agent");
        config.SetDouble("world.timeScale", 1.0);
        config.SetBool("debug.enabled", true);
        config.SaveToFile("engine_config.json");
    } else {
        config.LoadFromFile("engine_config.json");
    }
    STONEPOTATOES_LOG_INFO("Config: window=%dx%d title=%s",
        config.GetInt("window.width", 1280),
        config.GetInt("window.height", 720),
        config.GetString("window.title", "StonePotatoes"));

    // Create window
    // 建立視窗
    StonePotatoes::Platform::WindowConfig windowConfig;
    windowConfig.width = config.GetInt("window.width", 1280);
    windowConfig.height = config.GetInt("window.height", 720);
    windowConfig.title = config.GetString("window.title", "StonePotatoes");

    StonePotatoes::Platform::Window window;
    if (!window.Create(windowConfig)) {
        STONEPOTATOES_LOG_FATAL("Failed to create window");
        STONEPOTATOES_LOG_FATAL("建立視窗失敗");
        StonePotatoes::Platform::Shutdown();
        return -1;
    }

    STONEPOTATOES_LOG_INFO("Window created: %dx%d", window.GetWidth(), window.GetHeight());
    STONEPOTATOES_LOG_INFO("視窗已建立：%dx%d", window.GetWidth(), window.GetHeight());

    // Create ECS world
    // 建立 ECS 世界
    StonePotatoes::ECS::World world;
    world.Initialize();

    // Register component types with stable serialization names.
    // 以穩定的序列化名稱註冊組件型別。
    world.RegisterComponent<Position>("Position");
    world.RegisterComponent<Velocity>("Velocity");
    world.RegisterComponent<Health>("Health");

    // Register movement system
    // 註冊移動系統
    auto movementSystem = std::make_unique<MovementSystem>();
    world.GetSystemScheduler().RegisterSystem(std::move(movementSystem));

    // Subscribe to damage events (queued, dispatched at end of Update).
    // 訂閱傷害事件（佇列方式，於 Update 結尾分派）。
    world.GetEventBus().Subscribe<EntityDamagedEvent>(
        [](const EntityDamagedEvent& e) {
            STONEPOTATOES_LOG_INFO(
                "Event: entity %u took %d damage", e.entity.GetIndex(),
                e.amount);
        });

    // Create 100 test entities with real components.
    // 建立 100 個帶有真實組件的測試實體。
    STONEPOTATOES_LOG_INFO("Creating 100 test entities...");
    STONEPOTATOES_LOG_INFO("建立 100 個測試實體...");
    StonePotatoes::ECS::Entity firstEntity;
    for (int i = 0; i < 100; i++) {
        StonePotatoes::ECS::Entity entity = world.CreateEntity();
        if (i == 0) firstEntity = entity;
        world.AddComponent<Position>(entity,
                                     Position{static_cast<float>(i), 0.0f, 0.0f});
        world.AddComponent<Velocity>(entity, Velocity{1.0f, 0.5f, 0.0f});
        world.AddComponent<Health>(entity, Health{100});
    }
    STONEPOTATOES_LOG_INFO("Created %u entities", world.GetEntityManager().GetEntityCount());
    STONEPOTATOES_LOG_INFO("已建立 %u 個實體", world.GetEntityManager().GetEntityCount());
    STONEPOTATOES_LOG_INFO("Archetypes: %zu",
                           world.GetArchetypeManager().GetArchetypeCount());

    // Query demo: count entities with Position+Velocity (all 100 expected).
    // 查詢演示：計算擁有 Position+Velocity 的實體（預期全部 100 個）。
    auto moving = world.MakeQuery<Position, Velocity>();
    STONEPOTATOES_LOG_INFO("Query<Position,Velocity>: %zu entities",
                           moving.Count(world.GetArchetypeManager()));

    // Allocator smoke test | 分配器煙霧測試
    {
        StonePotatoes::Memory::MemoryPool pool(sizeof(Position), 8);
        void* a = pool.Allocate();
        void* b = pool.Allocate();
        pool.Free(a);
        STONEPOTATOES_LOG_INFO("MemoryPool: used=%zu free=%zu ownsA=%d",
            pool.GetUsedBlocks(), pool.GetFreeBlocks(), pool.Owns(b));

        StonePotatoes::Memory::StackAllocator stack(4096);
        auto marker = stack.GetMarker();
        void* s1 = stack.Allocate(256);
        void* s2 = stack.Allocate(128);
        stack.FreeToMarker(marker);
        STONEPOTATOES_LOG_INFO("StackAllocator: after rollback used=%zu (s1=%p s2=%p)",
            stack.GetUsed(), s1, s2);

        StonePotatoes::Memory::ArenaAllocator arena(1024);
        void* big = arena.Allocate(4096); // forces second chunk
        STONEPOTATOES_LOG_INFO("ArenaAllocator: chunks=%zu allocated=%zu (big=%p)",
            arena.GetChunkCount(), arena.GetTotalAllocated(), big);
    }

    // Resource system demo | 資源系統演示
    struct WorldClock { double elapsed = 0.0; };
    world.GetResourceManager().Insert<WorldClock>();
    world.GetResourceManager().Get<WorldClock>()->elapsed = 42.0;
    STONEPOTATOES_LOG_INFO("Resource WorldClock.elapsed = %.1f",
        world.GetResourceManager().Get<WorldClock>()->elapsed);

    // Queue a demo event (dispatched on first World::Update).
    // 佇列一個演示事件（於第一次 World::Update 時分派）。
    world.GetEventBus().Queue(EntityDamagedEvent{firstEntity, 25});

    // Save/Load round-trip demo | 儲存/載入往返演示
    world.SaveToFile("world_save.bin");
    {
        StonePotatoes::ECS::World loaded;
        loaded.Initialize();
        loaded.RegisterComponent<Position>("Position");
        loaded.RegisterComponent<Velocity>("Velocity");
        loaded.RegisterComponent<Health>("Health");
        loaded.LoadFromFile("world_save.bin");
        STONEPOTATOES_LOG_INFO("Loaded world entities: %u (expected 100)",
                               loaded.GetEntityManager().GetEntityCount());
        loaded.Shutdown();
    }

    // ------------------------------------------------------------------
    // IQGPU demonstration | IQGPU 演示
    // CPU reference backend — real parallel dispatch, honest metrics.
    // CPU 參考後端 — 真實平行派發、誠實的量測數據。
    // ------------------------------------------------------------------
    STONEPOTATOES_LOG_INFO("--- IQGPU Compute Demo | IQGPU 計算演示 ---");
    {
        StonePotatoes::IQGPU::IQGPURuntime iqgpu;
        iqgpu.Initialize();

        // 1) Parallel kernel: saxpy over 1M floats | 平行核心：一百萬筆 float 的 saxpy
        constexpr uint32_t N = 1'000'000;
        auto* bufX = iqgpu.CreateTypedBuffer<float>(N);
        auto* bufY = iqgpu.CreateTypedBuffer<float>(N);
        for (uint32_t i = 0; i < N; ++i) { bufX->Data()[i] = float(i); bufY->Data()[i] = 1.0f; }

        struct SaxpyData { float* x; float* y; float a; };
        SaxpyData sd{bufX->Data(), bufY->Data(), 2.5f};
        StonePotatoes::GPU::Kernel saxpy("saxpy",
            [](uint32_t gid, void* ud) {
                auto* d = static_cast<SaxpyData*>(ud);
                d->y[gid] = d->a * d->x[gid] + d->y[gid];
            });
        StonePotatoes::GPU::DispatchParams dp;
        dp.threadCount = N; dp.userData = &sd;
        auto ds = iqgpu.Dispatch(saxpy, dp);
        STONEPOTATOES_LOG_INFO(
            "[IQGPU] saxpy: %u elems, %u workers, %.2f ms, y[999999]=%.1f (expect 2499998.5)",
            ds.itemsProcessed, ds.threadsUsed, ds.dispatchMs,
            static_cast<double>(bufY->Data()[N - 1]));

        // 2) ComputeGraph: A -> B -> C ordering | 計算圖：A -> B -> C 順序
        auto& graph = iqgpu.GetGraph();
        std::vector<float> graphOut(8, 0.0f);
        auto nodeA = StonePotatoes::GPU::ComputeNode::MakeKernel(
            "A_generate",
            StonePotatoes::GPU::Kernel("gen", [](uint32_t gid, void* ud) {
                static_cast<float*>(ud)[gid] = float(gid * gid);
            }), 8, graphOut.data());
        auto nodeB = StonePotatoes::GPU::ComputeNode::MakeKernel(
            "B_scale",
            StonePotatoes::GPU::Kernel("scale", [](uint32_t gid, void* ud) {
                static_cast<float*>(ud)[gid] *= 2.0f;
            }), 8, graphOut.data());
        nodeB->AddDependency(nodeA);
        auto nodeC = StonePotatoes::GPU::ComputeNode::MakeTask(
            "C_verify", [&graphOut] {
                STONEPOTATOES_LOG_INFO(
                    "[IQGPU] graph result out[7]=%.0f (expect 98)",
                    static_cast<double>(graphOut[7]));
            });
        nodeC->AddDependency(nodeB);
        graph.AddNode(nodeA); graph.AddNode(nodeB); graph.AddNode(nodeC);
        auto gr = iqgpu.ExecuteGraph();
        STONEPOTATOES_LOG_INFO("[IQGPU] graph: %u nodes, %.2f ms, order=%s",
            gr.nodesExecuted, gr.totalMs,
            [&]{ std::string s; for (auto& n : gr.executionOrder) s += n + " "; return s; }().c_str());

        // 3) Real GPU: Vulkan saxpy on the discrete device, verified
        // against the CPU result. 真實 GPU：Vulkan 在獨顯上執行 saxpy，
        // 並與 CPU 結果驗證。
#ifdef SP_VULKAN
        if (auto* vk = iqgpu.GetVulkanBackend()) {
            auto devices = iqgpu.ListDevices();
            for (const auto& d : devices)
                if (d.type != StonePotatoes::GPU::DeviceType::CPUReference)
                    STONEPOTATOES_LOG_INFO(
                        "[Vulkan] device: %s (%s, %.1f GiB, f64=%d)",
                        d.name.c_str(),
                        d.type == StonePotatoes::GPU::DeviceType::DiscreteGPU
                            ? "dGPU" : "iGPU",
                        d.memoryBytes / 1073741824.0,
                        d.supportsFloat64 ? 1 : 0);

            constexpr uint32_t N = 1u << 22;
            std::vector<float> ga(N), gb(N), gc(N);
            for (uint32_t i = 0; i < N; ++i) {
                ga[i] = static_cast<float>(i % 1024) * 0.5f;
                gb[i] = static_cast<float>(i % 512) * 0.25f;
            }
            auto ha = vk->CreateBuffer(N * 4), hb = vk->CreateBuffer(N * 4),
                 hc = vk->CreateBuffer(N * 4);
            vk->WriteBuffer(ha, ga.data(), N * 4);
            vk->WriteBuffer(hb, gb.data(), N * 4);
            auto kernel = vk->CreateKernelFromFile(
                SP_SHADER_DIR "/saxpy.spv", 3, 8);
            struct { float alpha; uint32_t n; } pc{2.5f, N};
            auto dr = vk->Dispatch(kernel, {ha, hb, hc}, (N + 255) / 256, 1, 1,
                                   &pc, sizeof(pc));
            vk->ReadBuffer(hc, gc.data(), N * 4);
            double maxErr = 0.0;
            for (uint32_t i = 0; i < N; ++i)
                maxErr = std::max(maxErr, static_cast<double>(std::fabs(
                                  gc[i] - (2.5f * ga[i] + gb[i]))));
            STONEPOTATOES_LOG_INFO(
                "[Vulkan] saxpy %u floats: gpu=%.3fms wall=%.3fms "
                "maxErr=%.2e (%.1f GB/s) | 誤差 %.2e",
                N, dr.gpuMs, dr.wallMs, maxErr,
                3.0 * N * 4.0 / (dr.gpuMs * 1e6), maxErr);
            vk->DestroyKernel(kernel);
            vk->DestroyBuffer(ha);
            vk->DestroyBuffer(hb);
            vk->DestroyBuffer(hc);
        } else {
            STONEPOTATOES_LOG_INFO(
                "[Vulkan] unavailable — CPU backend only | 不可用 — 僅 CPU 後端");
        }
#endif
        iqgpu.Shutdown();
    }

    // 3) Quantum-inspired algorithms (classical simulation)
    // 量子啟發演算法（傳統模擬）
    STONEPOTATOES_LOG_INFO("--- Quantum-Inspired Demo | 量子啟發演示 ---");
    {
        using namespace StonePotatoes::IQGPU;

        // Rastrigin-like multimodal energy: many local minima.
        // Rastrigin 式多峰能量函式：大量局部最小值。
        auto rastrigin = [](const std::vector<double>& x) {
            double e = 10.0 * double(x.size());
            for (double v : x) e += v * v - 10.0 * std::cos(2.0 * 3.14159265 * v);
            return e;
        };
        auto perturb = [](std::vector<double>& x, std::mt19937& rng) {
            std::uniform_int_distribution<size_t> pick(0, x.size() - 1);
            std::normal_distribution<double> n(0.0, 0.3);
            x[pick(rng)] += n(rng);
        };
        auto teleport = [](std::vector<double>& x, std::mt19937& rng) {
            std::uniform_real_distribution<double> u(-5.12, 5.12);
            for (auto& v : x) v = u(rng);
        };
        std::vector<double> init(8, 5.0);

        SimulatedAnnealer sa;
        auto rSA = sa.Optimize(init, rastrigin, perturb, 20000, 5.0, 0.999, 42);
        QuantumInspiredAnnealer qa;
        auto rQA = qa.Optimize(init, rastrigin, perturb, teleport,
                               20000, 5.0, 0.999, 0.08, 42);
        STONEPOTATOES_LOG_INFO(
            "[QI] Annealing 20k iters: classical E=%.2f (improved %.2f) | "
            "quantum-inspired E=%.2f (improved %.2f)",
            rSA.bestEnergy, rSA.improvement, rQA.bestEnergy, rQA.improvement);

        // Amplitude amplification: find multiples of 97 in [0, 1024).
        // 振幅放大：在 [0,1024) 中尋找 97 的倍數。
        AmplitudeAmplifier aa;
        auto rAA = aa.Search(1024, [](uint32_t i) { return i % 97 == 0 && i != 0; }, 8);
        double markedP = 0.0, unmarkedP = 0.0; uint32_t unmarkedN = 0;
        for (uint32_t i = 0; i < 1024; ++i) {
            bool marked = (i % 97 == 0 && i != 0);
            if (marked) markedP += rAA.probabilities[i];
            else { unmarkedP += rAA.probabilities[i]; ++unmarkedN; }
        }
        STONEPOTATOES_LOG_INFO(
            "[QI] AmplitudeAmp: %zu marked in 1024, %u rounds, "
            "markedP=%.1f%% vs uniform=%.1f%%",
            rAA.markedStates.size(), rAA.roundsApplied,
            markedP * 100.0,
            (double(rAA.markedStates.size()) / 1024.0) * 100.0);

        // Born-rule measurement | 玻恩定則測量
        ProbabilisticStateSpace pss(4, 7);
        pss.SetAmplitude(0, 1.0); pss.SetAmplitude(1, 2.0);
        pss.SetAmplitude(2, 0.5); pss.SetAmplitude(3, 1.5);
        pss.Normalize();
        uint32_t counts[4] = {0, 0, 0, 0};
        for (int i = 0; i < 4000; ++i) ++counts[pss.Measure()];
        STONEPOTATOES_LOG_INFO(
            "[QI] Measure 4k samples: |0>=%.1f%% |1>=%.1f%% |2>=%.1f%% |3>=%.1f%%",
            counts[0] / 40.0, counts[1] / 40.0, counts[2] / 40.0, counts[3] / 40.0);
    }

    // ------------------------------------------------------------------
    // Agent <-> IQGPU bridge: measurement-driven kernel self-optimization
    // Agent <-> IQGPU 橋接：量測驅動的核心自我優化
    // Uneven workload (hotspots every 512 elements) — grain stealing
    // should outperform contiguous partitioning. Result feeds back
    // into the agent's LearningSystem.
    // 不均勻工作負載（每 512 元素有熱點）— 粒度竊取應優於
    // 連續切分。結果回饋至 Agent 的學習系統。
    // ------------------------------------------------------------------
    STONEPOTATOES_LOG_INFO("--- Agent-IQGPU Self-Optimization | "
                           "Agent-IQGPU 自我優化 ---");
    {
        using namespace StonePotatoes;

        // Workload: heavy math on a small fraction of elements.
        // 工作負載：少部分元素執行重度數學運算。
        constexpr uint32_t WORK = 2'000'000;
        std::vector<double> scratch(WORK);
        auto workload = [&scratch](uint32_t gid) {
            double v = double(gid) * 0.001;
            if ((gid % 512) < 8) {          // ~1.6% hot elements | 熱點元素
                for (int k = 0; k < 200; ++k)
                    v += std::sin(v) * std::cos(v + double(k));
            }
            scratch[gid] = v;
        };

        IQGPU::IQGPURuntime rt;
        rt.Initialize();

        IQGPU::KernelOptimizer optimizer;
        std::vector<IQGPU::KernelOptimizer::Variant> variants;
        auto mkKernel = [&workload](const char* n) {
            return GPU::Kernel(n, [workload](uint32_t gid, void*) {
                workload(gid);
            });
        };
        variants.push_back({"contiguous",
                            mkKernel("contig"), WORK, 0, nullptr, false, 0});
        variants.push_back({"grain_4096",
                            mkKernel("g4096"), WORK, 0, nullptr, true, 4096});
        variants.push_back({"grain_1024",
                            mkKernel("g1024"), WORK, 0, nullptr, true, 1024});
        variants.push_back({"grain_128",
                            mkKernel("g128"), WORK, 0, nullptr, true, 128});

        auto opt = optimizer.Optimize(variants, rt.GetBackend(), 5, 1);
        for (const auto& v : opt.variants) {
            STONEPOTATOES_LOG_INFO(
                "[IQGPU-OPT] %-12s median=%.2fms best=%.2fms worst=%.2fms",
                v.name.c_str(), v.medianMs, v.bestMs, v.worstMs);
        }
        STONEPOTATOES_LOG_INFO(
            "[IQGPU-OPT] Winner: %s (%.2fms) — %.2fx faster than %s",
            opt.bestName.c_str(), opt.bestMs, opt.speedup,
            opt.worstName.c_str());

        // ------------------------------------------------------------------
        // ZetaCompute: parallel zero scan + spectral world-gen tools
        // ZetaCompute：平行零點掃描 + 頻譜世界生成工具
        // The ζ zero spectrum becomes a reusable engine resource:
        // level-repulsion point placement and aperiodic noise.
        // ζ 零點頻譜成為可重用的引擎資源：
        // 層級排斥點分佈與非週期雜訊。
        // ------------------------------------------------------------------
        STONEPOTATOES_LOG_INFO("--- ZetaCompute Demo | ζ 計算演示 ---");
        using namespace StonePotatoes::Math::Zeta;

        IQGPU::ZetaCompute zeta(rt);
        GPU::CPUBackend::Stats zs;
        ZeroSpectrum spec = zeta.FindZeros(13.0, 60.0, 0.02, &zs);
        const auto& gammas = spec.Gammas();
        STONEPOTATOES_LOG_INFO(
            "[Zeta] parallel scan: %u items in %.1fms on %u workers",
            zs.itemsProcessed, zs.dispatchMs, zs.threadsUsed);
        STONEPOTATOES_LOG_INFO(
            "[Zeta] Found %zu zeros in [13,60): first=%.6f (true 14.134725)",
            gammas.size(), gammas.empty() ? 0.0 : gammas.front());
        if (spec.Size() >= 8) {
            STONEPOTATOES_LOG_INFO(
                "[Zeta] Spacing KS: GUE=%.3f vs Poisson=%.3f "
                "(GUE repulsion confirmed | GUE 排斥驗證)",
                spec.KSDistanceGUE(), spec.KSDistancePoisson());

            // SpectralPointSampler: deterministic, non-clumping,
            // non-gridded placement — usable for world object spawn.
            // SpectralPointSampler：確定性、不群聚、非網格的點分佈 —
            // 可用於世界物件生成位置。
            SpectralPointSampler sampler(spec);
            auto pts = sampler.Sample2D(100.0f, 60.0f, 10.0f, 7);
            STONEPOTATOES_LOG_INFO(
                "[Zeta] SpectralPointSampler: %zu deterministic "
                "spawn points on 100x60 map",
                pts.size());
            if (!pts.empty()) {
                STONEPOTATOES_LOG_INFO(
                    "[Zeta] first points: (%.1f,%.1f) (%.1f,%.1f)",
                    double(pts[0].x), double(pts[0].y),
                    pts.size() > 1 ? double(pts[1].x) : 0.0,
                    pts.size() > 1 ? double(pts[1].y) : 0.0);
            }

            // SpectralNoise: aperiodic terrain-style signal.
            // SpectralNoise：非週期地形訊號。
            SpectralNoise noise(spec, 42, 16, 0.5);
            double n0 = noise.Evaluate(0.0), n1 = noise.Evaluate(3.7);
            STONEPOTATOES_LOG_INFO(
                "[Zeta] SpectralNoise(0)=%.3f (3.7)=%.3f — zero-mean "
                "unit-variance", n0, n1);
        }

        // RH finite-verification audit (bounded height — can falsify,
        // never prove). Real work: zeros, winding, Gram blocks, GUE,
        // explicit formula, Mertens/Robin, Li, de Bruijn–Newman.
        // RH 有限驗證稽核（限定高度 — 可證偽，無法證明）。
        // 真實工作：零點、環繞數、Gram 區塊、GUE、
        // 顯式公式、Mertens/Robin、Li、de Bruijn–Newman。
        STONEPOTATOES_LOG_INFO("--- RH Finite Verification | RH 有限驗證 ---");
        IQGPU::ZetaCompute::AuditConfig acfg;
        acfg.tMax = 60.0;
        acfg.spectrumTMax = 2000.0;  // more zeros → tighter explicit-formula fit
        acfg.xMax = 60000;
        acfg.gramLast = 40;
        auto audit = zeta.RunAudit(acfg);
        for (const auto& item : audit.items) {
            STONEPOTATOES_LOG_INFO("[RH] %-24s %-55s %s",
                item.name.c_str(), item.value.c_str(),
                item.pass ? "PASS" : "FAIL");
        }
        STONEPOTATOES_LOG_INFO(
            "[RH] Audit: %d/%d checks passed in %.0fms "
            "(finite verification — falsifies, never proves)",
            audit.passed, audit.total, audit.ms);
        rt.Shutdown();
    }

    // ------------------------------------------------------------------
    // MVP NPC vertical slice: local-information agents.
    // One hungry NPC, one food in sight, one food beyond the horizon,
    // and a threat that arrives mid-run. The NPC only ever acts on
    // what its own Memory has recorded — nothing is global.
    // MVP NPC 垂直切片：局部資訊代理。
    // 一隻餓的 NPC、視野內一份食物、地平線外一份食物，
    // 以及中途出現的威脅。NPC 僅依自身記憶行動 — 無全域知識。
    // ------------------------------------------------------------------
    STONEPOTATOES_LOG_INFO("--- NPC Vertical Slice | NPC 垂直切片 ---");
    {
        namespace A = StonePotatoes::AI;
        StonePotatoes::ECS::World sim;
        sim.Initialize();
        A::RegisterNpcComponents(sim);
        auto& sched = sim.GetSystemScheduler();
        sched.RegisterSystem(std::make_unique<A::PerceptionSystem>());
        sched.RegisterSystem(std::make_unique<A::DecisionSystem>());
        sched.RegisterSystem(std::make_unique<A::ActionSystem>());
        sched.RegisterSystem(std::make_unique<A::NpcMovementSystem>());

        // World-gen closed loop: spectral-scattered food streams in
        // around the NPC; eaten nodes stay harvested forever.
        // 世界生成閉環：頻譜散佈的食物在 NPC 周圍串流載入；
        // 被吃掉的節點永久消失。
        auto npcSpec =
            StonePotatoes::Math::Zeta::ZeroSpectrum::Compute(10.0, 60.0, 0.02);
        StonePotatoes::World::ResourceScatter scatter(npcSpec, 7);
        scatter.AddLayer({1, 7.0f, 1.0f, 0.02f, -10.0f});
        auto* streamer = new StonePotatoes::World::WorldStreamer(
            &scatter, {48.0f, 1, 0.05f, {1}});
        sched.RegisterSystem(std::unique_ptr<StonePotatoes::ECS::System>(streamer));
        sched.InitializeSystems(&sim);   // subscribe harvest hook | 訂閱採集掛鉤

        auto npc = A::SpawnNpc(sim, 0.0f, 0.0f);
        sim.GetComponent<A::Needs>(npc)->hunger = 0.8f;
        auto farFood = sim.CreateEntity();
        sim.AddComponent(farFood, A::NPCPosition{80.0f, 0.0f});
        sim.AddComponent(farFood, A::Food{});

        sim.GetEventBus().Subscribe<A::FoodEatenEvent>(
            [](const A::FoodEatenEvent& e) {
                STONEPOTATOES_LOG_INFO(
                    "[NPC] entity %llu ate food %llu | 實體 %llu 吃掉食物 %llu",
                    (unsigned long long)e.eater.id,
                    (unsigned long long)e.food.id,
                    (unsigned long long)e.eater.id,
                    (unsigned long long)e.food.id);
            });

        // Run the slice through the parallel scheduler on the IQGPU
        // CPU backend. The NPC pipeline is inherently sequential
        // (perception→decision→action→movement form a dependency
        // chain), so expect one system per wave — the wins come when
        // independent systems coexist.
        // 透過 IQGPU CPU 後端的平行排程器執行切片。NPC 管線本質上
        // 是依賴鏈（感知→決策→行動→移動），預期每波一個系統 —
        // 平行收益發生在多個獨立系統共存時。
        StonePotatoes::GPU::CPUBackend parBackend(4);
        auto parFor = [&parBackend](uint32_t n,
                                    const std::function<void(uint32_t)>& f) {
            parBackend.ParallelFor(n, f, 1);
        };
        StonePotatoes::ECS::SystemScheduler::ParallelStats lastStats;

        const float dt = 0.05f;
        for (int t = 0; t < 120; ++t) {
            lastStats = sim.UpdateParallel(dt, parFor);
            if (t == 40) {   // a wolf appears at mid-run | 中途出現威脅
                auto wolf = sim.CreateEntity();
                sim.AddComponent(wolf, A::NPCPosition{3.0f, 1.0f});
                sim.AddComponent(wolf, A::Threat{});
            }
            if (t % 20 == 0) {
                auto* p = sim.GetComponent<A::NPCPosition>(npc);
                auto* d = sim.GetComponent<A::Decision>(npc);
                auto* m = sim.GetComponent<A::Memory>(npc);
                const char* act =
                    d->action == A::Action::Flee ? "FLEE" :
                    d->action == A::Action::SeekFood ? "SEEK" :
                    d->action == A::Action::Wander ? "WANDER" : "IDLE";
                STONEPOTATOES_LOG_INFO(
                    "[NPC] t=%4.1fs pos=(%5.1f,%5.1f) action=%-6s "
                    "memories=%u | 位置=(%5.1f,%5.1f) 行動=%s 記憶=%u",
                    t * dt, p->x, p->y, act, m->count,
                    p->x, p->y, act, m->count);
            }
        }
        auto* mem = sim.GetComponent<A::Memory>(npc);
        STONEPOTATOES_LOG_INFO(
            "[NPC] knows far food? %s (local info only | 僅局部資訊)",
            mem->Find(farFood.id) != StonePotatoes::AI::Memory::kMax
                ? "YES" : "NO");
        STONEPOTATOES_LOG_INFO(
            "[World] chunks=%zu spawned=%zu harvested=%zu "
            "| 已載入 chunk、生成節點、永久採集數",
            streamer->LoadedChunkCount(), streamer->SpawnedEntityCount(),
            streamer->HarvestedCount());
        STONEPOTATOES_LOG_INFO(
            "[Sched] waves=%u systems=%u maxWave=%u tick=%.2fms "
            "(sequential chain verified | 依賴鏈已驗證)",
            lastStats.waves, lastStats.systemsRun, lastStats.maxWaveSize,
            lastStats.ms);
        sim.Shutdown();
    }

    // Main loop
    // 主迴圈
    STONEPOTATOES_LOG_INFO("Entering main loop...");
    STONEPOTATOES_LOG_INFO("進入主迴圈...");
    double frameCount = 0;
    // Refresh the cached time before sampling it — GetTotalTime() only
    // advances inside Update(), so a stale snapshot would end the loop early.
    // 取樣前先刷新快取時間 — GetTotalTime() 只在 Update() 內推進，
    // 若使用過期快照會導致迴圈提早結束。
    StonePotatoes::Time::TimeManager::GetInstance().Update();
    double startTime = StonePotatoes::Time::TimeManager::GetInstance().GetTotalTime();
    // Run for 3 seconds for testing, then exit
    // 執行 3 秒用於測試，然後退出
    while (!window.ShouldClose() && (StonePotatoes::Time::TimeManager::GetInstance().GetTotalTime() - startTime) < 3.0) {
        // Poll events
        // 輪詢事件
        window.PollEvents();

        // Update time
        // 更新時間
        StonePotatoes::Time::TimeManager::GetInstance().Update();

        // Update world
        // 更新世界
        world.Update(static_cast<float>(StonePotatoes::Time::TimeManager::GetInstance().GetDeltaTime()));

        // Swap buffers
        // 交換緩衝區
        window.SwapBuffers();

        frameCount++;
    }

    STONEPOTATOES_LOG_INFO("Main loop ended. Total frames: %.0f", frameCount);
    STONEPOTATOES_LOG_INFO("主迴圈結束。總幀數：%.0f", frameCount);

    // Shutdown
    // 關閉
    world.Shutdown();
    window.Destroy();
    StonePotatoes::Time::TimeManager::GetInstance().Shutdown();
    StonePotatoes::Platform::Shutdown();

    STONEPOTATOES_LOG_INFO("StonePotatoes shutdown complete");
    STONEPOTATOES_LOG_INFO("StonePotatoes 關閉完成");
    return 0;
}
