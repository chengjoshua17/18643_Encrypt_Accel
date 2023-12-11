# Used to calculate grouping ratios of different data

datafile = open('data_ideal.txt', 'r')
Lines = datafile.readlines()

linecount = 0
diffcount = 0

# Get first line
firstline = Lines[0]
Lines.pop(0)
lastline = 0

if firstline.startswith('Paillier'):
    lastline = 0 #Paillier
elif firstline.startswith('AES'):
    lastline = 1 #AES
else:
    print("Invalid Data Line!")
    exit(-1)

# Parse Line
for line in Lines:
    linecount = linecount + 1

    if line.startswith('Paillier'):
        currline = 0 #Paillier
    elif line.startswith('AES'):
        currline = 1 #AES
    else:
        print("Invalid Data Line!")
        exit(-1)
    
    if (currline != lastline):
        diffcount = diffcount + 1
    
    lastline = currline

gratio = 100 - (diffcount / linecount) * 100

print(f'Line Count: {linecount}')
print(f'Diff Count: {diffcount}')
print(f'Grouping Ratio: {gratio}%')
    

