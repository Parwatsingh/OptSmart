import subprocess
import os
if os.path.exists("inp-output/Time.csv"):
	os.remove("inp-output/Time.csv")
 
for i in range(1, 11):
	cmd = ["./serialMix", "11", "5", "2"]
	subprocess.call(cmd)
print("\n             +++++++++++++++++++++++++++++++++\n")
