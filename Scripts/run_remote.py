"""在已打开的编辑器里通过 UE 的 Python 远程执行跑一段脚本。

用法（用引擎自带的 python 运行）：
    python run_remote.py <要执行的 .py 文件路径> [超时秒数]

编辑器需要启用 Python 远程执行（PythonScriptPlugin 默认开启）。
"""

import os
import socket
import sys
import time

sys.path.append(
    r"C:\Program Files\Epic Games\UE_5.8\Engine\Plugins\Experimental\PythonScriptPlugin\Content\Python"
)
import remote_execution  # noqa: E402


def main():
    if len(sys.argv) < 2:
        print("usage: run_remote.py <script.py> [timeout]")
        return 2

    script_path = sys.argv[1]
    timeout = float(sys.argv[2]) if len(sys.argv) > 2 else 300.0

    with open(script_path, "r", encoding="utf-8") as handle:
        code = handle.read()

    config = remote_execution.RemoteExecutionConfig()
    config.multicast_bind_address = "0.0.0.0"
    remote_exec = remote_execution.RemoteExecution(config)
    remote_exec.start()

    # 等编辑器在组播里被发现
    deadline = time.time() + 30.0
    nodes = []
    while time.time() < deadline:
        nodes = remote_exec.remote_nodes
        if nodes:
            break
        time.sleep(0.5)

    if not nodes:
        print("ERROR: no remote node discovered; is the editor running with Python remote execution enabled?")
        remote_exec.stop()
        return 1

    node_id = nodes[0]["node_id"]
    print(f"connecting to node {node_id}")

    # 连接结果通过回调异步返回
    connected = {"ok": False}

    def on_open(result):
        connected["ok"] = result.get("success", False)

    remote_exec.open_command_connection(node_id, on_open=on_open)
    for _ in range(40):
        if connected["ok"]:
            break
        time.sleep(0.25)

    if not connected["ok"]:
        print("ERROR: failed to open command connection")
        remote_exec.stop()
        return 1

    result = remote_exec.run_command(code, exec_mode=remote_execution.MODE_EXEC_FILE)

    if result is None:
        print("ERROR: no result (timed out?)")
        remote_exec.stop()
        return 1

    for line in result.get("output", []):
        print(line.get("output", ""), end="" if line.get("type") != "error" else "\n")
    if result.get("result"):
        print(f"result: {result['result']}")

    remote_exec.stop()
    return 0 if result.get("success") else 1


if __name__ == "__main__":
    sys.exit(main())