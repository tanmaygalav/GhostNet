import subprocess
import json
import os
from typing import Tuple, Dict, Any

# Path to the compiled C++ executable
DEFAULT_CLI_PATH = os.path.abspath("../build/ghostnet_cli")


class EngineExecutionError(Exception):
    pass


def run_simulation(
    scenario_dict: Dict[str, Any],
    cli_path: str = DEFAULT_CLI_PATH,
    override_seed: int | None = None,
) -> Tuple[str, Dict[str, Any]]:
    """Runs the compiled C++ simulation executable using stdin/stdout IPC.

    Returns:
        (status, full_json_output)
    """
    if not os.path.exists(cli_path):
        raise FileNotFoundError(
            f"Simulation engine executable not found at: {cli_path}. Please build it first."
        )

    # Apply seed override if provided
    if override_seed is not None:
        if "config" not in scenario_dict:
            scenario_dict["config"] = {}
        scenario_dict["config"]["seed"] = override_seed

    input_json_str = json.dumps(scenario_dict)

    # Launch subprocess with pipe communication
    process = subprocess.Popen(
        [cli_path],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )

    stdout, stderr = process.communicate(input=input_json_str)

    if process.returncode != 0:
        raise EngineExecutionError(
            f"Engine crashed (code {process.returncode}):\n{stderr}"
        )

    try:
        output_data = json.loads(stdout)
    except json.JSONDecodeError as err:
        raise EngineExecutionError(
            f"Failed to parse engine output JSON: {err}\nRaw output: {stdout}"
        )

    status = output_data.get("status", "COMPLETED")
    return status, output_data