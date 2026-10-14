#include "MetaProgrammingAgent.h"

#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: StonePotatoesSelfDevelopingAgent <development goal>\n";
        return 2;
    }

    std::string goal;
    for (int i = 1; i < argc; ++i) {
        if (!goal.empty()) goal += ' ';
        goal += argv[i];
    }

    StonePotatoes::SelfDevelopingAgent::MetaProgrammingAgent agent;
    agent.SetDevelopmentGoal(goal);
    agent.Initialize();
    const bool succeeded = agent.DevelopmentCycle();

    const auto& stats = agent.GetStatistics();
    std::cout << "Goal: " << goal << '\n'
              << "Generated lines: " << stats.codeGeneratedLines << '\n'
              << "Generated tests: " << stats.testsGenerated << '\n'
              << "Bugs found: " << stats.bugsFound << '\n'
              << "Optimizations: " << stats.optimizationsPerformed << '\n'
              << "Workspace: " << agent.GetWorkspaceDirectory() << '\n';
    agent.Shutdown();
    return succeeded ? 0 : 1;
}
