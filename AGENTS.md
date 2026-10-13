# AGENTS.md

## web3-agent (Java, standalone Maven module)

- Toolchain: `D:\HWC\.tools` (JDK 25 Temurin, Maven 3.9.11). In Git Bash:
  `export JAVA_HOME=/d/HWC/.tools/jdk/jdk-25.0.4.1+1; export PATH=$JAVA_HOME/bin:/d/HWC/.tools/apache-maven-3.9.11/bin:$PATH`
- Test: `cd web3-agent && mvn -B test` (fully offline after first dependency download; no chain or LLM needed)
- Package: `mvn -B package` -> `web3-agent/target/web3-agent-0.1.0.jar`
- JSON uses Jackson 3 (`tools.jackson.*`, pulled in by web3j 6), not `com.fasterxml.jackson`.
- Not part of the CMake build; independent of the C++ engine.
