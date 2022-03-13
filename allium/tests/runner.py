import os
import subprocess
import sys
import tempfile

allium = sys.argv[1]
filecheck = sys.argv[2]

tests_run = 0
tests_passed = 0
failed_tests = []

def run(name):
    global tests_run, tests_passed
    print("Running test", name)

    # (allium -i $name --log-level=2 ; echo "Exit code:" $?) | FileCheck $name
    with tempfile.TemporaryFile() as tracefile:
        exe = subprocess.run([allium, "-i", name, "--log-level=2"], stdout=tracefile, stderr=subprocess.STDOUT)
        tracefile.write(f"Exit code: {exe.returncode}".encode('utf8'))
        tracefile.seek(0)
        result = subprocess.run([filecheck, name], input=tracefile.read())

    tests_run += 1
    if result.returncode == 0:
        print("Passed.")
        tests_passed += 1
    else:
        print("Failed.")
        failed_tests.append(name)

if __name__ == "__main__":
    testdir = os.path.dirname(os.path.realpath(__file__))

    for dirpath, _, files in os.walk(testdir):
        for file in files:
            if(file.endswith(".allium")):
                full_test_path = os.path.join(dirpath, file)
                run(full_test_path)

    if failed_tests:
        print("Failing tests:")
        for test in failed_tests:
            print("\t", test)
    print(tests_passed, "/", tests_run, "passed")
    sys.exit(tests_run != tests_passed)
