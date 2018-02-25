from sys import stdin

temp_str = stdin.readline()
temp = float(temp_str)
print("{:.2f}ºC = {:.2f}ºF".format(temp, temp*1.8 + 32))
