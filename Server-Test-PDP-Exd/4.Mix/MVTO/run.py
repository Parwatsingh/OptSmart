import subprocess
import os
if os.path.exists("inp-output/Time.csv"):
	os.remove("inp-output/Time.csv")
 
#for i in range(1, 3):
#	cmd = ["./def-main-a1", "21", "5", "3"]
#	subprocess.call(cmd)
#print("\n             +++++++++++++++++++++++++++++++++\n")
for i in range(1, 3):
	cmd = ["./def-main-a2", "21", "5", "3"]
	subprocess.call(cmd)
print("\n             *********************************          \n")
