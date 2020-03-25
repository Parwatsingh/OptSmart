import subprocess
for i in range(1, 6):
	cmd = ["./serialCoin", "20", "2", "20"]
	subprocess.call(cmd)
print("\n                  *********************************               \n")
