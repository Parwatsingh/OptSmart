import subprocess
import os
if os.path.exists("inp-output/Time.csv"):
	os.remove("inp-output/Time.csv")
 
for i in range(1, 6):
	cmd = ["./def-main", "20", "2", "20"]
	subprocess.call(cmd)
print("\n             +++++++++++++++++++++++++++++++++\n")
for i in range(1, 6):
	cmd = ["./scv-main", "20", "2", "20"]
	subprocess.call(cmd)
print("\n             *********************************          \n")
