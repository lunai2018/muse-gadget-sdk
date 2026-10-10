#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.
# Licensed under the Apache License, Version 2.0 (the "License");

"""Securely provision Azure Speech settings over USB without putting the key on the command line."""

import argparse
import getpass
import json
import time

from chat import Board, BoardError, pick_port


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port")
    parser.add_argument("--region", default="eastus")
    parser.add_argument("--voice", default="en-US-AvaNeural")
    args = parser.parse_args()

    key = getpass.getpass("Azure Speech key (hidden): ").strip()
    if not key:
        parser.error("the key is empty")

    try:
        with Board(args.port or pick_port()) as board:
            board.drain(0.2)
            # 密钥只进入串口写缓冲，不出现在命令行、日志或脚本文件中。
            board.write_line("tts.azure.key=" + key)
            board.write_line("tts.azure.region=" + args.region)
            board.write_line("tts.azure.voice=" + args.voice)
            key = ""
            time.sleep(0.4)
            status = board.status()
            if not status:
                raise BoardError("The board did not return status after provisioning.")
            tts = status.get("device", {}).get("tts", {})
            configured = tts.get("configured")
            persisted = tts.get("persisted")
            if configured is not True or persisted is not True:
                raise BoardError("The board did not confirm persisted Azure TTS configuration.")
            print(json.dumps({"ok": True, "port": board.port, "region": args.region,
                              "voice": args.voice, "configured": True, "persisted": True}))
    except BoardError as exc:
        raise SystemExit(str(exc))


if __name__ == "__main__":
    main()
