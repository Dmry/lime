import matplotlib.pyplot as plt
import csv
import os
import argparse

parser = argparse.ArgumentParser(description='Plot g(t) values from file.')
parser.add_argument('input', nargs='+', type=argparse.FileType('r'))

args = parser.parse_args()

plt.xlabel('t')
plt.ylabel('g(t)')

for input_file in args.input:
    x = []
    y = []

    csv_reader = csv.reader(filter(lambda row: row[0]!='#', input_file), delimiter=' ')

    for row in csv_reader:
        x.append(float(row[0]))
        y.append(float(row[1]))

    plt.loglog(x,y, '+', label=os.path.splitext(os.path.basename(input_file.name))[0])

plt.legend()
plt.show()

    