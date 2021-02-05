#!/usr/bin/env python3

"""
This script will generate screenshots for all samples inside assets/screenshots/ScreenshotsData/,
and save the status in assets/screenshots/ScreenshotsData/aa_runStatus.json

After this, it will invoke compare_screenshot_git (which will compare all screenshots
with the previous git version and keep only the screenshots with heavy visual modifications)
"""
import os
import os.path
import subprocess
import time
import json
import compare_screenshots_git

THIS_SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_DIR = THIS_SCRIPT_DIR + "/../"
SCREENSHOTS_FOLDER = REPO_DIR + "/assets/screenshots/ScreenshotsData/"
BUILD_DIR = REPO_DIR + "/build/"
# BUILD_DIR = REPO_DIR + "/cmake-build-relwithdebinfo/"
BABYLON_STUDIO_EXE = BUILD_DIR + "/build/bin/BabylonStudio"
STATUS_FILE= REPO_DIR + "/assets/screenshots/ScreenshotsData/aa_runStatus.json"

def run_one_sample(sample_name):
    max_duration = 15
    try:
        completed_process = subprocess.run(
            [BABYLON_STUDIO_EXE, "--sample", sample_name, "--shot-one-sample"],
            capture_output = True,
            timeout=max_duration)
    except subprocess.TimeoutExpired as e:
        return "tooSlowOrHung"
    except Exception as e:
        return "unknown"
    if completed_process.returncode == 0:
        return "success"
    else:
        return "unhandledException"


def list_samples():
    completed_process = subprocess.run([BABYLON_STUDIO_EXE, "-l"], capture_output = True, check = True, timeout=None)
    samples_str = completed_process.stdout.decode("utf-8")
    samples = samples_str.split("\n")
    samples = list(filter(lambda s: len(s) > 0, samples))
    samples.sort()
    return samples


def run_all_samples():
    sample_names = list_samples()
    run_statuses = {}
    nb = len(sample_names)
    for i, category_sample_name in enumerate(sample_names):
        sample_name = category_sample_name[category_sample_name.index("/") + 1 :]
        print(f"Running sample {i}/{nb}: {sample_name}", end=" ---> ")
        run_result = run_one_sample(sample_name)
        run_statuses[sample_name] = run_result
        print(run_result)

    json_status= json.dumps(run_statuses, indent = 4)
    print(json_status)
    with open(STATUS_FILE, "w") as f:
        f.write(json_status)


if __name__ == "__main__":
    run_all_samples()
    compare_screenshots_git.do_compare()
