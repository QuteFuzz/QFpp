import os
import subprocess
import sys
from pathlib import Path
from typing import Dict, List


class Color:
    GREEN = "\033[92m"
    RED = "\033[91m"
    YELLOW = "\033[93m"
    BLUE = "\033[94m"
    RESET = "\033[0m"


def log(msg, color=Color.RESET):
    print(f"{color} {msg}{Color.RESET}")


def pipe_to_process(cmd, cwd, cmd_to_process):
    try:
        process = subprocess.Popen(
            cmd,
            cwd=cwd,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )

        stdout, stderr = process.communicate(input=cmd_to_process)

        if process.returncode != 0:
            print(stderr)
            raise Exception("Fuzzer exited with errors")

        log("Generation complete.", Color.GREEN)

    except Exception as e:
        log(f"Command {cmd} failed", Color.RED)
        raise e


def run_command(command, cwd=None, env=None, capture_output=False, timeout=None):
    """Helper to run shell commands"""
    try:
        result = subprocess.run(
            command,
            cwd=cwd,
            env=env,
            check=True,
            capture_output=capture_output,
            text=True,
            timeout=timeout,
        )

        if result.returncode != 0:
            print(result.stdout)
            print(result.stderr)
            sys.exit(-1)

        return result

    except subprocess.CalledProcessError as e:
        if capture_output:
            print(e.stdout)
            print(e.stderr)

        raise e

    except subprocess.TimeoutExpired:
        print(f"Command {command} timed out after {timeout}s")
        sys.exit(-1)


def modify_env(vars: Dict[str, List[Path] | str]):
    env = os.environ.copy()

    for var, new_vals in vars.items():
        old_vals = env.get(var, "")

        if isinstance(new_vals, str):
            env[var] = new_vals

        else:
            joined_new_vals = os.pathsep.join(
                str(val) for val in new_vals if str(val) not in old_vals
            )

            if old_vals == "":
                env[var] = joined_new_vals
            else:
                env[var] = f"{joined_new_vals}{os.pathsep}{old_vals}"

    return env
