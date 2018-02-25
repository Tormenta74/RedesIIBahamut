from sys import stdin

temp_str = stdin.readline()
temp = float(temp_str.strip().replace('\x00', ''))
print("Hi. I'm just here to mess things up.")
print("{:.2f}ºC = {:.2f}ºF".format(temp, temp*1.8 + 32))
print("\r")
