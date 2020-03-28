import subprocess
#subprocess.call("make")
GenAUs = ["g++", "-std=c++17", "GenAUs.cpp", "-o", "genAUs", "-O3", "-g"]
subprocess.call(GenAUs)

print("\n\n\n---------------- Workload 1 ----------------\n")

cmd = ["./genAUs", "200", "800", "50", "500", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n-------------------------------------------------\n")


cmd = ["./genAUs", "200", "800", "50", "1000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n-------------------------------------------------\n")


cmd = ["./genAUs", "200", "800", "50", "1500", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n-------------------------------------------------\n")


cmd = ["./genAUs", "200", "800", "50", "2000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n-------------------------------------------------\n")


cmd = ["./genAUs", "200", "800", "50", "2500", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n-------------------------------------------------\n")


cmd = ["./genAUs", "200", "800", "50", "3000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n-------------------------------------------------\n")
