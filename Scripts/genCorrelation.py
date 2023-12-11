# Used to generate data of different grouping ratios

import random

random.seed(18643)

# We issue in groups of 3 for Paillier and groups of 2 for AES (to maintain approx 1.5 ratio)
grouping = 40
maxPaillierIssue = 3 * grouping
maxAESIssue = 2 * grouping

# Create file
f = open("gendata.txt", "w")

numPaillier = 798000
numAES = 482240

while ((numAES > maxAESIssue) and (numPaillier > maxPaillierIssue)):
    for i in range(grouping):
        f.write("Paillier," + str(random.getrandbits(64)) + "\n")
        f.write("Paillier," + str(random.getrandbits(64)) + "\n")
        f.write("Paillier," + str(random.getrandbits(64)) + "\n")
        numPaillier = numPaillier - 3
    
    for i in range(grouping):
        f.write("AES," + str(random.getrandbits(64)) + "\n")
        f.write("AES," + str(random.getrandbits(64)) + "\n")
        numAES = numAES - 2

for i in range(numPaillier):
    f.write("Paillier," + str(random.getrandbits(64)) + "\n")

for i in range(numAES):
    f.write("AES," + str(random.getrandbits(64)) + "\n")

f.close()