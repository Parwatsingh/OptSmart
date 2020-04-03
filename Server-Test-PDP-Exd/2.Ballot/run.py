import subprocess
#subprocess.call("make")
GenAUs = ["g++", "-std=c++17", "GenAUs.cpp", "-o", "genAUs", "-O3", "-g"]
subprocess.call(GenAUs)

print("\n\n\n---------------- Workload 1 ----------------\n")
cmd = ["./genAUs", "100", "900", "50", "1000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
#subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n-------------------------------------------------\n")


cmd = ["./genAUs", "100", "900", "50", "2000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
#subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n-------------------------------------------------\n")


cmd = ["./genAUs", "100", "900", "50", "3000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
#subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n-------------------------------------------------\n")


cmd = ["./genAUs", "100", "900", "50", "4000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
#subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n-------------------------------------------------\n")


cmd = ["./genAUs", "100", "900", "50", "5000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
#subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n-------------------------------------------------\n")


cmd = ["./genAUs", "100", "900", "50", "6000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
#subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n-------------------------------------------------\n")


print("\n\n\n---------------- Workload 2 ----------------\n")
cmd = ["./genAUs", "100", "900", "10", "3000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
#subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n-------------------------------------------------\n")


cmd = ["./genAUs", "100", "900", "20", "2000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
#subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n-------------------------------------------------\n")


cmd = ["./genAUs", "100", "900", "30", "3000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
#subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n-------------------------------------------------\n")


cmd = ["./genAUs", "100", "900", "40", "4000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
#subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n-------------------------------------------------\n")


cmd = ["./genAUs", "100", "900", "60", "5000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
#subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n-------------------------------------------------\n")


print("\n\n\n---------------- Workload 3 ----------------\n")

cmd = ["./genAUs", "200", "1800", "50", "2000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
#subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n-------------------------------------------------\n")


cmd = ["./genAUs", "300", "2700", "50", "3000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
#subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n-------------------------------------------------\n")


cmd = ["./genAUs", "400", "3600", "50", "4000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
#subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n-------------------------------------------------\n")


cmd = ["./genAUs", "500", "4500", "50", "5000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
#subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n-------------------------------------------------\n")


cmd = ["./genAUs", "600", "5400", "50", "6000", "0"]
subprocess.call(cmd)
subprocess.call(["python", "run.py"], cwd="Serial")
subprocess.call(["python", "run.py"], cwd="BTO-STM")
subprocess.call(["python", "run.py"], cwd="MVTO")
subprocess.call(["python", "run.py"], cwd="Spec-Bin")
#subprocess.call(["python", "run.py"], cwd="Static-Bin")
print("\n-------------------------------------------------\n")
