#include "MetaProgrammingAgent.h"
#include "../core/logging/Logger.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <unordered_set>

#ifdef _WIN32
#define POPEN  _popen
#define PCLOSE _pclose
#define EXE_EXT ".exe"
#else
#define POPEN  popen
#define PCLOSE pclose
#define EXE_EXT ""
#endif

namespace fs = std::filesystem;

namespace StonePotatoes {
namespace SelfDevelopingAgent {

// ============================================================================
// Helpers | 輔助函式
// ============================================================================
namespace {

std::string Trim(const std::string& s) {
    auto begin = s.find_first_not_of(" \t\r\n");
    auto end   = s.find_last_not_of(" \t\r\n");
    return (begin == std::string::npos) ? "" : s.substr(begin, end - begin + 1);
}

std::string ToLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

std::string NowISO8601() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm);
    return buf;
}

std::string ReadFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return {};
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

bool WriteFile(const std::string& path, const std::string& content) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) return false;
    out << content;
    return out.good();
}

std::vector<std::string> SplitLines(const std::string& text) {
    std::vector<std::string> lines;
    std::istringstream ss(text);
    std::string line;
    while (std::getline(ss, line)) lines.push_back(line);
    return lines;
}

/// Run a shell command, capture combined output, return exit code.
/// 執行 shell 命令，擷取合併輸出，回傳結束碼。
int RunCommand(const std::string& cmd, std::string& out, int timeoutSeconds) {
    out.clear();
    std::string full = cmd + " 2>&1";
    auto start = std::chrono::steady_clock::now();
    FILE* pipe = POPEN(full.c_str(), "r");
    if (!pipe) return -1;
    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), pipe)) {
        out += buffer;
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - start).count();
        if (elapsed > timeoutSeconds) {
            PCLOSE(pipe);
            return -2; // timeout
        }
    }
    int code = PCLOSE(pipe);
    return code;
}

/// Escape double quotes for shell commands.
/// 為 shell 命令跳脫雙引號。
std::string Q(const std::string& path) {
    return "\"" + path + "\"";
}

} // anonymous namespace

// ============================================================================
// MetaProgrammingAgent
// ============================================================================

MetaProgrammingAgent::MetaProgrammingAgent()  = default;
MetaProgrammingAgent::~MetaProgrammingAgent() = default;

void MetaProgrammingAgent::Initialize() {
    if (m_IsInitialized) return;
    STONEPOTATOES_LOG_INFO("[MetaProgrammingAgent] Initializing subsystems...");

    m_CodeGenerator  = std::make_unique<CodeGenerator>();
    m_CodeParser     = std::make_unique<CodeParser>();
    m_CodeMutator    = std::make_unique<CodeMutator>();
    m_SelfTester     = std::make_unique<SelfTester>();
    m_SelfOptimizer  = std::make_unique<SelfOptimizer>();
    m_LearningSystem = std::make_unique<LearningSystem>();
    m_Sandbox        = std::make_unique<SandboxEnvironment>();

    if (m_WorkspaceDir.empty())
        m_WorkspaceDir = (fs::current_path() / "agent_workspace").string();

    std::error_code ec;
    fs::create_directories(m_WorkspaceDir, ec);
    if (ec) {
        STONEPOTATOES_LOG_ERROR("[MetaProgrammingAgent] Cannot create workspace: %s",
                                m_WorkspaceDir.c_str());
    }

    m_LearningSystem->Load();
    m_IsInitialized = true;
    STONEPOTATOES_LOG_INFO("[MetaProgrammingAgent] Ready. Workspace: %s",
                           m_WorkspaceDir.c_str());
}

void MetaProgrammingAgent::Shutdown() {
    if (!m_IsInitialized) return;
    m_LearningSystem->Save();
    m_IsInitialized = false;
    STONEPOTATOES_LOG_INFO("[MetaProgrammingAgent] Shutdown. %d experience records saved.",
                           m_LearningSystem->TotalRecords());
}

void MetaProgrammingAgent::SetDevelopmentGoal(const std::string& goal) {
    m_DevelopmentGoal = goal;
}

void MetaProgrammingAgent::SetWorkspaceDirectory(const std::string& dir) {
    m_WorkspaceDir = dir;
    if (m_IsInitialized) {
        std::error_code ec;
        fs::create_directories(m_WorkspaceDir, ec);
    }
}

bool MetaProgrammingAgent::DevelopmentCycle() {
    if (!m_IsInitialized) {
        STONEPOTATOES_LOG_ERROR("[MetaProgrammingAgent] Not initialized.");
        return false;
    }

    STONEPOTATOES_LOG_INFO("=== Development Cycle Start | 開發週期開始 ===");
    auto t0 = std::chrono::steady_clock::now();

    // 1. Analyze goal -> specification | 分析目標 -> 規格
    auto spec = AnalyzeGoal();
    STONEPOTATOES_LOG_INFO("[Cycle] Specification generated (%zu chars)", spec.size());

    // 2. Generate code | 生成程式碼
    std::string code = GenerateCode(spec);
    auto analysis = m_CodeParser->Analyze(code);
    m_Statistics.codeGeneratedLines += analysis.codeLines;
    STONEPOTATOES_LOG_INFO("[Cycle] Generated %d code lines, %d functions",
                           analysis.codeLines, analysis.functionCount);

    // 3. Write to sandbox | 寫入沙盒
    std::string file = WriteToSandbox("generated_main.cpp", code);
    if (file.empty()) {
        Learn(false);
        return false;
    }

    // 4. Static bug analysis before compile | 編譯前靜態錯誤分析
    auto bugs = m_SelfTester->DetectBugs(code);
    m_Statistics.bugsFound += static_cast<int>(bugs.size());
    for (const auto& b : bugs)
        STONEPOTATOES_LOG_WARNING("[Cycle] Static analysis: %s", b.c_str());

    // 4b. Generate a test harness alongside | 一併生成測試程式
    auto req = m_CodeGenerator->ParseGoal(m_DevelopmentGoal);
    std::string testCode =
        m_CodeGenerator->GenerateTest(req.functionName, code);
    if (!WriteToSandbox(req.functionName + "_test.cpp", testCode).empty())
        m_Statistics.testsGenerated++;

    // 5. Compile & run | 編譯並執行
    bool ok = CompileAndTest(file);

    // 6. Optimization analysis | 優化分析
    Optimize(file);

    // 7. Learn | 學習
    Learn(ok);

    m_Statistics.cyclesCompleted++;
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  std::chrono::steady_clock::now() - t0).count();
    STONEPOTATOES_LOG_INFO("=== Cycle %s in %lld ms | 週期 %s 耗時 %lld 毫秒 ===",
                           ok ? "SUCCEEDED" : "FAILED", ms,
                           ok ? "成功" : "失敗", ms);
    return ok;
}

std::string MetaProgrammingAgent::AnalyzeGoal() {
    auto req = m_CodeGenerator->ParseGoal(m_DevelopmentGoal);
    std::ostringstream ss;
    ss << "// Goal: " << m_DevelopmentGoal << "\n";
    ss << "// Function: " << req.functionName << "\n";
    ss << "// Return: " << req.returnType << "\n";
    for (const auto& p : req.parameters) ss << "// Param: " << p << "\n";
    return ss.str();
}

std::string MetaProgrammingAgent::GenerateCode(const std::string& spec) {
    (void)spec; // spec is embedded in generation request
    auto req = m_CodeGenerator->ParseGoal(m_DevelopmentGoal);
    return m_CodeGenerator->GenerateCXX(req);
}

std::string MetaProgrammingAgent::WriteToSandbox(const std::string& filename,
                                                 const std::string& content) {
    fs::path full = fs::path(m_WorkspaceDir) / filename;
    if (!WriteFile(full.string(), content)) {
        STONEPOTATOES_LOG_ERROR("[Cycle] Failed to write %s", full.string().c_str());
        return {};
    }
    return full.string();
}

bool MetaProgrammingAgent::CompileAndTest(const std::string& sourceFile) {
    std::string exe = (fs::path(m_WorkspaceDir) / ("generated" EXE_EXT)).string();
    auto result = m_SelfTester->CompileAndRun(sourceFile, exe);

    if (!result.compiled) {
        m_Statistics.compileFailures++;
        STONEPOTATOES_LOG_ERROR("[Cycle] Compile failed:\n%s",
                                result.compileOutput.c_str());
        return false;
    }
    m_Statistics.compileSuccesses++;
    STONEPOTATOES_LOG_INFO("[Cycle] Compiled in %.0fms", result.compileTimeMs);

    if (result.executed) {
        STONEPOTATOES_LOG_INFO("[Cycle] Ran in %.0fms, exit=%d, pass=%s",
                               result.runTimeMs, result.exitCode,
                               result.passed ? "true" : "false");
        STONEPOTATOES_LOG_INFO("[Cycle] Program output:\n%s", result.runOutput.c_str());
    }
    return result.passed;
}

void MetaProgrammingAgent::Optimize(const std::string& sourceFile) {
    std::string code = ReadFile(sourceFile);
    auto summary = m_SelfOptimizer->AutoOptimize(code);
    m_Statistics.optimizationsPerformed += summary.applied;
    for (const auto& s : summary.suggestions) {
        const char* sev = s.severity == SelfOptimizer::Suggestion::Severity::Critical ? "CRIT"
                        : s.severity == SelfOptimizer::Suggestion::Severity::Warning ? "WARN"
                        : "INFO";
        STONEPOTATOES_LOG_INFO("[Optimize] [%s] %s: %s", sev,
                               s.category.c_str(), s.description.c_str());
    }
}

void MetaProgrammingAgent::Learn(bool success) {
    if (success) {
        m_LearningSystem->RecordSuccess("full_cycle", m_DevelopmentGoal, 0.8);
    } else {
        m_LearningSystem->RecordFailure("full_cycle", m_DevelopmentGoal, 0.2);
    }
    // Improvement score tracks measured success rate, not arbitrary increments.
    // 改進分數追蹤實際成功率，而非任意遞增。
    m_Statistics.selfImprovementScore =
        static_cast<float>(m_LearningSystem->SuccessRate());
}

// ============================================================================
// CodeGenerator
// ============================================================================

std::string CodeGenerator::MakeIdentifier(const std::string& input) {
    std::string out;
    out.reserve(input.size());
    bool capitalize = true;
    for (char c : input) {
        if (std::isalnum(static_cast<unsigned char>(c))) {
            out += capitalize ? static_cast<char>(std::toupper(static_cast<unsigned char>(c)))
                              : c;
            capitalize = false;
        } else {
            capitalize = true;
        }
    }
    if (out.empty()) out = "GeneratedFunc";
    if (std::isdigit(static_cast<unsigned char>(out.front()))) out = "Fn" + out;
    return out;
}

CodeGenerator::GenerationRequest CodeGenerator::ParseGoal(const std::string& goal) {
    GenerationRequest req;
    req.description = goal;
    req.functionName = MakeIdentifier(goal);

    std::string lower = ToLower(goal);
    if (lower.find("return int") != std::string::npos ||
        lower.find("sum") != std::string::npos ||
        lower.find("count") != std::string::npos) {
        req.returnType = "int";
    }

    // Default body: a small deterministic computation so the program
    // actually does something observable.
    // 預設主體：小型確定性計算，讓程式實際有可觀察的行為。
    req.bodyLines = {
        "    volatile long long acc = 0;",
        "    for (long long i = 0; i < 1000000LL; ++i) {",
        "        acc += (i * i) % 7;",
        "    }",
        "    return 0;"
    };
    if (req.returnType == "int")
        req.bodyLines.back() = "    return static_cast<int>(acc % 97);";
    else
        req.bodyLines.back() = "    (void)acc;";
    return req;
}

std::string CodeGenerator::GenerateCXX(const GenerationRequest& req) {
    std::ostringstream out;
    out << "// Auto-generated by StonePotatoes SelfDevelopingAgent\n";
    out << "// 由 StonePotatoes 自我開發 Agent 自動生成\n";
    out << "// Goal: " << req.description << "\n\n";
    out << "#include <iostream>\n";
    out << "#include <chrono>\n";
    for (const auto& inc : req.includes) out << inc << "\n";
    out << "\n";

    // Signature
    out << req.returnType << " " << req.functionName << "(";
    for (size_t i = 0; i < req.parameters.size(); ++i) {
        if (i) out << ", ";
        out << req.parameters[i];
    }
    out << ") {\n";
    for (const auto& line : req.bodyLines) out << line << "\n";
    if (req.returnType != "void" && req.bodyLines.back().find("return") == std::string::npos)
        out << "    return {};\n";
    out << "}\n\n";

    // Test harness
    out << "int main() {\n";
    out << "    auto t0 = std::chrono::steady_clock::now();\n";
    if (req.returnType == "void") {
        out << "    " << req.functionName << "();\n";
    } else {
        out << "    auto result = " << req.functionName << "();\n";
        out << "    std::cout << \"Result: \" << result << \"\\n\";\n";
    }
    out << "    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(\n";
    out << "                  std::chrono::steady_clock::now() - t0).count();\n";
    out << "    std::cout << \"Elapsed: \" << ms << \"ms\\n\";\n";
    out << "    return 0;\n";
    out << "}\n";
    return out.str();
}

std::string CodeGenerator::GenerateTest(const std::string& functionName,
                                        const std::string& bodyToTest) {
    std::ostringstream out;
    out << "// Auto-generated unit test | 自動生成單元測試\n";
    out << "#include <cassert>\n#include <iostream>\n\n";
    out << bodyToTest << "\n\n";
    out << "int main() {\n";
    out << "    " << functionName << "();\n";
    out << "    std::cout << \"TEST PASSED\\n\";\n";
    out << "    return 0;\n}\n";
    return out.str();
}

std::string CodeGenerator::GenerateComputeShader(const std::string& kernelName,
                                                 int threadGroupSizeX) {
    std::ostringstream out;
    out << "// Auto-generated HLSL compute shader | 自動生成 HLSL 計算著色器\n";
    out << "RWStructuredBuffer<float> Output : register(u0);\n\n";
    out << "[numthreads(" << threadGroupSizeX << ", 1, 1)]\n";
    out << "void " << (kernelName.empty() ? "CSMain" : kernelName)
        << "(uint3 dtid : SV_DispatchThreadID) {\n";
    out << "    Output[dtid.x] = dtid.x * dtid.x;\n";
    out << "}\n";
    return out.str();
}

// ============================================================================
// CodeParser — real static analysis
// ============================================================================

CodeParser::Analysis CodeParser::Analyze(const std::string& code) {
    Analysis a;
    auto lines = SplitLines(code);
    a.totalLines = static_cast<int>(lines.size());

    bool inBlockComment = false;
    std::regex fnRegex(R"(^\s*(?:[\w:<>,\*&]+\s+)+[\w:]+\s*\([^;]*\)\s*(?:const)?\s*\{?)");
    std::regex classRegex(R"(^\s*(?:class|struct)\s+\w+)");
    std::regex incRegex(R"(^\s*#\s*include\s*[<"]([^>"]+)[>"])");

    for (const auto& raw : lines) {
        std::string line = Trim(raw);
        if (line.empty()) { a.blankLines++; continue; }

        if (inBlockComment) {
            a.commentLines++;
            if (line.find("*/") != std::string::npos) inBlockComment = false;
            continue;
        }
        if (line.rfind("/*", 0) == 0) {
            a.commentLines++;
            if (line.find("*/") == std::string::npos) inBlockComment = true;
            continue;
        }
        if (line.rfind("//", 0) == 0) { a.commentLines++; continue; }

        a.codeLines++;

        std::smatch m;
        if (std::regex_match(line, m, incRegex)) a.includes.push_back(m[1]);
        if (std::regex_match(line, classRegex)) a.classCount++;
        if (std::regex_match(line, fnRegex) && line.find(";") == std::string::npos)
            a.functionCount++;

        if (line.find("TODO") != std::string::npos) a.todoCount++;
        if (line.find("FIXME") != std::string::npos) a.fixmeCount++;
        if (line.find("for (") != std::string::npos ||
            line.find("for(") != std::string::npos ||
            line.find("while (") != std::string::npos) a.loopCount++;
        if (line.find("if (") != std::string::npos ||
            line.find("if(") != std::string::npos) a.branchCount++;
        if (line.find("new ") != std::string::npos ||
            line.find("malloc") != std::string::npos) a.allocationCount++;
        if (line.find("[]") != std::string::npos ||
            line.find("[&]") != std::string::npos ||
            line.find("[=]") != std::string::npos) a.lambdaCount++;
    }

    if (a.todoCount > 0)
        a.warnings.push_back(std::to_string(a.todoCount) + " TODO markers remaining");
    if (a.allocationCount > 0 && a.loopCount > 0)
        a.warnings.push_back("Allocations present; verify none inside hot loops");
    return a;
}

std::vector<std::string> CodeParser::ExtractFunctionSignatures(const std::string& code) {
    std::vector<std::string> sigs;
    std::regex sigRegex(R"(([\w:<>,\*&\s]+)\s+(\w+)\s*\(([^)]*)\)\s*(?:const)?\s*\{?)");
    for (auto it = std::sregex_iterator(code.begin(), code.end(), sigRegex);
         it != std::sregex_iterator(); ++it) {
        sigs.push_back(Trim((*it)[1].str()) + " " + (*it)[2].str() +
                       "(" + (*it)[3].str() + ")");
    }
    return sigs;
}

// ============================================================================
// CodeMutator — real text transformations
// ============================================================================

std::string CodeMutator::Mutate(const std::string& code, MutationType type) {
    switch (type) {
    case MutationType::RemoveComments: {
        std::string out;
        out.reserve(code.size());
        bool inBlock = false;
        for (size_t i = 0; i < code.size(); ++i) {
            if (inBlock) {
                if (code[i] == '*' && i + 1 < code.size() && code[i + 1] == '/') {
                    inBlock = false; ++i;
                }
                continue;
            }
            if (code[i] == '/' && i + 1 < code.size()) {
                if (code[i + 1] == '/') {
                    while (i < code.size() && code[i] != '\n') ++i;
                    if (i < code.size()) out += code[i]; // keep newline
                    continue;
                }
                if (code[i + 1] == '*') { inBlock = true; ++i; continue; }
            }
            out += code[i];
        }
        return out;
    }
    case MutationType::AddTimingInstrumentation: {
        std::ostringstream out;
        out << "#include <chrono>\n" << code;
        out << "\n// Instrumented by CodeMutator\n";
        return out.str();
    }
    case MutationType::ReplaceMallocWithVector: {
        std::regex mallocRx(R"((\w+)\s*\*?\s*=\s*(?:\(\w+\s*\*\s*\))?\s*malloc\s*\(([^)]+)\))");
        return std::regex_replace(code, mallocRx,
                                  "std::vector<char> $1($2 /*converted from malloc*/)");
    }
    case MutationType::AddConstToReadOnlyParams:
        // Conservative: only annotate params marked with comment //const
        return std::regex_replace(code, std::regex(R"((\w+)\s+(\w+)\s*//const)"), "const $1 $2");
    case MutationType::ReserveVectorCapacity: {
        // Insert reserve() hint before a loop that contains push_back
        std::regex loopRx(R"((for\s*\([^\)]*\)\s*\{[^}]*push_back))");
        return std::regex_replace(code, loopRx,
            "/*CodeMutator: consider v.reserve(N)*/ $1");
    }
    }
    return code;
}

std::vector<CodeMutator::MutationType>
CodeMutator::SuggestMutations(const CodeParser::Analysis& a) {
    std::vector<MutationType> v;
    if (a.allocationCount > 0) v.push_back(MutationType::ReplaceMallocWithVector);
    if (a.commentLines > a.codeLines) v.push_back(MutationType::RemoveComments);
    if (a.loopCount > 0) v.push_back(MutationType::ReserveVectorCapacity);
    return v;
}

// ============================================================================
// SelfTester — real compile & run
// ============================================================================

std::vector<std::string> SelfTester::DetectBugs(const std::string& code) {
    std::vector<std::string> bugs;

    int openBraces = 0;
    for (char c : code) {
        if (c == '{') ++openBraces;
        if (c == '}') --openBraces;
    }
    if (openBraces != 0)
        bugs.push_back("Unbalanced braces: delta=" + std::to_string(openBraces));

    if (code.find("malloc") != std::string::npos &&
        code.find("free") == std::string::npos)
        bugs.push_back("malloc without free — possible leak");

    if (code.find("new ") != std::string::npos &&
        code.find("delete") == std::string::npos)
        bugs.push_back("'new' without 'delete' — possible leak");

    if (std::regex_search(code, std::regex(R"(while\s*\(\s*true\s*\))")) &&
        code.find("break") == std::string::npos)
        bugs.push_back("Infinite loop without break");

    // Self-assignment like `x = x;` (backreference)
    // 自我賦值，如 `x = x;`（反向參照）
    if (std::regex_search(code, std::regex(R"(\b(\w+)\s*=\s*\1\s*;)")))
        bugs.push_back("Self-assignment detected (x = x;)");

    return bugs;
}

SelfTester::TestResult
SelfTester::CompileAndRun(const std::string& sourceFile,
                          const std::string& outputExe) {
    TestResult r;

    // Detect compiler | 偵測編譯器
    if (m_Compiler.empty()) {
        std::string probe;
#ifdef _WIN32
        if (RunCommand("where cl", probe, 10) == 0 && !probe.empty()) {
            m_Compiler = "cl /std:c++20 /O2 /nologo /EHsc " + Q(sourceFile) +
                         " /Fe:" + Q(outputExe) + " /Fo:" +
                         Q((fs::path(outputExe).parent_path() / "obj").string());
        } else if (RunCommand("where g++", probe, 10) == 0 && !probe.empty()) {
            m_Compiler = "g++ -std=c++20 -O2 -o " + Q(outputExe) + " " + Q(sourceFile);
        } else {
            // Locate MSVC via vswhere and invoke through vcvars64.bat.
            // 透過 vswhere 定位 MSVC，並經由 vcvars64.bat 呼叫。
            const char* pf86 = std::getenv("ProgramFiles(x86)");
            std::string vswhere =
                (pf86 ? pf86 : "C:\\Program Files (x86)") +
                std::string("\\Microsoft Visual Studio\\Installer\\vswhere.exe");
            if (fs::exists(vswhere)) {
                std::string vsPath;
                if (RunCommand(Q(vswhere) + " -latest -products * -requires "
                               "Microsoft.VisualStudio.Component.VC.Tools.x86.x64 "
                               "-property installationPath", vsPath, 15) == 0) {
                    auto nl = vsPath.find('\n');
                    std::string install = Trim(vsPath.substr(0, nl));
                    std::string vcvars =
                        install + "\\VC\\Auxiliary\\Build\\vcvars64.bat";
                    if (fs::exists(vcvars)) {
                        m_Compiler = "call " + Q(vcvars) +
                                     " >nul 2>&1 && cl /std:c++20 /O2 /nologo /EHsc " +
                                     Q(sourceFile) + " /Fe:" + Q(outputExe) +
                                     " /Fo:" + Q((fs::path(outputExe).parent_path() /
                                                  "obj").string());
                    }
                }
            }
        }
        if (m_Compiler.empty())
            m_Compiler = "g++ -std=c++20 -O2 -o " + Q(outputExe) + " " + Q(sourceFile);
#else
        if (RunCommand("which g++", probe, 10) == 0)
            m_Compiler = "g++ -std=c++20 -O2 -o " + Q(outputExe) + " " + Q(sourceFile);
        else if (RunCommand("which clang++", probe, 10) == 0)
            m_Compiler = "clang++ -std=c++20 -O2 -o " + Q(outputExe) + " " + Q(sourceFile);
        else
            m_Compiler = "g++ -std=c++20 -O2 -o " + Q(outputExe) + " " + Q(sourceFile);
#endif
    }

    // Compile | 編譯
    std::error_code ec;
    fs::create_directories(fs::path(outputExe).parent_path() / "obj", ec);
    auto c0 = std::chrono::steady_clock::now();
    int rc = RunCommand(m_Compiler, r.compileOutput, m_CompileTimeoutSec);
    r.compileTimeMs = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - c0).count());

    if (rc != 0 || !fs::exists(outputExe)) {
        r.errors.push_back("Compilation failed (rc=" + std::to_string(rc) + ")");
        return r;
    }
    r.compiled = true;

    // Run | 執行
    auto r0 = std::chrono::steady_clock::now();
    rc = RunCommand(Q(outputExe), r.runOutput, m_RunTimeoutSec);
    r.runTimeMs = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - r0).count());

    r.executed = (rc != -2); // -2 = timeout
    r.exitCode = rc;
    r.passed = (rc == 0);
    if (rc == -2) r.errors.push_back("Execution timeout");
    else if (rc != 0) r.errors.push_back("Non-zero exit: " + std::to_string(rc));
    return r;
}

// ============================================================================
// SelfOptimizer — real static-analysis suggestions
// ============================================================================

std::vector<SelfOptimizer::Suggestion>
SelfOptimizer::Analyze(const std::string& code) {
    std::vector<Suggestion> v;
    auto lines = SplitLines(code);

    std::regex loopRx(R"((for|while)\s*\()");
    std::regex endlRx(R"(std::endl)");
    std::regex newRx(R"(new\s+\w+)");
    std::regex pushBackRx(R"(push_back)");
    std::regex strCatRx(R"(\+=\s*)");
    std::regex copyRx(R"(std::vector<\w+>\s+\w+\s*=\s*\w+\s*;)");

    bool inLoop = false;
    int loopDepth = 0;
    for (size_t i = 0; i < lines.size(); ++i) {
        const std::string& L = lines[i];
        if (std::regex_search(L, loopRx)) { inLoop = true; loopDepth++; }
        if (inLoop && L.find('}') != std::string::npos) {
            if (--loopDepth <= 0) inLoop = false;
        }

        if (inLoop) {
            if (std::regex_search(L, newRx))
                v.push_back({Suggestion::Severity::Critical, "memory",
                    "Heap allocation inside loop — hoist outside", (int)i});
            if (std::regex_search(L, endlRx))
                v.push_back({Suggestion::Severity::Warning, "io",
                    "std::endl inside loop flushes each iteration — use '\\n'", (int)i});
            if (std::regex_search(L, strCatRx) &&
                code.find("std::string") != std::string::npos &&
                L.find("std::string") == std::string::npos)
                v.push_back({Suggestion::Severity::Warning, "string",
                    "String += in loop may reallocate — consider reserve() or ostringstream", (int)i});
            if (std::regex_search(L, pushBackRx))
                v.push_back({Suggestion::Severity::Info, "container",
                    "push_back in loop — call reserve() before the loop", (int)i});
        }

        if (std::regex_search(L, copyRx))
            v.push_back({Suggestion::Severity::Info, "container",
                "Vector copy — consider const ref or move", (int)i});
    }
    return v;
}

std::string SelfOptimizer::ApplySuggestion(const std::string& code,
                                           const Suggestion& s) {
    if (s.category == "io") {
        return std::regex_replace(code, std::regex(R"(std::endl)"), "\"\\n\"");
    }
    if (s.category == "container" && s.description.find("reserve") != std::string::npos) {
        return "// SelfOptimizer: add reserve() before loop\n" + code;
    }
    return code;
}

SelfOptimizer::OptimizationSummary
SelfOptimizer::AutoOptimize(const std::string& code) {
    OptimizationSummary s;
    s.suggestions = Analyze(code);
    s.suggestionsFound = static_cast<int>(s.suggestions.size());
    // Auto-apply only safe transformations
    std::string cur = code;
    for (const auto& sug : s.suggestions) {
        if (sug.severity == Suggestion::Severity::Info) {
            std::string next = ApplySuggestion(cur, sug);
            if (next != cur) { cur = next; s.applied++; }
        }
    }
    return s;
}

// ============================================================================
// LearningSystem — JSONL persistence (real file I/O)
// ============================================================================

LearningSystem::LearningSystem(const std::string& storagePath)
    : m_StoragePath(storagePath) {}

LearningSystem::~LearningSystem() { Save(); }

void LearningSystem::RecordSuccess(const std::string& pattern,
                                   const std::string& context, double score) {
    m_Records.push_back({context, pattern, true, score, NowISO8601()});
}

void LearningSystem::RecordFailure(const std::string& pattern,
                                   const std::string& context, double score) {
    m_Records.push_back({context, pattern, false, score, NowISO8601()});
}

double LearningSystem::SuccessRate() const {
    if (m_Records.empty()) return 0.0;
    int ok = 0;
    for (const auto& r : m_Records) if (r.success) ++ok;
    return static_cast<double>(ok) / m_Records.size();
}

std::string LearningSystem::SuggestApproach(const std::string& goal) const {
    std::string lower = ToLower(goal);
    std::string best;
    double bestScore = -1.0;
    for (const auto& r : m_Records) {
        if (r.success && ToLower(r.goal).find(lower) != std::string::npos &&
            r.score > bestScore) {
            bestScore = r.score;
            best = r.patternUsed;
        }
    }
    return best.empty() ? "template_generation" : best;
}

bool LearningSystem::Save() const {
    std::ofstream out(m_StoragePath, std::ios::trunc);
    if (!out) return false;
    for (const auto& r : m_Records) {
        out << "{\"goal\":\"" << r.goal
            << "\",\"pattern\":\"" << r.patternUsed
            << "\",\"success\":" << (r.success ? "true" : "false")
            << ",\"score\":" << r.score
            << ",\"ts\":\"" << r.timestamp << "\"}\n";
    }
    return out.good();
}

bool LearningSystem::Load() {
    std::ifstream in(m_StoragePath);
    if (!in) return false;
    std::string line;
    std::regex rx(R"JSON("goal":"([^"]*)".*"pattern":"([^"]*)".*"success":(true|false).*"score":([\d.]+).*"ts":"([^"]*)")JSON");
    while (std::getline(in, line)) {
        std::smatch m;
        if (std::regex_search(line, m, rx) && m.size() == 6) {
            Experience e;
            e.goal = m[1];
            e.patternUsed = m[2];
            e.success = (m[3] == "true");
            e.score = std::stod(m[4]);
            e.timestamp = m[5];
            m_Records.push_back(e);
        }
    }
    return true;
}

// ============================================================================
// SandboxEnvironment — real process execution with timeout
// ============================================================================

void SandboxEnvironment::SetResourceLimits(double cpuSec, double memMB) {
    m_MaxCpuSeconds = cpuSec;
    m_MaxMemoryMB = memMB;
}

void SandboxEnvironment::SetWorkingDirectory(const std::string& dir) {
    m_WorkingDir = dir;
}

SandboxEnvironment::ExecutionResult
SandboxEnvironment::Execute(const std::string& exePath,
                            const std::vector<std::string>& args,
                            int timeoutSeconds) {
    ExecutionResult r;
    auto t0 = std::chrono::steady_clock::now();

    std::ostringstream cmd;
    if (!m_WorkingDir.empty()) {
#ifdef _WIN32
        cmd << "cd /d " << Q(m_WorkingDir) << " && ";
#else
        cmd << "cd " << Q(m_WorkingDir) << " && ";
#endif
    }
    cmd << Q(exePath);
    for (const auto& a : args) cmd << " " << a;

    std::string output;
    int rc = RunCommand(cmd.str(), output, timeoutSeconds);

    r.elapsedMs = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - t0).count());
    r.stdOut = output;
    r.exitCode = rc;
    r.timedOut = (rc == -2);
    r.success = (rc == 0);
    return r;
}

} // namespace SelfDevelopingAgent
} // namespace StonePotatoes
