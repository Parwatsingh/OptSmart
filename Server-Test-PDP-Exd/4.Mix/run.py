import subprocess

GenAUs = ["g++", "-std=c++17", "GenAUs.cpp", "-o", "genAUs", "-O3", "-g"]
subprocess.call(GenAUs)

print("\n\n\n---------------- Workload 1 ----------------\n")
################ Warmup-Run ##################
cmd = ["./genAUs", "50", "900", "334", "64", "268", "334", "6000", "0"]
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


cmd = ["./genAUs", "50", "2000", "334", "64", "268", "334", "6000", "0"]
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


cmd = ["./genAUs", "50", "4000", "334", "64", "268", "334", "6000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial") 
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n--------------------------------------------------------------\n")


cmd = ["./genAUs", "50", "5000", "334", "64", "268", "334", "6000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial") 
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n--------------------------------------------------------------\n")


cmd = ["./genAUs", "50", "6000", "334", "64", "268", "334", "6000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial") 
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n--------------------------------------------------------------\n")


############################ Workload 2 ####################################
print("\n\n\n---------------- Workload 2 ----------------\n")
cmd = ["./genAUs", "10", "3000", "334", "64", "268", "334", "6000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial") 
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n--------------------------------------------------------------\n")


cmd = ["./genAUs", "20", "3000", "334", "64", "268", "334", "6000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial") 
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n--------------------------------------------------------------\n")


cmd = ["./genAUs", "30", "3000", "334", "64", "268", "334", "6000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial") 
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n--------------------------------------------------------------\n")


cmd = ["./genAUs", "40", "3000", "334", "64", "268", "334", "6000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial") 
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n--------------------------------------------------------------\n")


cmd = ["./genAUs", "60", "3000", "334", "64", "268", "334", "6000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial") 
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n--------------------------------------------------------------\n")


############################ Workload 3 ####################################


cmd = ["./genAUs", "50", "3000", "668", "128", "536", "668", "6000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial") 
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n--------------------------------------------------------------\n")


cmd = ["./genAUs", "50", "3000", "1002", "192", "784", "1002", "6000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial") 
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n--------------------------------------------------------------\n")


cmd = ["./genAUs", "50", "3000", "1336", "256", "1052", "1336", "6000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial") 
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n--------------------------------------------------------------\n")


cmd = ["./genAUs", "50", "3000", "1670", "320", "1340", "1670", "6000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial") 
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n--------------------------------------------------------------\n")

cmd = ["./genAUs", "50", "3000", "2004", "384", "1608", "2004", "6000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial") 
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n--------------------------------------------------------------\n")
