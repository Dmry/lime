import sys
import argparse
import csv

parser = argparse.ArgumentParser(description='Rescale the values of a column in a csv file.')

parser.add_argument('input', type=argparse.FileType('r'))
parser.add_argument('-c', metavar='columns', required=True, type=int, nargs='+', help='Columns to rescale')
parser.add_argument('scale_factor', type=float)
parser.add_argument('outfile', type=argparse.FileType('w'), help="Write rescaled variables to file")
parser.add_argument('--comment', action='store_true', default='store_false', help='Insert comment at the start about the file how the original was rescaled')

args = parser.parse_args()

if args.input == args.outfile:
    sys.exit("Input and output file cannot be the same!") 

csv_reader = csv.reader(args.input, delimiter=' ')
csv_writer = csv.writer(args.outfile, delimiter=' ')

if args.comment:
    args.outfile.write(f'# Column(s) {args.c} of the original file scaled by {args.scale_factor}\n')

line_count = 0
for row in csv_reader:
    if row[0].strip()[0] != '#':
        for index in args.c:
            row[index] = float(row[index]) * args.scale_factor
    csv_writer.writerow(row)
    line_count += 1

print(f'Wrote {line_count} lines')