from sys import stdin, stdout

# take a whole line
temp_str = stdin.readline()

# separate key=value pairs
args = temp_str.split('&')

if len(args) <= 1:
    stdout.write('Error: Could not get parameters\r\n\r\n')
    exit(0)

d = dict()

# run through the pairs and separate keys and values
for keyval in args:
    try:
        pair = keyval.split('=')
        d[pair[0].strip()] = pair[1].strip()
    except ValueError:
        stdout.write('Error: Could not get parameters\r\n\r\n')
        exit(0)


# a necessary parameter is the amount of temperature
if 'temp' not in d:
    stdout.write('Error: No temperature was specified\r\n\r\n')
    exit(1)

# if a number can't be extracted, it's no good
try:
    t = float(d['temp'].replace('\x00', ''))
except ValueError:
    stdout.write('Error: ' + d['temp'] + ' is not a valid temperature\r\n\r\n')
    exit(2)

# if no scale is specified, no good either
if 'scale' not in d:
    stdout.write('Error: No scale was specified\r\n\r\n')
    exit(3)
else:
    # convert from Celsius to Fahrenheit
    if d['scale'] == 'c':
        stdout.write('{:.2f}ºC = {:.2f}ºF\r\n'
                     .format(t, (t*(9/5))+32))
    # convert from Fahrenheit to Celsius
    elif d['scale'] == 'f':
        stdout.write('{:.2f}ºF = {:.2f}ºC\r\n'
                     .format(t, (t-32)*(5/9)))
    # no other conversion is supported
    else:
        stdout.write('Error: ' + d['scale'].capitalize() + ' is not a valid scale\r\n\r\n')
        exit(3)

stdout.write('\r\n')
