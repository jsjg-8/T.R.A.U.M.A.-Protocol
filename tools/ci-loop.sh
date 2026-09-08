#!/usr/bin/env bash
# ci-loop.sh — poll gh run until completed/success or completed/failure, then break
# Usage: ./tools/ci-loop.sh <run-id> [next-phase-cmd]
# Example: ./tools/ci-loop.sh 34276987961 "echo 'launch Phase 1'"
set -e
RUN_ID="${1:?run-id required}"
NEXT_CMD="${2:-}"
POLL=30
echo "Watching run $RUN_ID (poll ${POLL}s, break on completed/*) ..."
echo "https://github.com/jsjg-8/T.R.A.U.M.A.-Protocol/actions/runs/$RUN_ID"
while true; do
  JSON=$(gh run view "$RUN_ID" --json status,conclusion -q '"\(.status)/\(.conclusion)"' 2>&1 || echo "unknown/unknown")
  TS=$(date +%T)
  echo "---$TS $JSON---"
  case "$JSON" in
    *completed/success*)
      echo "✓ SUCCESS $RUN_ID"
      if [ -n "$NEXT_CMD" ]; then echo "→ launching next phase: $NEXT_CMD"; eval "$NEXT_CMD"; fi
      exit 0
      ;;
    *completed/failure*)
      echo "✗ FAILURE $RUN_ID"
      echo "--- log errors ---"
      gh run view "$RUN_ID" --log 2>&1 | grep -A8 -B2 "error:" | head -100 || gh run view "$RUN_ID" --log 2>&1 | tail -100
      if [ -n "$NEXT_CMD" ]; then echo "→ not launching next phase due to failure"; fi
      exit 1
      ;;
    *completed/cancelled*)
      echo "⊘ CANCELLED $RUN_ID"
      exit 2
      ;;
  esac
  sleep "$POLL"
done
