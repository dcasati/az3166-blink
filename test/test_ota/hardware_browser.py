"""Opt-in real-board OTA smoke test. Never interrupt bootloader installation."""
import argparse
import base64
import hashlib
import json
import os
import pathlib
import threading
import time
import urllib.error
import urllib.request

import serial
from playwright.sync_api import sync_playwright


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--credentials-file", type=pathlib.Path, required=True)
    parser.add_argument("--image", type=pathlib.Path, required=True)
    parser.add_argument("--expected-version", required=True)
    parser.add_argument("--serial", required=True)
    parser.add_argument("--log", type=pathlib.Path, required=True)
    parser.add_argument("--install", action="store_true", required=True)
    args = parser.parse_args()
    credentials = json.loads(args.credentials_file.read_text())
    base = credentials["url"]
    auth = "Basic " + base64.b64encode(
        (credentials["username"] + ":" + credentials["password"]).encode()
    ).decode()
    headers = {"Authorization": auth}

    def request(path, method="GET", body=None, extra=None, expected=200):
        merged = dict(headers)
        merged.update(extra or {})
        req = urllib.request.Request(base + path, data=body, headers=merged, method=method)
        try:
            with urllib.request.urlopen(req, timeout=30) as response:
                status, data = response.status, response.read()
        except urllib.error.HTTPError as error:
            status, data = error.code, error.read()
        if expected is not None:
            assert status == expected, (path, status)
        return data

    finished = threading.Event()
    observed = {"pubacks": 0, "booted": False, "loaded": False}
    log_fd = os.open(args.log, os.O_CREAT | os.O_EXCL | os.O_WRONLY, 0o600)
    port = serial.Serial(args.serial, 115200, timeout=0.1)
    port.reset_input_buffer()

    def monitor():
        pending = b""
        with os.fdopen(log_fd, "wb") as log:
            while not finished.is_set():
                data = port.read(2048)
                log.write(data)
                log.flush()
                pending += data
                while b"\n" in pending:
                    line, pending = pending.split(b"\n", 1)
                    if b"=== PURE STM32 CODE STARTING ===" in line:
                        observed["booted"] = True
                    if observed["booted"] and b"CONFIG: external record loaded" in line:
                        observed["loaded"] = True
                    if observed["loaded"] and b"MQTT delivery acknowledged by broker" in line:
                        observed["pubacks"] += 1

    thread = threading.Thread(target=monitor)
    thread.start()
    try:
        with sync_playwright() as playwright:
            browser = playwright.chromium.launch(headless=True)
            try:
                context = browser.new_context(http_credentials={
                    "username": credentials["username"], "password": credentials["password"]
                })
                page = context.new_page()
                page.goto(base + "/setup")
                page.wait_for_load_state("networkidle")
                settings = page.locator("form input").evaluate_all(
                    "(fields)=>Object.fromEntries(fields.filter(x=>x.name!=='password')"
                    ".map(x=>[x.name,x.value]))"
                )
                page.goto(base + "/ota")
                page.wait_for_load_state("networkidle")
                assert page.get_by_role("button", name="Upload BIN").count() == 1
                assert not page.evaluate("isSecureContext"), "Test must cover ordinary LAN HTTP"
                assert page.locator("#apply").is_disabled()
                page.locator("#image").set_input_files(str(args.image.resolve()))
                page.locator("#upload").click()
                page.wait_for_function(
                    "()=>!document.getElementById('apply').disabled", timeout=180000
                )
                print("Browser uploaded and verified image using plain-HTTP SHA-256 fallback", flush=True)
                staged = json.loads(request("/ota/status"))
                assert staged["state"] == "staged"
                csrf = {"X-CSRF-Token": staged["csrf"], "X-Firmware-SHA256": "0" * 64}
                request("/ota/apply", "POST", b"", csrf, 409)
                request("/reset", "POST", b"", csrf, 409)
                request("/ota/abort", "POST", b"", csrf)
                assert json.loads(request("/ota/status"))["state"] == "idle"
                csrf["X-Firmware-SHA256"] = hashlib.sha256(args.image.read_bytes()).hexdigest()
                request("/ota/apply", "POST", b"", csrf, 409)
                print("Wrong-image apply, administrative reset during OTA, and stale apply denied", flush=True)
                page.locator("#upload").click()
                page.wait_for_function(
                    "()=>!document.getElementById('apply').disabled", timeout=180000
                )
                page.once("dialog", lambda dialog: dialog.accept())
                page.locator("#apply").click()
                page.wait_for_function(
                    "()=>document.getElementById('status').textContent.includes('Rebooting')",
                    timeout=45000,
                )
                print("Browser explicitly applied verified firmware", flush=True)
                # Network readiness after an intentional reset is asynchronous.
                deadline = time.monotonic() + 100
                while time.monotonic() < deadline:
                    try:
                        if observed["loaded"] and (
                            "v" + args.expected_version
                        ).encode() in request("/", expected=None):
                            break
                    except (OSError, urllib.error.URLError):
                        pass
                    time.sleep(2)
                else:
                    raise AssertionError("Expected firmware version did not appear")
                page.goto(base + "/setup")
                page.wait_for_load_state("networkidle")
                after = page.locator("form input").evaluate_all(
                    "(fields)=>Object.fromEntries(fields.filter(x=>x.name!=='password')"
                    ".map(x=>[x.name,x.value]))"
                )
                assert settings == after, "Configuration changed across OTA"
                assert json.loads(request("/ota/status"))["state"] == "idle"
                deadline = time.monotonic() + 100
                while time.monotonic() < deadline and observed["pubacks"] < 2:
                    time.sleep(1)
                assert observed["booted"] and observed["loaded"], "Missing reboot/config evidence"
                assert observed["pubacks"] >= 2, "MQTT did not acknowledge twice after installation"
                print("Verified firmware " + args.expected_version +
                      ", unchanged settings, persistent authentication and two MQTT PUBACKs", flush=True)
            finally:
                browser.close()
    finally:
        finished.set()
        thread.join(timeout=3)
        port.close()


if __name__ == "__main__":
    main()
