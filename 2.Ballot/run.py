import subprocess
#subprocess.call("make")
GenAUs = ["g++", "-std=c++17", "GenAUs.cpp", "-o", "genAUs", "-O3", "-g"]
subprocess.call(GenAUs)

print("\n\n\n---------------- Workload 1 ----------------\n")

cmd = ["./genAUs", "100", "400", "50", "50", "1"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="OSTM")
subprocess.call(["python", "run.py"], cwd="MVOSTM")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n-------------------------------------------------\n")


cmd = ["./genAUs", "100", "400", "50", "100", "1"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="OSTM")
subprocess.call(["python", "run.py"], cwd="MVOSTM")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n-------------------------------------------------\n")


cmd = ["./genAUs", "100", "400", "50", "150", "1"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="OSTM")
subprocess.call(["python", "run.py"], cwd="MVOSTM")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n-------------------------------------------------\n")


cmd = ["./genAUs", "100", "400", "50", "200", "1"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="OSTM")
subprocess.call(["python", "run.py"], cwd="MVOSTM")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n-------------------------------------------------\n")


cmd = ["./genAUs", "100", "400", "50", "250", "1"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="OSTM")
subprocess.call(["python", "run.py"], cwd="MVOSTM")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n-------------------------------------------------\n")


cmd = ["./genAUs", "100", "400", "50", "300", "1"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="OSTM")
subprocess.call(["python", "run.py"], cwd="MVOSTM")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n-------------------------------------------------\n")
