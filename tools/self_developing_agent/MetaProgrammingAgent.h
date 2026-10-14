#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <chrono>

namespace StonePotatoes {
namespace SelfDevelopingAgent {

// ============================================================================
// Forward Declarations | 前置宣告
// ============================================================================
class CodeGenerator;
class CodeParser;
class CodeMutator;
class SelfTester;
class SelfOptimizer;
class LearningSystem;
class SandboxEnvironment;

// ============================================================================
// MetaProgrammingAgent - Orchestrates the self-development cycle
// MetaProgrammingAgent - 協調自我開發週期
// ============================================================================
class MetaProgrammingAgent {
public:
    MetaProgrammingAgent();
    ~MetaProgrammingAgent();

    // Lifecycle | 生命週期
    void Initialize();
    void Shutdown();
    bool IsInitialized() const { return m_IsInitialized; }

    // Development | 開發
    void SetDevelopmentGoal(const std::string& goal);
    const std::string& GetDevelopmentGoal() const { return m_DevelopmentGoal; }

    /**
     * @brief Run a full development cycle:
     *        Analyze -> Generate -> Write -> Compile -> Test -> Optimize -> Learn
     * @brief 執行完整開發週期：
     *        分析 -> 生成 -> 寫入 -> 編譯 -> 測試 -> 優化 -> 學習
     * @return true if the cycle completed without critical failure
     * @return 若週期無嚴重失敗則回傳 true
     */
    bool DevelopmentCycle();

    // Statistics | 統計
    struct Statistics {
        int codeGeneratedLines = 0;
        int testsGenerated = 0;
        int bugsFound = 0;
        int optimizationsPerformed = 0;
        int cyclesCompleted = 0;
        int compileSuccesses = 0;
        int compileFailures = 0;
        float selfImprovementScore = 0.0f;
    };
    const Statistics& GetStatistics() const { return m_Statistics; }

    // Subsystem access | 子系統存取
    CodeGenerator&   GetCodeGenerator()   { return *m_CodeGenerator; }
    CodeParser&      GetCodeParser()      { return *m_CodeParser; }
    CodeMutator&     GetCodeMutator()     { return *m_CodeMutator; }
    SelfTester&      GetSelfTester()      { return *m_SelfTester; }
    SelfOptimizer&   GetSelfOptimizer()   { return *m_SelfOptimizer; }
    LearningSystem&  GetLearningSystem()  { return *m_LearningSystem; }
    SandboxEnvironment& GetSandbox()      { return *m_Sandbox; }

    /**
     * @brief Set the working directory where generated code is written
     * @brief 設置生成程式碼的寫入目錄
     */
    void SetWorkspaceDirectory(const std::string& dir);
    const std::string& GetWorkspaceDirectory() const { return m_WorkspaceDir; }

private:
    // Cycle phases | 週期階段
    std::string AnalyzeGoal();
    std::string GenerateCode(const std::string& spec);
    std::string WriteToSandbox(const std::string& filename, const std::string& content);
    bool CompileAndTest(const std::string& sourceFile);
    void Optimize(const std::string& sourceFile);
    void Learn(bool success);

    std::unique_ptr<CodeGenerator>   m_CodeGenerator;
    std::unique_ptr<CodeParser>      m_CodeParser;
    std::unique_ptr<CodeMutator>     m_CodeMutator;
    std::unique_ptr<SelfTester>      m_SelfTester;
    std::unique_ptr<SelfOptimizer>   m_SelfOptimizer;
    std::unique_ptr<LearningSystem>  m_LearningSystem;
    std::unique_ptr<SandboxEnvironment> m_Sandbox;

    std::string m_DevelopmentGoal;
    std::string m_WorkspaceDir;
    Statistics  m_Statistics;
    bool        m_IsInitialized = false;
};

// ============================================================================
// CodeGenerator - Generates compilable C++ source from specifications
// CodeGenerator - 從規格生成可編譯的 C++ 原始碼
// ============================================================================
class CodeGenerator {
public:
    struct GenerationRequest {
        std::string functionName;
        std::string description;
        std::vector<std::string> includes;
        std::string returnType = "void";
        std::vector<std::string> parameters;
        std::vector<std::string> bodyLines;
    };

    /**
     * @brief Generate a complete .cpp file with a main() entry point
     * @brief 生成含 main() 入口點的完整 .cpp 檔案
     */
    std::string GenerateCXX(const GenerationRequest& req);

    /**
     * @brief Generate a unit-test .cpp that exercises the target function
     * @brief 生成針對目標函式的單元測試 .cpp
     */
    std::string GenerateTest(const std::string& functionName,
                             const std::string& bodyToTest);

    /**
     * @brief Generate an HLSL compute shader skeleton
     * @brief 生成 HLSL 計算著色器骨架
     */
    std::string GenerateComputeShader(const std::string& kernelName,
                                      int threadGroupSizeX = 64);

    /**
     * @brief Parse a goal string into a GenerationRequest
     * @brief 將目標字串解析為 GenerationRequest
     */
    GenerationRequest ParseGoal(const std::string& goal);

private:
    std::string MakeIdentifier(const std::string& input);
};

// ============================================================================
// CodeParser - Lightweight static analysis of C++ source text
// CodeParser - C++ 原始碼文字的輕量級靜態分析
// ============================================================================
class CodeParser {
public:
    struct Analysis {
        int totalLines = 0;
        int codeLines = 0;
        int commentLines = 0;
        int blankLines = 0;
        int functionCount = 0;
        int classCount = 0;
        int todoCount = 0;
        int fixmeCount = 0;
        int loopCount = 0;
        int branchCount = 0;
        int allocationCount = 0;   // new/malloc usage
        int lambdaCount = 0;
        std::vector<std::string> includes;
        std::vector<std::string> warnings;
    };

    Analysis Analyze(const std::string& code);

    /**
     * @brief Extract function signatures using simple pattern matching
     * @brief 使用簡單模式匹配擷取函式簽章
     */
    std::vector<std::string> ExtractFunctionSignatures(const std::string& code);
};

// ============================================================================
// CodeMutator - Transforms source code
// CodeMutator - 轉換原始碼
// ============================================================================
class CodeMutator {
public:
    enum class MutationType {
        RemoveComments,
        AddTimingInstrumentation,
        ReplaceMallocWithVector,
        AddConstToReadOnlyParams,
        ReserveVectorCapacity
    };

    std::string Mutate(const std::string& code, MutationType type);
    std::vector<MutationType> SuggestMutations(const CodeParser::Analysis& analysis);
};

// ============================================================================
// SelfTester - Compiles and runs generated code, captures results
// SelfTester - 編譯並執行生成的程式碼，擷取結果
// ============================================================================
class SelfTester {
public:
    struct TestResult {
        bool   compiled = false;
        bool   executed = false;
        bool   passed = false;
        int    exitCode = -1;
        double compileTimeMs = 0.0;
        double runTimeMs = 0.0;
        std::string compileOutput;
        std::string runOutput;
        std::vector<std::string> errors;
    };

    /**
     * @brief Compile a .cpp file with the system compiler
     * @brief 使用系統編譯器編譯 .cpp 檔案
     * @param sourceFile path to .cpp
     * @param sourceFile .cpp 檔案路徑
     * @param outputExe path for output executable
     * @param outputExe 輸出執行檔路徑
     */
    TestResult CompileAndRun(const std::string& sourceFile,
                             const std::string& outputExe);

    /**
     * @brief Static analysis bug detection (real checks on source text)
     * @brief 靜態分析錯誤檢測（對原始碼文字的真實檢查）
     */
    std::vector<std::string> DetectBugs(const std::string& code);

    void SetCompilerPath(const std::string& compiler) { m_Compiler = compiler; }
    void SetCompileTimeoutSeconds(int seconds) { m_CompileTimeoutSec = seconds; }

private:
    std::string m_Compiler;              // auto-detected
    int m_CompileTimeoutSec = 60;
    int m_RunTimeoutSec = 30;
};

// ============================================================================
// SelfOptimizer - Analyzes and suggests/applies optimizations
// SelfOptimizer - 分析並建議/應用優化
// ============================================================================
class SelfOptimizer {
public:
    struct Suggestion {
        enum class Severity { Info, Warning, Critical };
        Severity severity;
        std::string category;      // e.g. "memory", "algorithm", "concurrency"
        std::string description;
        int lineHint = -1;         // approximate line, -1 if unknown
    };

    std::vector<Suggestion> Analyze(const std::string& code);
    std::string ApplySuggestion(const std::string& code, const Suggestion& suggestion);

    struct OptimizationSummary {
        int suggestionsFound = 0;
        int applied = 0;
        std::vector<Suggestion> suggestions;
    };
    OptimizationSummary AutoOptimize(const std::string& code);
};

// ============================================================================
// LearningSystem - Persists experience records to disk
// LearningSystem - 將經驗記錄持久化到磁碟
// ============================================================================
class LearningSystem {
public:
    struct Experience {
        std::string goal;
        std::string patternUsed;
        bool   success;
        double score;              // 0.0 - 1.0
        std::string timestamp;     // ISO-8601
    };

    explicit LearningSystem(const std::string& storagePath = "agent_experience.jsonl");
    ~LearningSystem();

    void RecordSuccess(const std::string& pattern, const std::string& context, double score = 1.0);
    void RecordFailure(const std::string& pattern, const std::string& context, double score = 0.0);

    /**
     * @brief Query best known pattern for a goal keyword
     * @brief 查詢目標關鍵字的最佳已知模式
     */
    std::string SuggestApproach(const std::string& goal) const;

    int TotalRecords() const { return static_cast<int>(m_Records.size()); }
    double SuccessRate() const;

    bool Load();
    bool Save() const;

private:
    std::string m_StoragePath;
    std::vector<Experience> m_Records;
};

// ============================================================================
// SandboxEnvironment - Executes binaries with resource limits
// SandboxEnvironment - 以資源限制執行二進位檔案
// ============================================================================
class SandboxEnvironment {
public:
    struct ExecutionResult {
        bool   success = false;
        bool   timedOut = false;
        int    exitCode = -1;
        double elapsedMs = 0.0;
        std::string stdOut;
        std::string stdErr;
    };

    /**
     * @brief Run an executable, capture stdout/stderr, enforce timeout
     * @brief 執行可執行檔，擷取 stdout/stderr，強制逾時
     */
    ExecutionResult Execute(const std::string& executablePath,
                            const std::vector<std::string>& args = {},
                            int timeoutSeconds = 30);

    void SetResourceLimits(double maxCpuSeconds, double maxMemoryMB);
    void SetWorkingDirectory(const std::string& dir);

private:
    double m_MaxCpuSeconds = 30.0;
    double m_MaxMemoryMB = 1024.0;
    std::string m_WorkingDir;
};

} // namespace SelfDevelopingAgent
} // namespace StonePotatoes
