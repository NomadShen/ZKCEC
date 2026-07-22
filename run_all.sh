#!/usr/bin/env bash

set -u
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
DESIGN_FILE="${SCRIPT_DIR}/input/design.f"
RESULT_DIR="${SCRIPT_DIR}/res"
NON_OPT_RESULT_DIR="${RESULT_DIR}/non_opt"
OPT_RESULT_DIR="${RESULT_DIR}/opt"
RUNNER="${SCRIPT_DIR}/run_experiment.sh"

if [[ ! -f "${DESIGN_FILE}" ]]; then
    echo "Error: design file not found: ${DESIGN_FILE}" >&2
    exit 1
fi

if [[ ! -x "${RUNNER}" ]]; then
    echo "Error: runner is not executable: ${RUNNER}" >&2
    exit 1
fi

mkdir -p "${NON_OPT_RESULT_DIR}" "${OPT_RESULT_DIR}"
cd "${SCRIPT_DIR}"

failed_runs=0

run_variant() {
    local design_name="$1"
    local variant="$2"
    local log_file="$3"
    local -a args=(-d "${design_name}")
    local status

    if [[ "${variant}" == "optimized" ]]; then
        args+=(-o)
    fi

    echo "Running ${design_name} (${variant})..."

    {
        echo "===== ${design_name}: ${variant} ====="
        echo "+ ./run_experiment.sh ${args[*]}"
        "${RUNNER}" "${args[@]}"
        status=$?
        echo "===== exit status: ${status} ====="
        echo
    } >> "${log_file}" 2>&1

    return "${status}"
}

while IFS= read -r design_name || [[ -n "${design_name}" ]]; do
    # Support CRLF files, and ignore empty lines and comments.
    design_name="${design_name%$'\r'}"
    [[ -z "${design_name}" || "${design_name}" == \#* ]] && continue

    non_opt_log_file="${NON_OPT_RESULT_DIR}/${design_name}.log"
    opt_log_file="${OPT_RESULT_DIR}/${design_name}.log"
    : > "${non_opt_log_file}"
    : > "${opt_log_file}"

    if ! run_variant "${design_name}" "standard" "${non_opt_log_file}"; then
        ((failed_runs += 1))
    fi

    if ! run_variant "${design_name}" "optimized" "${opt_log_file}"; then
        ((failed_runs += 1))
    fi
done < "${DESIGN_FILE}"

if ((failed_runs > 0)); then
    echo "Completed with ${failed_runs} failed run(s). See ${RESULT_DIR} for details." >&2
    exit 1
fi

echo "All experiments completed successfully. Logs are in ${RESULT_DIR}."

echo "Analyzing results..."
python3 res/analyze.py
