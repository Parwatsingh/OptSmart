import subprocess
import os
if os.path.exists("inp-output/Time.csv"):
	os.remove("inp-output/Time.csv")
 
for i in range(1, 6):
	cmd = ["./serialAuction", "20", "50", "5"]
	subprocess.call(cmd)
print("\n             +++++++++++++++++++++++++++++++++\n")
