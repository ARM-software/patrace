#!/bin/bash

export PATH="${PATH}:/work/android-sdk/platform-tools"

source /work/python27_virtualenv/bin/activate

./content_capturing/scripts/test_automation/performance_regression_test/performance_regression_test.py
