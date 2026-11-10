#!/usr/bin/env bash
# Benchmarks the native GenevensiS --benchmark-gui mode on wxGTK and wxQt.
# The GUI opens, imports the set, paints the first card, then closes itself.
set -euo pipefail

usage() {
  echo "Usage: $0 /path/to/large.mse-set [runs-per-backend]" >&2
  echo "Run from the GenevensiS repository root, after rebuilding BOTH backends." >&2
}
if (( $# < 1 || $# > 2 )); then usage; exit 2; fi
SET_FILE=$(realpath -- "$1")
RUNS=${2:-10}
if [[ ! -e "$SET_FILE" ]]; then echo "Missing set: $SET_FILE" >&2; exit 2; fi
if [[ ! "$RUNS" =~ ^[1-9][0-9]*$ ]] || (( RUNS > 100 )); then echo "Runs must be between 1 and 100" >&2; exit 2; fi
for exe in ./build-ninja/magicseteditor ./build-qt/magicseteditor; do
  if [[ ! -x "$exe" ]]; then echo "Missing executable $exe" >&2; exit 2; fi
done
for command in /usr/bin/time timeout awk; do
  if ! command -v "$command" >/dev/null; then echo "Missing command $command" >&2; exit 2; fi
done

OUT="genevensis-gui-benchmark-$(date +%Y%m%d-%H%M%S).csv"
TMP=$(mktemp -d)
trap 'rm -rf -- "$TMP"' EXIT
printf 'backend,run,import_seconds,first_card_seconds,total_process_seconds,max_rss_kb,cpu_percent\n' > "$OUT"

run_one() {
  local backend=$1 number=$2 exe=$3 log="$TMP/app.log" tfile="$TMP/time.txt"
  : > "$log"; : > "$tfile"
  if ! timeout --signal=TERM --kill-after=5s 120s \
      /usr/bin/time -f '%e,%M,%P' -o "$tfile" \
      "$exe" --benchmark-gui "$SET_FILE" > "$log" 2>&1; then
    echo "ERROR: $backend run $number failed or exceeded 120 seconds. Application output:" >&2
    tail -n 50 "$log" >&2
    exit 1
  fi
  local import first elapsed rss cpu
  import=$(sed -n 's/^BENCHMARK_GUI_IMPORT_SECONDS=//p' "$log" | tail -n 1)
  first=$(sed -n 's/^BENCHMARK_GUI_FIRST_CARD_SECONDS=//p' "$log" | tail -n 1)
  IFS=, read -r elapsed rss cpu < "$tfile"
  if [[ ! "$import" =~ ^[0-9]+([.][0-9]+)?$ || ! "$first" =~ ^[0-9]+([.][0-9]+)?$ || ! "$elapsed" =~ ^[0-9]+([.][0-9]+)?$ || ! "$rss" =~ ^[0-9]+$ || ! "$cpu" =~ ^[0-9]+%$ ]]; then
    echo "ERROR: $backend output does not contain valid benchmark measurements." >&2
    cat "$log" >&2
    exit 1
  fi
  if (( number > 0 )); then
    printf '%s,%d,%s,%s,%s,%s,%s\n' "$backend" "$number" "$import" "$first" "$elapsed" "$rss" "$cpu" >> "$OUT"
    printf '  %-4s #%02d import %6.3fs | first card %6.3fs | process %6.3fs | RSS %6.1f MiB\n' \
      "$backend" "$number" "$import" "$first" "$elapsed" "$(awk -v r="$rss" 'BEGIN {printf "%.1f", r/1024}')"
  fi
}

echo "GenevensiS real GUI benchmark"
echo "Set: $SET_FILE"
echo "Runs per backend: $RUNS"
echo "Warm-up (not recorded)..."
run_one GTK 0 ./build-ninja/magicseteditor
run_one Qt 0 ./build-qt/magicseteditor
echo "Recorded runs:"
for (( i=1; i<=RUNS; i++ )); do
  if (( i % 2 )); then
    run_one GTK "$i" ./build-ninja/magicseteditor
    run_one Qt "$i" ./build-qt/magicseteditor
  else
    run_one Qt "$i" ./build-qt/magicseteditor
    run_one GTK "$i" ./build-ninja/magicseteditor
  fi
done
printf '\nSummary (averages):\n'
awk -F, '
NR>1 {n[$1]++; imp[$1]+=$3; paint[$1]+=$4; total[$1]+=$5; mem[$1]+=$6}
END {
  for (i=1;i<=2;i++) {
    b=(i==1?"GTK":"Qt"); if (!n[b]) continue;
    printf "  %-4s import %.3fs | first card %.3fs | process %.3fs | RAM %.1f MiB\n",b,imp[b]/n[b],paint[b]/n[b],total[b]/n[b],mem[b]/n[b]/1024
  }
  if (n["GTK"] && n["Qt"] && paint["Qt"] > 0)
    printf "  First-card GTK/Qt ratio: %.2fx\n", (paint["GTK"]/n["GTK"])/(paint["Qt"]/n["Qt"])
}' "$OUT"
printf '\nCSV saved: %s\n' "$OUT"
