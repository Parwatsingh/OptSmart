import subprocess
import os
if os.path.exists("inp-output/Time.csv"):
	os.remove("inp-output/Time.csv")
 
for i in range(1, 6):
	cmd = ["./def-main", "10", "10", "5"]
	subprocess.call(cmd)
print("\n             +++++++++++++++++++++++++++++++++\n")
for i in range(1, 6):
	cmd = ["./scv-main", "10", "10", "5"]
	subprocess.call(cmd)
print("\n             *********************************          \n")
