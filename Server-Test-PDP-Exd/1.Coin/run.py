import subprocess

GenAUs = ["g++", "-std=c++17", "GenAUs.cpp", "-o", "genAUs", "-O3", "-g"]
subprocess.call(GenAUs)

########################  Workload 1  ##################################
print("\n\n\n---------------- Workload 1 ----------------\n")
cmd = ["./genAUs", "1000", "50", "1000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n-------------------------------------------------\n")


cmd = ["./genAUs", "1000", "50", "2000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n-------------------------------------------------\n")


cmd = ["./genAUs", "1000", "50", "3000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n-------------------------------------------------\n")


cmd = ["./genAUs", "1000", "50", "4000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n-------------------------------------------------\n")


cmd = ["./genAUs", "1000", "50", "5000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n-------------------------------------------------\n")


cmd = ["./genAUs", "1000", "50", "6000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n-------------------------------------------------\n")




########################  Workload 2  ##################################
print("\n\n\n---------------- Workload 2 ----------------\n")
cmd = ["./genAUs", "1000", "10", "3000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n-------------------------------------------------\n")


cmd = ["./genAUs", "1000", "20", "3000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n-------------------------------------------------\n")


cmd = ["./genAUs", "1000", "30", "3000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n-------------------------------------------------\n")


cmd = ["./genAUs", "1000", "40", "3000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n-------------------------------------------------\n")

cmd = ["./genAUs", "1000", "60", "3000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n-------------------------------------------------\n")


########################  Workload 3  ##################################
print("\n\n\n---------------- Workload 3 ----------------\n")

cmd = ["./genAUs", "2000", "50", "3000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n-------------------------------------------------\n")


cmd = ["./genAUs", "3000", "50", "3000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n-------------------------------------------------\n")


cmd = ["./genAUs", "4000", "50", "3000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n-------------------------------------------------\n")


cmd = ["./genAUs", "5000", "50", "3000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n-------------------------------------------------\n")


cmd = ["./genAUs", "6000", "50", "3000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n-------------------------------------------------\n")
