#!/usr/bin/env bash
# Verify agent-service refuses to start without WEB3AGENT_PASSWORD,
# and starts normally when it is set (positive control).
# 驗證 agent-service 未設定 WEB3AGENT_PASSWORD 時拒絕啟動；設定時可正常啟動（對照組）。
set -uo pipefail
JAR="${1:-web3-ide/agent-service/target/agent-service-1.0.0.jar}"
ARGS=(--spring.cloud.config.enabled=false --eureka.client.enabled=false --server.port=0)
work=$(mktemp -d)

echo "== 1) WEB3AGENT_PASSWORD unset: expect non-zero exit + placeholder error"
env -u WEB3AGENT_PASSWORD timeout 120 java -jar "$JAR" "${ARGS[@]}" --agent.data-dir="$work/neg" >"$work/neg.log" 2>&1
code=$?
if [[ $code -eq 0 || $code -eq 124 ]]; then
  echo "FAIL: expected startup failure, got exit $code"; tail -50 "$work/neg.log"; exit 1
fi
if ! grep -q "Could not resolve placeholder 'WEB3AGENT_PASSWORD'" "$work/neg.log"; then
  echo "FAIL: exited $code but not because of the missing password"; tail -50 "$work/neg.log"; exit 1
fi
echo "OK: exit $code, missing WEB3AGENT_PASSWORD detected"

echo "== 2) WEB3AGENT_PASSWORD set: expect 'Started AgentServiceApplication'"
WEB3AGENT_PASSWORD=ci-dummy-pass timeout 120 java -jar "$JAR" "${ARGS[@]}" --agent.data-dir="$work/pos" >"$work/pos.log" 2>&1 &
pid=$!
for _ in $(seq 1 110); do
  if grep -q "Started AgentServiceApplication" "$work/pos.log"; then
    echo "OK: started with password set"; kill "$pid" 2>/dev/null; wait "$pid" 2>/dev/null; exit 0
  fi
  kill -0 "$pid" 2>/dev/null || break
  sleep 1
done
echo "FAIL: did not start with password set"; tail -50 "$work/pos.log"; kill "$pid" 2>/dev/null; exit 1
