import subprocess

GenAUs = ["g++", "-std=c++17", "GenAUs.cpp", "-o", "genAUs", "-O3", "-g"]
subprocess.call(GenAUs)

print("\n\n\n---------------- Workload 1 ----------------\n")
cmd = ["./genAUs", "50", "500", "334", "64", "268", "334", "6000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial") 
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n--------------------------------------------------------------\n")


cmd = ["./genAUs", "50", "1000", "334", "64", "268", "334", "6000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial") 
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n--------------------------------------------------------------\n")


cmd = ["./genAUs", "50", "1500", "334", "64", "268", "334", "6000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial") 
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n--------------------------------------------------------------\n")


cmd = ["./genAUs", "50", "2000", "334", "64", "268", "334", "6000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial") 
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n--------------------------------------------------------------\n")


cmd = ["./genAUs", "50", "2500", "334", "64", "268", "334", "6000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial") 
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n--------------------------------------------------------------\n")


cmd = ["./genAUs", "50", "3000", "334", "64", "268", "334", "6000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial") 
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n--------------------------------------------------------------\n")


############################ Workload 2 ####################################
print("\n\n\n---------------- Workload 2 ----------------\n")
cmd = ["./genAUs", "10", "1000", "334", "64", "268", "334", "6000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial") 
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n--------------------------------------------------------------\n")


cmd = ["./genAUs", "20", "1000", "334", "64", "268", "334", "6000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial") 
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n--------------------------------------------------------------\n")


cmd = ["./genAUs", "30", "1000", "334", "64", "268", "334", "6000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial") 
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n--------------------------------------------------------------\n")


cmd = ["./genAUs", "40", "1000", "334", "64", "268", "334", "6000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial") 
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n--------------------------------------------------------------\n")


cmd = ["./genAUs", "60", "1000", "334", "64", "268", "334", "6000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial") 
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n--------------------------------------------------------------\n")


############################ Workload 3 ####################################
print("\n\n\n---------------- Workload 3 ----------------\n")
cmd = ["./genAUs", "50", "1000", "334", "64", "268", "334", "6000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial") 
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n--------------------------------------------------------------\n")


cmd = ["./genAUs", "50", "1000", "668", "128", "536", "668", "6000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial") 
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n--------------------------------------------------------------\n")


cmd = ["./genAUs", "50", "1000", "1002", "192", "784", "1002", "6000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial") 
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n--------------------------------------------------------------\n")


cmd = ["./genAUs", "50", "1000", "1336", "256", "1052", "1336", "6000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial") 
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n--------------------------------------------------------------\n")


cmd = ["./genAUs", "50", "1000", "1670", "320", "1340", "1670", "6000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial") 
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n--------------------------------------------------------------\n")
