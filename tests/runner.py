import os
import subprocess
import sys

allium = sys.argv[1]

tests_run = 0
tests_passed = 0
failed_tests = []

def run(name):
    global tests_run, tests_passed
    print("Running test", name)
    print(" ".join([allium, name]))
    result = subprocess.run([allium, name])
    tests_run += 1
    if result.returncode == 0:
        print("Passed.")
        tests_passed += 1
    else:
        print("Failed.")
        failed_tests.append(name)

if __name__ == "__main__":
    testdir = os.path.dirname(os.path.realpath(__file__))

    for file in os.listdir(testdir):
        if(file.endswith(".allium")):
            full_test_path = os.path.join(testdir, file)
            run(full_test_path)
    
    if failed_tests:
        print("Failing tests:")
        for test in failed_tests:
            print("\t", test)
    print(tests_passed, "/", tests_run, "passed")
    sys.exit(tests_run != tests_passed)
