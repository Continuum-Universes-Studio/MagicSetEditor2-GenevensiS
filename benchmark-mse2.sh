#!/usr/bin/env bash
# GenevensiS repeated set-import benchmark (wxGTK vs wxQt).
# Runs the real MSE command-line set loader and exits cleanly with :quit.
set -euo pipefail

usage() {
  echo "Usage: $0 /absolute/path/to/large.mse-set [runs-per-backend]" >&2
  echo "Run from your MagicSetEditor2-GenevensiS repository root." >&2
}
if [[ $# -lt 1 || $# -gt 2 ]]; then usage; exit 2; fi
SET_FILE=$(realpath -- "$1")
RUNS=${2:-10}
if [[ ! -e "$SET_FILE" ]]; then echo "Set not found: $SET_FILE" >&2; exit 2; fi
if ! [[ "$RUNS" =~ ^[1-9][0-9]*$ ]] || (( RUNS > 100 )); then echo "Runs must be 1–100." >&2; exit 2; fi
for exe in ./build-ninja/magicseteditor ./build-qt/magicseteditor; do
  if [[ ! -x "$exe" ]]; then echo "Missing executable: $exe" >&2; exit 2; fi
done
if [[ ! -x /usr/bin/time ]]; then echo "GNU /usr/bin/time is required." >&2; exit 2; fi

OUT="genevensis-benchmark-$(date +%Y%m%d-%H%M%S).csv"
TMP=$(mktemp -d)
trap 'rm -rf -- "$TMP"' EXIT
printf 'backend,run,elapsed_seconds,max_rss_kb,cpu_percent\n' > "$OUT"

run_one() {
  local backend=$1 number=$2 exe=$3 timedata="$TMP/time" errors="$TMP/errors"
  : > "$timedata"; : > "$errors"
  # Feed the program its native :quit command. No GUI automation is involved.
  if ! /usr/bin/time -f '%e,%M,%P' -o "$timedata" \
     "$exe" --cli "$SET_FILE" --quiet <<< ':quit' > /dev/null 2> "$errors"; then
    echo "FAILED: $backend run $number. Log:" >&2
    tail -n 30 "$errors" >&2
    exit 1
  fi
  local measurement
  measurement=$(tail -n 1 "$timedata")
  if [[ ! "$measurement" =~ ^[0-9]+([.][0-9]+)?,[0-9]+,[0-9]+%$ ]]; then
    echo "Unexpected timing output: $measurement" >&2; exit 1
  fi
  if (( number > 0 )); then
    printf '%s,%d,%s\n' "$backend" "$number" "$measurement" >> "$OUT"
    printf '  %-5s #%02d  %s\n' "$backend" "$number" "$measurement"
  fi
}

printf 'Large-set import + clean shutdown benchmark\nSet: %s\nRuns per backend: %d\n' "$SET_FILE" "$RUNS"
printf 'Warm-up runs (excluded from results)...\n'
run_one GTK 0 ./build-ninja/magicseteditor
run_one Qt 0 ./build-qt/magicseteditor
printf '\nRecorded runs (elapsed seconds, peak RSS KB, CPU):\n'
for ((i=1; i<=RUNS; i++)); do
  if (( i % 2 )); then
    run_one GTK "$i" ./build-ninja/magicseteditor
    run_one Qt "$i" ./build-qt/magicseteditor
  else
    run_one Qt "$i" ./build-qt/magicseteditor
    run_one GTK "$i" ./build-ninja/magicseteditor
  fi
done

printf '\nSummary (mean elapsed time; mean peak RSS):\n'
awk -F, '
  NR > 1 { t[$1]+=$3; m[$1]+=$4; n[$1]++ }
  END {
    for (b in n) printf "  %-5s %.3f s | %.1f MiB (%d runs)\n", b, t[b]/n[b], m[b]/n[b]/1024, n[b]
    if (n["GTK"] && n["Qt"] && t["Qt"] > 0)
      printf "  GTK/Qt elapsed-time ratio: %.2fx\n", (t["GTK"]/n["GTK"])/(t["Qt"]/n["Qt"])
  }' "$OUT"
printf '\nCSV saved: %s\n' "$OUT"
